#include "command_parse.h"
#include <glib.h>

#define OPT_UNSET 1
#define OPT_LONGF  1 << 1
#define ORIGIN  1
#define SUCESS  1
#define FAIL  0
#define UNKNOWN  2


static gboolean helpBool = FALSE;

static const GList *optHead = NULL;

static char *
prefix_skip(char *str, const char *prefix)
{
    int len = strlen(prefix);
    return strncmp(str, prefix, len) ? NULL : str + len;
}

static int
prefix_cmp(const char *str, const char *prefix)
{
    for (;; str++, prefix++) {
        if (!*prefix) {
        	return 0;
        } else if (*str != *prefix) {
            return (unsigned char)*prefix - (unsigned char)*str;
        }
    }
}

static gboolean
argparse_error(chassis_options_t *self, const chassis_option_t *opt,
        const char *reason, gint flags)
{
    (void)self;
    if (flags & OPT_LONGF) {
        fprintf(stderr, "error: option `--%s` %s\n", opt->long_name, reason);
    } else {
        fprintf(stderr, "error: option `-%c` %s\n", opt->short_name, reason);
    }
    return FALSE;
}

static gboolean
opt_boolean (chassis_options_t *self, const chassis_option_t *opt, gint flags)
{
    if (flags & OPT_UNSET) {
        *(int *)opt->value = *(int *)opt->value - 1;
    } else {
        *(int *)opt->value = *(int *)opt->value + 1;
    }
    if (*(int *)opt->value < 0) {
        *(int *)opt->value = 0;
    }
    if (opt->callback) {
    	return opt->callback (self, opt);
    }
    return TRUE;
}

static gboolean
opt_string (chassis_options_t *self, const chassis_option_t *opt, gint flags)
{
    if (self->optvalue) {
        *(const char **)opt->value = self->optvalue;
        self->optvalue = NULL;
    } else if (self->argc > 1) {
        self->argc--; ++self->argv;
        *(const char **)opt->value = *self->argv;
    } else if (!opt->callback) {
        return argparse_error(self, opt, "requires a value", flags);
    }
    if (opt->callback) {
    	return opt->callback (self, opt);
    }
    return TRUE;
}

static gboolean
is_contained (char *arg)
{
    // this function checks if arg is a valid option
    for(; optHead != NULL; optHead = optHead->next) {
        chassis_option_t *option = optHead->data;
        if (option->long_name != NULL) { // a valid one then
            if (arg[0] == '-' && *arg+1 == option->short_name) { 
                return TRUE;
            }
            if (arg[0] == '-' && arg[1] == '-') {
                if (arg+2 == option->long_name) {
                    return TRUE;
                }else{
                    char *command = strtok(arg+2, "=");
                    if (arg+2 == option->long_name) {
                        return TRUE;
                    }
                  }
            }
        }
    }
    // arg is not an option, just a value
    return FALSE;
}

static int
count_elements_till_next_opt (char **argv, int limit)
{
	// this function iterates until next element that is a valid option is found
	// it is not enough to check that argv[i][0] == '-'
	assert (limit);
	int i = 0;
	while (i < limit && !is_contained (argv[i])) {
		i++;
	}
	return i;
}

static gboolean
opt_array (chassis_options_t *self, const chassis_option_t *opt, gint flags)
{
    if (self->optvalue) {
        int size = count_elements_till_next_opt (self->argv + 1, self->argc - 1);
        char **tmp = (char **) malloc ((size + 2) * sizeof (char *));
        tmp[0] = self->optvalue;
        int i;
        for (i = 1; i < size+1; ++i) {
                // now stuff the array
            self->argc--; ++self->argv; // shift to take track of position
            tmp[i] = (char *) *self->argv;
        }
        tmp[i] = NULL;
        *(char ***) opt->value = tmp;
        self->optvalue = NULL;
        return TRUE;
    } else if (self->argc > 1) {
        int size = count_elements_till_next_opt (self->argv + 1, self->argc - 1);
        char **tmp = (char **) malloc ((size + 1) * sizeof (char *));
        // allocate enough space
        int i;
        for (i = 0; i < size; ++i) {
        	// now stuff the array
        	self->argc--; ++self->argv; // shift to take track of position
            tmp[i] = (char *) *self->argv;
        }
        tmp[size] = NULL;
        *(char ***) opt->value = tmp;
    } else if (!opt->callback) {
        return argparse_error(self, opt, "requires a value", flags);
    }
    if (opt->callback) {
    	return opt->callback (self, opt);
    }
    return TRUE;
}

