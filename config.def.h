/* execv arguments to run Shen */
char *command[] = {"shen", 0};

/* name of initialization file */
char *confname = ".shen.shen";

/* name of file where error message is saved */
#define ERRFILE "shen.err"

/* command to exit
  on Shen SBCL it's (SB-UNIX:EXIT Ecode)
  on Shen CLisp it's (EXIT Ecode)
  on Shen JS it's (shenjs-exit Ecode) */
char *exit_expr = "(SB-UNIX:EXIT Ecode)\n";

char *start_expr = "(set shen-*history* [])\n";
char *main_func = "main";
char *conf = 0;

#define DEVNULL "/dev/null"
