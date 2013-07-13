#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pty.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "config.h"
#include "script.h"

pid_t pid;
char *err_name = 0;
int err = -1;
int err_bytes = 0;
int exit_on_eof = 1;
int running = 1;

char *err_expr = ""
"(define shen-run.call-with-err\n"
"  F -> (trap-error (thaw F)\n"
"                   (/. E (let F (open file (value shen-run.error) out)\n"
"                              - (pr (error-to-string E) F)\n"
"                              - (pr (n->string (shen.newline)) F)\n"
"                              - (close F)\n"
"                           %s))))\n";

void
usage(const char *argv0)
{
  fprintf(stderr, "Usage: %s [-nc] [-ne] [file.shen] [args...]\n", argv0);
}

void
write_escaped(int fd, const char *s)
{
  int i, j = 0, n;
  static char buf[1024];

  for (i = 0; *s; ++i, ++s) {
    switch (*s) {
      case 0: case '"': case '\n': case '\r': case '\\':
        n = 5;
        break;
      default:
        n = 1;
        buf[j++] = *s;
    }
    if (j + n >= sizeof(buf)) {
      write(fd, buf, j);
      j = 0;
    }
    if (n > 1)
      j += snprintf(buf + j, sizeof(buf) - j, "c#%d;", *s);
  }
  if (j)
    write(fd, buf, j);
}

int
init_err_fd()
{
  static char buf[256];
  if (!err_name) {
    snprintf(buf, sizeof(buf), "%s", err_tpl);
    err_name = mktemp(buf);
  }
  if (mkfifo(err_name, S_IRWXU))
    return -1;
  err = open(err_name, O_RDONLY | O_NONBLOCK, 0);
  return 0;
}

int
load_conf(int fd)
{
  static char buf[1024], fbuf[1024];
  char *home, *fname = conf, *s;
  int n;

  if (!fname) {
    home = getenv("HOME");
    if (!home)
      return -1;
    snprintf(fbuf, sizeof(fbuf), "%s/%s", home, confname);
    fname = fbuf;
  }
  if (access(fname, R_OK))
    return 0;
  s = "(shen-run.call-with-err (freeze (load \"%s\")))\n";
  n = snprintf(buf, sizeof(buf), s, fname);
  if (write(fd, buf, n) < 0)
    return -1;
  return 0;
}

int
init_shen(int fd, int argc, char **argv)
{
  static char buf[1024];
  char *s;
  int i, n;

  if (init_err_fd())
    return -1;

  s = "(set shen-run.error \"";
  write(fd, s, strlen(s));
  write_escaped(fd, err_name);
  s = "\")\n";
  write(fd, s, strlen(s));

  n = snprintf(buf, sizeof(buf), err_expr, exit_expr);
  if (write(fd, buf, n) < 0)
    return -1;

  if (confname && load_conf(fd) < 0)
    return -1;
  s = "(define shen-run.exit -> %s)\n";
  n = snprintf(buf, sizeof(buf), s, exit_expr);
  if (write(fd, buf, n) < 0)
    return -1;
  if (write(fd, start_expr, strlen(start_expr)) < 0)
    return -1;
  if (argc) {
    if (write(fd, run_expr, strlen(run_expr)) < 0)
      return -1;
    for (i = 1; i < argc; ++i) {
      s = "(shen-run.add-arg \"";
      write(fd, s, strlen(s));
      write_escaped(fd, argv[i]);
      s = "\")\n";
      write(fd, s, strlen(s));
    }
    s = "(shen-run.execute (reverse (value shen-run.args)) \"%s\" %s)\n";
    n = snprintf(buf, sizeof(buf), s, argv[0], main_func);
    if (write(fd, buf, n) < 0)
      return -1;
  }
  fsync(fd);
  return 0;
}

int
pump_data(int from, int to)
{
  static char buf[1024];
  size_t read_bytes;

  read_bytes = read(from, buf, sizeof(buf));
  if (read_bytes > 0) {
    write(to, buf, read_bytes);
    fsync(to);
  }
  return read_bytes;
}

