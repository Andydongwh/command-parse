#include "command_parse.h"

static  char *const usages[] = {
    "test_argparse [options] [[--] args]",
    "test_argparse [options]",
    NULL,
};

#define PERM_READ  (1<<0)
#define PERM_WRITE (1<<1)
#define PERM_EXEC  (1<<2)
static int glob = 0;
gboolean funct(chassis_options_t *self, const chassis_option_t *option)
{
    // when argument ar is found in argv, this function is called;
    char **value = * (char ***) option->value;
    if (value != NULL) {
        while (*value) {
            glob++;
            value++;
        }
    }
    return TRUE;
}
int
main(gint argc, gchar **argv)
{
    int i;
    int force = 0;
    int test = 0;
    int num = 0;
    const char **ar = NULL;
    long int lnum = 0;
    double mum = 0.0;
    const char *path = NULL;
//    chassis_options_t chassis_options;
//    chassis_options_t *opts = &chassis_options;
/*    chassis_option_t option = {CHASSIS_OPTIONS_BOOLEAN, 'h', "help", NULL,
        "show this help message and exit", &argparse_help_cb, 0, 0};
*/
//    chassis_options_init(opts, NULL, NULL, 0);
    chassis_options_t *opts = chassis_options_new();
    chassis_options_add(opts,
        CHASSIS_OPTIONS_GROUP, 0, NULL, NULL,  
        "Basic options", NULL);

    chassis_options_add(opts,
        CHASSIS_OPTIONS_BOOLEAN, 'f', "force", &force,
        "force to do", NULL);

    chassis_options_add(opts,
        CHASSIS_OPTIONS_STRING, 'p', "path", &path, 
        "path to read", NULL);

    chassis_options_add(opts,
        CHASSIS_OPTIONS_DOUBLE, 'd', "double", &mum,
        "double number", NULL);

    chassis_options_add(opts,
        CHASSIS_OPTIONS_ARRAY, 'a', "array", &ar, 
        "path to read", NULL);

    chassis_options_add(opts,
        CHASSIS_OPTIONS_INTEGER, 'n', "num", &num,
        "selected num", NULL);
    
    chassis_options_add(opts,
        CHASSIS_OPTIONS_LONG, 'l', "long", &lnum,
        "selected long int num", NULL);

    if (chassis_options_parse(opts, argc, argv, 1)) {
        printf("argc: %d\n", argc);
        for (i = 0; i < argc; i++) {
            printf("argv[%d]: %s\n", i, *(argv + i));
        }
    }
    if (force)
        printf("force: %d\n", force);
    if (path)
        printf("path: %s\n", path);
    if (num)
        printf("num: %d\n", num);
    if (lnum)
        printf("lnum: %ld\n", lnum);
    if (mum)
        printf("mum: %lf\n", mum);
    if (ar[0]){
        for (i = 0; strlen(*ar); ++i){
            printf("ar[%d]: %s\n", i, ar[i]);
        }
    }
 
    return 0;
}
