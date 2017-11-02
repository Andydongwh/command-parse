#ifndef COMMAND_PARSE_H
#define COMMAND_PARSE_H

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

struct chassis_options_t;
struct chassis_option_t;
typedef gboolean argparse_callback (struct chassis_options_t *self, const struct chassis_option_t *opt);

enum argparse_flag {
    ARGPARSE_STOP_AT_NON_OPTION = 1,
};

enum argparse_option_type {

    CHASSIS_OPTIONS_GROUP,
    CHASSIS_OPTIONS_BOOLEAN,
    CHASSIS_OPTIONS_DOUBLE,
    CHASSIS_OPTIONS_INTEGER,
    CHASSIS_OPTIONS_LONG,
    CHASSIS_OPTIONS_STRING,
    CHASSIS_OPTIONS_ARRAY,
};

enum options_origin {
    CMD       = 1,
    KEY_FILE  = 2,
};

enum argparse_option_flags {
    OPT_NONEG = 1,              /* disable negation */
};

/**
 *  chassis_options_t option
 *
 *  `type`:
 *    holds the type of the option, you must have an CHASSIS_OPTIONS_END last in your
 *    array.
 *
 *  `short_name`:
 *    the character to use as a short option name, '\0' if none.
 *
 *  `long_name`:
 *    the long option name, without the leading dash, NULL if none.
 *
 *  `value`:
 *    stores pointer to the value to be filled.
 *
 *  `help`:
 *    the short help message associated to what the option does.
 *    Must never be NULL (except for CHASSIS_OPTIONS_END).
 *
 *  `callback`:
 *    function is called when corresponding argument is parsed.
 *
 *  `flags`:
 *    option flags.
 */
typedef struct chassis_option_t {
    gint type;
    gchar short_name;
    const char *long_name;
    gpointer value;
    const char *help;
    argparse_callback *callback;
    gint flags;
    gint origin;
} chassis_option_t;


typedef struct chassis_options_t {
    GList *options;
    char *const *usages;
    char *programName;
    gint flags;
    // internal context
    gint argc;
    gchar **argv;
    gchar **out;
    gint cpidx;
    gchar *optvalue;       // current option value
} chassis_options_t;

int
chassis_options_add(chassis_options_t *opts, 
        gint type,
        gchar short_name,
        const char *long_name,
        gpointer values,
        const char *help,
        argparse_callback *callback);

gboolean
chassis_options_parse(chassis_options_t *self, gint argc, gchar **argv, gint origin);

chassis_options_t *chassis_options_new(void);
#endif