static gboolean
opt_double (chassis_options_t *self, const chassis_option_t *opt, gint flags)
{
    const char *s = NULL;
    if (self->optvalue) {
        *(double *)opt->value = strtod(self->optvalue, (char **)&s);
        self->optvalue = NULL;
    } else if (self->argc > 1) {
        self->argc--; ++self->argv;
        *(double *)opt->value = strtod(*self->argv, (char **)&s);
    } else if (!opt->callback) {
        // go to error if no callback defined,
        // if callback defined, handle error there
        return argparse_error(self, opt, "requires a value", flags);
    }
    if (s && s[0] != '\0'){ 
        return argparse_error(self, opt, "expects a numerical value", flags);
    }
    if (opt->callback) {
        return opt->callback (self, opt);
    }
    return TRUE;
}

static gboolean
opt_int (chassis_options_t *self, const chassis_option_t *opt, gint flags)
{
    const char *s = NULL;
    if (self->optvalue) {
        *(int *)opt->value = strtol(self->optvalue, (char **)&s, 0);
        self->optvalue = NULL;
    } else if (self->argc > 1) {
        self->argc--; ++self->argv;
        *(int *)opt->value = strtol(*self->argv, (char **)&s, 0);
    } else if (!opt->callback) {
    	// go to error if no callback defined,
    	// if callback defined, handle error there
        return argparse_error(self, opt, "requires a value", flags);
    }
    if (s && s[0] != '\0'){ 
        return argparse_error(self, opt, "expects a numerical value", flags);
	}
    if (opt->callback) {
    	return opt->callback (self, opt);
    }
	return TRUE;
}

static gboolean
opt_longint (chassis_options_t *self, const chassis_option_t *opt, gint flags)
{
    const char *s = NULL;
    if (self->optvalue) {
        *(long int *)opt->value = strtol(self->optvalue, (char **)&s, 0);
        self->optvalue = NULL;
    } else if (self->argc > 1) {
        self->argc--; ++self->argv;
        *(long int *)opt->value = strtol(*self->argv, (char **)&s, 0);
    } else if (!opt->callback) {
    	// go to error if no callback defined,
    	// if callback defined, handle error there
        return argparse_error(self, opt, "requires a value", flags);
    }
    if (s && s[0] != '\0'){ 
        return argparse_error(self, opt, "expects a numerical value", flags);
	}
    if (opt->callback) {
    	return opt->callback (self, opt);
    }
	return TRUE;
}

static gboolean
argparse_getvalue(chassis_options_t *self, chassis_option_t *opt, gint flags, gint origin)
{
    if (!opt->value) {
        if (opt->callback) {
        return opt->callback(self, opt);
        }
    }
    switch (opt->type) {

    	case CHASSIS_OPTIONS_BOOLEAN :
            if(ORIGIN & origin){
               opt->origin = CMD;
            } else{
               opt->origin = KEY_FILE;
            }
    	    return opt_boolean (self, opt, flags);

    	case CHASSIS_OPTIONS_STRING :
    	    if(ORIGIN & origin){
                opt->origin = CMD;
             } else{
                opt->origin = KEY_FILE;
             }
            return opt_string (self, opt, flags);

	    case CHASSIS_OPTIONS_ARRAY :
            if(ORIGIN & origin){
                opt->origin = CMD;
             } else{
                opt->origin = KEY_FILE;
             }
	       return opt_array (self, opt, flags);

    	case CHASSIS_OPTIONS_INTEGER:
            if(ORIGIN & origin){
                opt->origin = CMD;
             } else{
                opt->origin = KEY_FILE;
             }
            return opt_int (self, opt, flags);
 
        case CHASSIS_OPTIONS_DOUBLE:
            if(ORIGIN & origin){
                opt->origin = CMD;
             } else{
                opt->origin = KEY_FILE;
             }
            return opt_double (self, opt, flags);

    	case CHASSIS_OPTIONS_LONG:
    	    if(ORIGIN & origin){
                opt->origin = CMD;
             } else{
                opt->origin = KEY_FILE;
             }
            return opt_longint (self, opt, flags);

    	default:
        	assert(0);
    }
}


static int
argparse_short_opt(chassis_options_t *self, GList *options, gint origin)
{
    GList *list = NULL;
    for (list = options; list != NULL; list = list->next) {
        chassis_option_t *option = list->data;
        if (option->short_name == *self->optvalue) {
            self->optvalue = self->optvalue[1] ? self->optvalue + 1 : NULL;
            if(argparse_getvalue(self, option, 0, origin)) {
                return SUCESS;
            } else {
                return FAIL;
            }
        }
    }
    return UNKNOWN;
}

