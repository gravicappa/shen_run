/* execv arguments to run Shen */
char *command[] = {"shen", 0};

/* name of initialization file */
char *confname = ".shen.shen";

/* command to exit
  on Shen Common lisp it's ((protect QUIT))
  on Shen-js it's (shenjs.exit) 
  on Shen-py it's (shenpy.exit) 
  */
char *exit_expr = "((protect QUIT))\n";

/* Error message prefix */
char *err_prefix = "\nError: ";

char *start_expr = "(set shen.*history* [])\n";
char *main_func = "main";
char *err_tpl = "/tmp/shen_run.XXXXXX";
char *conf = 0;

#define DEVNULL "/dev/null"
