/* execv arguments to run Shen */
char *command[] = {"shen", 0};

/* name of initialization file */
char *confname = ".shen.shen";

/* name of file where error message is saved */
#define ERRFILE "shen.err"

/* command to exit
  on Shen SBCL it's (QUIT)
  on Shen CLisp it's (EXIT Ecode)
  on Shen JS it's (shenjs-exit Ecode) */
char *exit_expr = "(QUIT)\n";

char *start_expr = "(set shen-*history* [])\n";
char *main_func = "main";
char *conf = 0;

#define DEVNULL "/dev/null"