static int
argparse_long_opt(chassis_options_t *self, GList *options, gint origin)
{
    GList *list = NULL;
    for (list = options; list != NULL; list = list->next) {
        chassis_option_t *option = list->data;
        char *rest;
        int opt_flags = 0;
        if (!option->long_name)
            continue;

        rest = prefix_skip(self->argv[0] + 2, option->long_name);
        if (!rest) {
            // negation disabled?
            if (option->flags & OPT_NONEG) {
                continue;
            }
            // only OPT_BOOLEAN/OPT_BIT supports negation
            if (option->type != CHASSIS_OPTIONS_BOOLEAN) {
                continue;
            }

            if (prefix_cmp(self->argv[0] + 2, "no-")) {
                continue;
            }
            rest = prefix_skip(self->argv[0] + 2 + 3, option->long_name);
            if (!rest)
                continue;
            opt_flags |= OPT_UNSET;
        }
        if (*rest) {
            if (*rest != '=')
                continue;
            self->optvalue = rest + 1;
        }
        if(argparse_getvalue(self, option, opt_flags | OPT_LONGF, origin)) {
                return SUCESS;
            } else {
                return FAIL;
            }
    }
    return UNKNOWN;
}

/*void
chassis_options_init(chassis_options_t *self, char *programName, char *const *usages, gint flags)
{
    memset(self, 0, sizeof(*self));
    chassis_options_add(self,
         CHASSIS_OPTIONS_BOOLEAN, 'h', "help", NULL,"show this help message and exit", &argparse_help_cb, 0, 0);
    chassis_option_t option = {CHASSIS_OPTIONS_BOOLEAN, 'h', "help", NULL,
                          "show this help message and exit", &argparse_help_cb, 0, 0};

//    self->options = g_list_append(self->options, &option);
    self->programName = programName;
    self->usages = usages;
    self->flags = flags;
    optHead = self->options; // store a pointer to beginning of options
}
*/

gboolean
chassis_options_parse(chassis_options_t *self, gint argc, gchar **argv ,gint origin)
{
    self->argc = argc - 1;
    self->argv = argv + 1;
    self->out = argv;
    helpBool = FALSE;


    for (; self->argc; self->argc--, self->argv++) {
        char *arg = self->argv[0];
        if (arg[0] != '-' || !arg[1]) {
            if (self->flags & ARGPARSE_STOP_AT_NON_OPTION) {
                goto end;
            }
            // if it's not option or is a single char '-', copy verbatim
            self->out[self->cpidx] = self->argv[0];
			self->cpidx++;
            continue;
        }
        // short option
        if (arg[1] != '-') {
            self->optvalue = arg + 1;
            switch (argparse_short_opt(self, self->options, origin)) {
            	case FAIL:
            		goto callback_failed;
            	case SUCESS:
                	break;
            	case UNKNOWN:
                	goto unknown;
            }
            continue;
        }
        // if '--' presents
        if (!arg[2]) {
            self->argc--; self->argv++;
            break;
        }
        // long option
        switch (argparse_long_opt(self, self->options, origin)) {
            case FAIL:
            	goto callback_failed;
        	case SUCESS:
            	break;
        	case UNKNOWN:
            	goto unknown;
        }
        continue;
end:
    memmove(self->out + self->cpidx, self->argv, self->argc * sizeof(*self->out));
    self->out[self->cpidx + self->argc] = NULL;

    return FALSE;

unknown:
        fprintf (stderr, "error: unknown option `%s`\n", self->argv[0]);
        /*argparse_usage(self);*/
        fprintf (stderr, "try '%s --help' for more information\n", self->programName);
        return FALSE; // error in processing args

callback_failed:
        if (*self->usages) fprintf(stdout, "Usage: %s\n", *self->usages++);
        fprintf (stderr, "try '%s --help' for more information\n", self->programName);
        return FALSE; // error in processing args,used when callback fails, opt_x returns false
    }
    
    return TRUE;


}