int
pump_error(int from, int to)
{
  static char buf[256];
  size_t read_bytes, i = 0;

  read_bytes = read(from, buf, sizeof(buf));
  if (read_bytes > 0) {
    i = (!err_bytes && buf[0] == '\n') ? 1 : 0;
    if (!err_bytes && i == 0) {
      write(to, err_prefix, strlen(err_prefix));
    }
    err_bytes += read_bytes - i;
    write(to, buf + i, read_bytes - i);
    fsync(to);
  }
  return read_bytes;
}

int
eat_data(int from, int *pstarted)
{
  char buf[8];
  int i, rbytes, started = *pstarted;
  static int state = 0;

  rbytes = read(from, buf, 1);
  if (rbytes > 0)
    for (i = 0; i < rbytes && !started; ++i)
      switch (state) {
      case 0:
        if (buf[i] == '\n')
          state = 1;
        else
          state = 0;
        break;
      case 1:
        if (buf[i] == '\n')
          break;
      case 2: case 3: case 4: case 5: case 6:
        if (buf[i] == '"')
          ++state;
        else
          state = 0;
        if (state == 6)
          started = 1;
        break;
      default: state = 0;
      }
  *pstarted = started;
  return rbytes;
}

int
serve_process(int fd, int err, int initialized)
{
  fd_set fds;
  int res, stdin_closed = 0;

  for (running = 1; running;) {
    FD_ZERO(&fds);
    if (!stdin_closed)
      FD_SET(0, &fds);
    FD_SET(fd, &fds);
    FD_SET(err, &fds);

    res = select(((fd > err) ? fd : err) + 1, &fds, 0, 0, 0);
    if (res > 0) {
      if (FD_ISSET(0, &fds)) {
        if (pump_data(0, fd) <= 0) {
          stdin_closed = 1;
          if (exit_on_eof)
            break;
        }
      }
      if (FD_ISSET(err, &fds)) {
        if (pump_error(err, 2) <= 0)
          if (init_err_fd())
            break;
      }
      if (FD_ISSET(fd, &fds)) {
        if (initialized) {
          if (pump_data(fd, 1) < 0)
            break;
        } else {
          if (eat_data(fd, &initialized) < 0)
            break;
        }
      }
    } else {
      if (errno != EINTR)
        break;
    }
  }
  return 0;
}

static int
set_noecho(int fd)
{
  struct termios stermios;

  if (tcgetattr(fd, &stermios) < 0)
    return 1;

  stermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
  stermios.c_lflag |= ICANON;
  /* stermios.c_oflag &= ~(ONLCR); */
  /* would also turn off NL to CR/NL mapping on output */
  stermios.c_cc[VERASE] = 0177;
#ifdef VERASE2
  stermios.c_cc[VERASE2] = 0177;
#endif
  if (tcsetattr(fd, TCSANOW, &stermios) < 0)
    return 1;
  return 0;
}

void
handle_sigint(int sig)
{
  signal(SIGINT, handle_sigint);
  kill(pid, SIGINT);
}

void
handle_sighup(int sig)
{
  running = 0;
}

int
main(int argc, char **argv)
{
  int i, fd, ret = 0;

  for (i = 1; i < argc; ++i)
    if (strcmp(argv[i], "--") == 0)
      break;
    else if (strcmp(argv[i], "-h") == 0) {
      usage(argv[0]);
      return 1;
    }
    else if (strcmp(argv[i], "-nc") == 0)
      confname = 0;
    else if (strcmp(argv[i], "-ne") == 0)
      exit_on_eof = 0;
    else
      break;
  signal(SIGINT, handle_sigint);

  if (ret == 0) {
    ret = 0;
    switch ((pid = forkpty(&fd, 0, 0, 0))) {
      case -1:
        perror("forkpty");
        return 1;

      case 0:
        set_noecho(0);
        execvp(command[0], command);
        perror("exec");
        return 1;

      default:
        signal(SIGHUP, handle_sighup);
        init_shen(fd, argc - i, argv + i);
        serve_process(fd, err, argc == i);
        ret = 0;
        close(fd);
        wait(0);
        if (err_bytes)
          ret = 1;
        if (err >= 0)
          close(err);
        if (err_name)
          remove(err_name);
    }
  } else
    usage(argv[0]);
  return ret;
}
