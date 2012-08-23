#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pty.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "config.h"
#include "script.h"

pid_t pid;

void
usage(const char *argv0)
{
  fprintf(stderr, "Usage: %s [-nc] [file.shen] [args...]\n", argv0);
}

void
write_arg(int fd, const char *arg)
{
  int i, j = 0, n, x;
  static char buf[1024];

  for (i = 0; *arg; ++i, ++arg) {
    switch (*arg) {
      case 0: case '"': case '\n': case '\r': case '\\':
        n = 5;
        break;
      default:
        n = 1;
        buf[j++] = *arg;
    }
    if (j + n >= sizeof(buf)) {
      write(fd, buf, j);
      j = 0;
    }
    if (n > 1)
      j += snprintf(buf + j, sizeof(buf) - j, "c#%d;", *arg);
  }
  if (j)
    write(fd, buf, j);
}

int
load_conf(int fd)
{
  static char buf[1024], fbuf[1024];
  char *home, *fname = conf;
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
  n = snprintf(buf, sizeof(buf), "(load \"%s\")\n", fname);
  if (write(fd, buf, n) < 0)
    return -1;
  return 0;
}

int
init_shen(int fd, int argc, char **argv)
{
  char buf[1024], *s;
  int i, n;

  if (confname && load_conf(fd) < 0)
    return -1;
  if (write(fd, start_expr, strlen(start_expr)) < 0)
    return -1;
  if (argc) {
    if (write(fd, run_expr, strlen(run_expr)) < 0)
      return -1;
    for (i = 1; i < argc; ++i) {
      s = "(script-add-arg \"";
      write(fd, s, strlen(s));
      write_arg(fd, argv[i]);
      s = "\")";
      write(fd, s, strlen(s));
    }
    s = "(script-execute (reverse (value script-args)) \"%s\" %s)%s\n";
    n = snprintf(buf, sizeof(buf), s, argv[0], main_func, exit_expr);
    if (write(fd, buf, n) < 0)
      return -1;
  }
  fsync(fd);
  return 0;
}

int
pump_data(int from, int to)
{
  char buf[1024];
  size_t read_bytes;

  read_bytes = read(from, buf, sizeof(buf));
  if (read_bytes > 0) {
    write(to, buf, read_bytes);
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
      case 2:
      case 3:
        if (buf[i] == '"')
          ++state;
        else
          state = 0;
        if (state == 4)
          started = 1;
        break;
      default: state = 0;
      }
  *pstarted = started;
  return rbytes;
}

int
serve_process(int fd, int initialized)
{
  fd_set fds;
  int res;

  for (;;) {
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    FD_SET(fd, &fds);

    res = select(fd + 1, &fds, 0, 0, 0);
    if (res > 0) {
      if (FD_ISSET(0, &fds)) {
        if (pump_data(0, fd) <= 0)
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

int
main(int argc, char **argv)
{
  int i, fd, ret = 0;

  for (i = 1; i < argc; ++i)
    if (strcmp(argv[i], "--") == 0)
      break;
    else if (strcmp(argv[i], "-h") == 0)
      usage(argv[0]);
    else if (strcmp(argv[i], "-nc") == 0)
      confname = 0;
    else
      break;
  signal(SIGINT, handle_sigint);

  if (ret == 0) {
    ret = 0;
    switch ((pid = forkpty(&fd, 0, 0, 0))) {
      case -1:
        perror("forkpty");
        return -1;

      case 0:
        set_noecho(0);
        execvp(command[0], command);
        perror("exec");
        return -1;

      default:
        init_shen(fd, argc - i, argv + i);
        serve_process(fd, argc == i);
        ret = 0;
        close(fd);
        wait(0);
    }
  } else
    usage(argv[0]);
  return ret;
}