static void
argparse_usage(chassis_options_t *self)
{
    if (self->usages) {
        fprintf(stdout, "Usage: %s\n", *self->usages++);
        while (*self->usages && **self->usages)
            fprintf(stdout, "   or: %s\n", *self->usages++);
    }
    else {
        fprintf(stdout, "Usage:\n");
    }

    fputc('\n', stdout);


    // figure out best width
    int usage_opts_width = 0;
    int len;
    GList *list = self->options;
    for (; list != NULL; list = list->next) {
        chassis_option_t *option = list->data;
        len = 0;
        if ((option)->short_name) {
            len += 2;
        }
        if ((option)->short_name && (option)->long_name) {
            len += 2;           // separator ", "
        }
        if ((option)->long_name) {
            len += strlen((option)->long_name) + 2;
        }
        if (option->type == CHASSIS_OPTIONS_INTEGER) {
            len += strlen("=<int>");
        } else if (option->type == CHASSIS_OPTIONS_LONG) {
            len += strlen("=<long>");
        } else if (option->type == CHASSIS_OPTIONS_STRING) {
            len += strlen("=<str>");
        } else if (option->type == CHASSIS_OPTIONS_DOUBLE) {
            len += strlen("=<double>");
        }
        len = ceil((float)len / 4) * 4;
        if (usage_opts_width < len) {
            usage_opts_width = len;
        }
    }
    usage_opts_width += 4;      // 4 spaces prefix

    GList *list2 = self->options;
    for (; list2 != NULL; list2 = list2->next) {
        chassis_option_t *option = list2->data;
        int pos = 0;
        int pad = 0;
        if (option->type == CHASSIS_OPTIONS_GROUP) {
            fputc('\n', stdout);
            fprintf(stdout, "%s", option->help);
            fputc('\n', stdout);
            continue;
        }
        pos = fprintf(stdout, "    ");
        if (option->short_name) {
            pos += fprintf(stdout, "-%c", option->short_name);
        }
        if (option->long_name && option->short_name) {
            pos += fprintf(stdout, ", ");
        }
        if (option->long_name) {
            pos += fprintf(stdout, "--%s", option->long_name);
        }
        if (option->type == CHASSIS_OPTIONS_INTEGER) {
            pos += fprintf(stdout, "=<int>");
        } else if (option->type == CHASSIS_OPTIONS_LONG) {
            pos += fprintf(stdout, "=<long>");
        } else if (option->type == CHASSIS_OPTIONS_STRING) {
            pos += fprintf(stdout, "=<str>");
        }else if (option->type == CHASSIS_OPTIONS_DOUBLE) {
            pos += fprintf(stdout, "=<double>");
        }
        if (pos <= usage_opts_width) {
            pad = usage_opts_width - pos;
        } else {
            fputc('\n', stdout);
            pad = usage_opts_width;
        }
        fprintf(stdout, "%*s%s\n", pad + 2, "", option->help);
    }
}

static gboolean
argparse_help_cb(struct chassis_options_t *self, const struct chassis_option_t *option)
{
    helpBool = TRUE;
    argparse_usage(self);
    return FALSE;
}

static gboolean
argparse_requested_help ()
{
	return helpBool;
}

/**
 * create a command-line option
 */
chassis_option_t *chassis_option_new() {
    chassis_option_t *opt;

    opt = g_slice_new0(chassis_option_t);

    return opt;
}

/**
 * free the option
 */
void chassis_option_free(chassis_option_t *opt) {
    if (!opt) return;

    g_slice_free(chassis_option_t, opt);
}

/**
 * create command-line options
 */
chassis_options_t *chassis_options_new() {
    chassis_options_t *opt;

    opt = g_slice_new0(chassis_options_t);

    chassis_options_add(opt,
        CHASSIS_OPTIONS_BOOLEAN, 'h', "help", NULL,
        "show this help message and exit", &argparse_help_cb);

    opt->programName = NULL;
    opt->usages = NULL;
    opt->flags = 0;
    /* store a pointer to beginning of options */
    optHead = opt->options;

    return opt;
}

/**
 * add a option
 */
int chassis_options_add_option(chassis_options_t *opts,
        chassis_option_t *opt) {

    opts->options = g_list_append(opts->options, opt);

    return 0;
}

static int
chassis_option_set(chassis_option_t *opt,
        gint type,
        gchar short_name,
        const char *long_name,
        gpointer value,
        const char *help,
        argparse_callback *callback) {
    opt->type            = type;
    opt->long_name       = long_name;
    opt->short_name      = short_name;
    opt->value           = value;
    opt->help            = help;
    opt->callback        = callback;
    opt->flags           = 0;
    opt->origin          = 0;

    return 0;
}

int chassis_options_add(chassis_options_t *opts,
        gint type,
        gchar short_name,
        const char *long_name,
        gpointer value,
        const char *help,
        argparse_callback *callback) {
    chassis_option_t *opt;

    opt = chassis_option_new();
    if (0 != chassis_option_set(opt,
                type,
                short_name,
                long_name,
                value,
                help,
                callback) ||
            0 != chassis_options_add_option(opts, opt)) {
        chassis_option_free(opt);
        return -1;
    } else {
        return 0;
    }
}
