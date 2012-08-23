char *command[] = {"shen-cl", 0};
char *start_expr = "(set shen-*history* [])\n";
char *exit_expr = "(QUIT)\n";
char *main_func = "main";
char *confname = ".shen.shen";
char *conf = 0;

#define DEVNULL "/dev/null"
#define ERRFILE "shen.err"
