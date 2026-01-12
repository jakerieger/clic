/*
    cli.h - v0.1.0 - Command-Line interface application framework for C

    Do this:
        #define CLI_IMPLEMENTATION
    before you include this file in *one* C or C++ file to create the implementation.

    LICENSE

        ISC License

        Copyright (c) 2026 Jake Rieger

        Permission to use, copy, modify, and/or distribute this software for any
        purpose with or without fee is hereby granted, provided that the above
        copyright notice and this permission notice appear in all copies.

        THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
        WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
        MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
        ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
        WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
        ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
        OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

    REVISION HISTORY

        0.1.0  (2026-01-12) initial release of clic
*/

#ifndef CLI_H
#define CLI_H

#define CLI_VERSION_MAJOR 0
#define CLI_VERSION_MINOR 1
#define CLI_VERSION_PATCH 0
#define CLI_VERSION_STRING "0.1.0"

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdnoreturn.h>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
#if defined(__GNUC__) || defined(__clang__)
typedef __int128_t i128;
typedef __uint128_t u128;
#endif
typedef float f32;
typedef double f64;
typedef u8 byte;

noreturn inline static void cli_panic(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "PANIC: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
}

typedef struct cli_option cli_option;
typedef struct cli_command cli_command;
typedef struct cli_app cli_app;
typedef i32 (*cli_action)(cli_option** opts, u32 opt_count);

struct cli_option {
    const char* names;  // Comma-separated names, e.g. "-f,--file"
    const char* help_text;
    cli_command* command;  // Associated command. NULL if global option.
    bool required;
    bool is_flag;
    bool is_present;
    char value[256];  // Parsed value
};

struct cli_command {
    const char* names;
    cli_action action;
    const char* help_text;
};

struct cli_app {
    const char* name;
    const char* version;
    const char* description;

    cli_command* commands;
    u32 command_count;

    cli_option* options;
    u32 option_count;

    cli_action* default_action;
};

cli_app* cli_app_create(const char* name, const char* version, const char* description, cli_action* default_action);
void cli_app_destroy(cli_app* app);
void cli_app_print_info(const cli_app* app);

cli_command* cli_app_add_command(cli_app* app, const char* names, cli_action action, const char* help_text);
void cli_app_add_option(cli_app* app, const char* names, bool required, bool is_flag, const char* help_text);
void cli_cmd_add_option(
  cli_app* app, cli_command* cmd, const char* names, bool required, bool is_flag, const char* help_text);

int cli_app_run(cli_app* app, const i32 argc, char** argv);

static inline cli_option* cli_get_option(cli_option* opts, u32 count, const char* name) {
    for (u32 i = 0; i < count; i++) {
        // TODO: Simple check that should get refactored. This should split and check each name.
        if (strstr(opts[i].names, name) != NULL && opts[i].is_present) {
            return &opts[i];
        }
    }

    return NULL;
}

#endif  // End of header

#ifdef CLI_IMPLEMENTATION

/*********************************************************************/
/* Memory management                                                 */
/*********************************************************************/

#define ARENA_BASE (sizeof(cli_arena))
#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define KB(n) ((size_t)(n) << 10)
#define MB(n) ((size_t)(n) << 20)
#define PAGESIZE (sizeof(void*))
#define MIN(a, b)                                                                                                      \
    ({                                                                                                                 \
        __typeof__(a) _a = (a);                                                                                        \
        __typeof__(b) _b = (b);                                                                                        \
        _a < _b ? _a : _b;                                                                                             \
    })
#define MAX(a, b)                                                                                                      \
    ({                                                                                                                 \
        __typeof__(a) _a = (a);                                                                                        \
        __typeof__(b) _b = (b);                                                                                        \
        _a > _b ? _a : _b;                                                                                             \
    })

typedef struct {
    size_t capacity;
    size_t position;
} cli_arena;

cli_arena* g_arena;

// Define constant memory sizes for arrays
#define MAX_COMMAND_COUNT 64
#define COMMANDS_SIZE (sizeof(cli_command) * MAX_COMMAND_COUNT)

#define MAX_OPTION_COUNT 64
#define OPTIONS_SIZE (sizeof(cli_option) * MAX_OPTION_COUNT)

static cli_arena* arena_create(size_t capacity) {
    cli_arena* out = (cli_arena*)malloc(capacity);
    if (!out) {
        cli_panic("error: failed to allocate memory for arena");
    }

    out->position = ARENA_BASE;
    out->capacity = capacity;

    return out;
}

static void arena_destroy(cli_arena* arena) {
    free(arena);
}

static void* arena_push(cli_arena* arena, size_t size, bool non_zero) {
    size_t position_aligned = ALIGN_UP(arena->position, PAGESIZE);
    size_t new_position     = position_aligned + size;

    if (new_position > arena->capacity) {
        cli_panic("error: arena memory full (%lu > %lu)", new_position, arena->capacity);
    }

    arena->position = new_position;
    u8* out         = (u8*)arena + position_aligned;
    memset(out, 0, size);

    return out;
}

static void arena_pop(cli_arena* arena, size_t size) {
    size = MIN(size, arena->position - ARENA_BASE);
    arena->position -= size;
}

static void arena_pop_to(cli_arena* arena, size_t position) {
    size_t size = position < arena->position ? arena->position - 1 : 0;
    arena_pop(arena, size);
}

static void arena_clear(cli_arena* arena) {
    arena_pop_to(arena, ARENA_BASE);
}

#define ARENA_ALLOC(type) (type*)arena_push(g_arena, sizeof(type), false)
#define ARENA_ALLOC_ARRAY(type, count) (type*)arena_push(g_arena, sizeof(type) * (count), false);
#define MINIMUM_CAPACITY (COMMANDS_SIZE + OPTIONS_SIZE + ARENA_BASE + sizeof(cli_app))

/*********************************************************************/
/* Internal helpers                                                  */
/*********************************************************************/

#define STREQ(s1, s2) strcmp(s1, s2) == 0

static void free_string_array(char** arr, size_t count) {
    if (arr == NULL)
        return;
    for (size_t i = 0; i < count; i++) {
        free(arr[i]);
    }
    free(arr);
}

static char** split_names(const char* names, size_t* name_count) {
    char** buffer = NULL;
    *name_count   = 0;

    char* str_cpy = strdup(names);
    if (str_cpy == NULL)
        return NULL;

    char* token = strtok(str_cpy, ",");
    while (token != NULL) {
        // skip leading whitespace (for "name, -n" style)
        while (*token == ' ')
            token++;

        char** new_buffer = realloc(buffer, sizeof(char*) * (*name_count + 1));
        if (new_buffer == NULL) {
            free_string_array(buffer, *name_count);
            free(str_cpy);
            return NULL;
        }
        buffer              = new_buffer;
        buffer[*name_count] = strdup(token);
        if (buffer[*name_count] == NULL) {
            free_string_array(buffer, *name_count);
            free(str_cpy);
            return NULL;
        }
        (*name_count)++;
        token = strtok(NULL, ",");
    }

    free(str_cpy);
    return buffer;
}

static bool option_matches_name(const cli_option* opt, const char* name) {
    size_t name_count;
    char** names = split_names(opt->names, &name_count);
    if (names == NULL)
        return false;

    bool found = false;
    for (size_t i = 0; i < name_count; i++) {
        if (strcmp(names[i], name) == 0) {
            found = true;
            break;
        }
    }

    free_string_array(names, name_count);
    return found;
}

static cli_command* find_command(cli_app* app, const char* name) {
    for (u32 i = 0; i < app->command_count; i++) {
        cli_command* cmd = &app->commands[i];
        size_t name_count;
        char** names = split_names(cmd->names, &name_count);
        if (names == NULL)
            continue;

        bool found = false;
        for (size_t k = 0; k < name_count; k++) {
            if (strcmp(names[k], name) == 0) {
                found = true;
                break;
            }
        }

        free_string_array(names, name_count);
        if (found)
            return cmd;
    }

    return NULL;
}

// Find an option by argument name (e.g., "-f" or "--file")
static cli_option* find_option(cli_app* app, const char* arg) {
    for (u32 i = 0; i < app->option_count; i++) {
        if (option_matches_name(&app->options[i], arg)) {
            return &app->options[i];
        }
    }

    return NULL;
}

static cli_option* find_option_from_list(cli_option** opts, u32 opt_count, const char* arg) {
    assert(opts);
    assert(opt_count > 0);
    for (u32 i = 0; i < opt_count; i++) {
        if (option_matches_name(opts[i], arg)) {
            return opts[i];
        }
    }

    return NULL;
}

// Check if argument looks like an ooption (starts with -)
static bool is_option_arg(const char* arg) {
    return arg != NULL && arg[0] == '-';
}

cli_option** get_command_options(const cli_app* app, const cli_command* cmd, u32* opt_count) {
    cli_option** opts = NULL;
    *opt_count        = 0;
    for (u32 i = 0; i < app->option_count; i++) {
        cli_option* opt = &app->options[i];
        if (opt->command == cmd) {
            cli_option** opts_new = realloc(opts, sizeof(cli_option*) * (*opt_count + 1));
            if (!opts_new) {
                free(opts);
                return NULL;
            }
            opts             = opts_new;
            opts[*opt_count] = opt;
            if (opts[*opt_count] == NULL) {
                free(opts);
                return NULL;
            }
            (*opt_count)++;
        }
    }
    return opts;
}

/*********************************************************************/
/* Help text generation                                              */
/*********************************************************************/

static size_t calc_longest_cmd(const cli_command* cmds, size_t count) {
    size_t longest = 0;
    for (size_t i = 0; i < count; i++) {
        size_t len = strlen(cmds[i].names);
        if (len > longest)
            longest = len;
    }

    return longest;
}

static size_t calc_longest_opt(cli_option** opts, size_t count) {
    size_t longest = 0;
    for (size_t i = 0; i < count; i++) {
        size_t len = strlen(opts[i]->names);
        if (len > longest)
            longest = len;
    }
    return longest;
}

static void print_app_help(const cli_app* app) {
    printf("%s - %s\n%s\n\n", app->name, app->version, app->description);

    printf("USAGE\n");
    printf("  %s <command> [options]\n\n", app->name);

    if (app->command_count > 0) {
        printf("COMMANDS\n");
        size_t longest = calc_longest_cmd(app->commands, app->command_count);

        for (u32 i = 0; i < app->command_count; i++) {
            const cli_command* cmd = &app->commands[i];
            printf("  %-*s    %s\n", (int)longest, cmd->names, cmd->help_text);
        }
        printf("\n");
    }

    printf("Run '%s <command> --help' for more information on a command.\n", app->name);
}

static void print_command_help(const cli_app* app, const cli_command* cmd, cli_option** opts, u32* opt_count) {
    printf("%s %s - %s\n\n", app->name, cmd->names, cmd->help_text);

    u32 opt_count_;
    cli_option** opts_ = NULL;

    if (opts == NULL) {
        opts_ = get_command_options(app, cmd, &opt_count_);
    } else {
        assert(opt_count_);
        opts_      = opts;
        opt_count_ = *opt_count;
    }

    if (opt_count_ > 0) {
        printf("OPTIONS\n");
        size_t longest = calc_longest_opt(opts_, opt_count_);

        for (u32 i = 0; i < opt_count_; i++) {
            const cli_option* opt = opts_[i];
            const char* req       = opt->required ? " (required)" : "";
            const char* type      = opt->is_flag ? "<flag> " : "<value>";
            printf("  %-*s    %s    %s%s\n", (int)longest, opt->names, type, opt->help_text ? opt->help_text : "", req);
        }

        printf("\n");
    }

    if (opts == NULL)
        free(opts_);
}

/*********************************************************************/
/* Public API                                                        */
/*********************************************************************/

cli_app* cli_app_create(const char* name, const char* version, const char* description, cli_action* default_action) {
    // Initialize the arena allocator
    g_arena = arena_create(MINIMUM_CAPACITY);
    if (!g_arena)
        cli_panic("fatal: failed to initialize memory arena");
    assert(g_arena->capacity == MINIMUM_CAPACITY);

    cli_app* app = ARENA_ALLOC(cli_app);
    if (app == NULL)
        return NULL;

    app->name        = name;
    app->version     = version;
    app->description = description;

    app->commands      = ARENA_ALLOC_ARRAY(cli_command, MAX_COMMAND_COUNT);
    app->command_count = 0;

    app->options      = ARENA_ALLOC_ARRAY(cli_option, MAX_OPTION_COUNT);
    app->option_count = 0;

    app->default_action = default_action;

    return app;
}

void cli_app_destroy(cli_app* app) {
    arena_clear(g_arena);
    arena_destroy(g_arena);
}

void cli_app_print_info(const cli_app* app) {
    printf("Name: %s\nVersion: %s\nDescription: %s\n", app->name, app->version, app->description);
}

cli_command* cli_app_add_command(cli_app* app, const char* names, cli_action action, const char* help_text) {
    if (app->command_count + 1 > MAX_COMMAND_COUNT) {
        cli_panic("error: maximum command count reached (%d)", MAX_COMMAND_COUNT);
    }

    cli_command* cmd = &app->commands[app->command_count++];
    memset(cmd, 0, sizeof(*cmd));
    cmd->names     = names;
    cmd->action    = action;
    cmd->help_text = help_text;

    return cmd;
}

void cli_app_add_option(cli_app* app, const char* names, bool required, bool is_flag, const char* help_text) {
    // Global options just have NULL commands (uses app's default_action)
    cli_cmd_add_option(app, NULL, names, required, is_flag, help_text);
}

void cli_cmd_add_option(
  cli_app* app, cli_command* cmd, const char* names, bool required, bool is_flag, const char* help_text) {
    if (app->option_count + 1 > MAX_OPTION_COUNT) {
        cli_panic("error: maximum option count reached (%d)", MAX_OPTION_COUNT);
    }

    cli_option* opt = &app->options[app->option_count++];
    memset(opt, 0, sizeof(*opt));
    opt->command   = cmd;
    opt->help_text = help_text;
    opt->is_flag   = is_flag;
    opt->required  = required;
    opt->names     = names;
}

/*********************************************************************/
/* Argument Parsing                                                  */
/*********************************************************************/

static int parse_command_args(cli_app* app, cli_command* cmd, int argc, char* argv[], int start_index) {
    u32 opt_count;
    cli_option** opts = get_command_options(app, cmd, &opt_count);

    // Reset all options to defaults
    for (u32 i = 0; i < opt_count; i++) {
        opts[i]->is_present = false;
        memset(opts[i]->value, 0, sizeof(opts[i]->value));
    }

    int idx = start_index;
    while (idx < argc) {
        const char* arg = argv[idx];

        if (STREQ(arg, "--help") || STREQ(arg, "-h")) {
            print_command_help(app, cmd, opts, &opt_count);
            return 0;
        }

        if (!is_option_arg(arg)) {
            fprintf(stderr, "error: unexpected argument '%s'\n", arg);
            return 1;
        }

        cli_option* opt = find_option_from_list(opts, opt_count, arg);
        if (opt == NULL) {
            fprintf(stderr, "error: unknown option '%s'\n", arg);
            return 1;
        }

        opt->is_present = true;

        if (opt->is_flag) {
            strcpy(opt->value, "true");
        } else {
            idx++;
            if (idx >= argc) {
                fprintf(stderr, "error: option '%s' requires a value\n", arg);
                return 1;
            }

            const char* value = argv[idx];
            if (is_option_arg(value)) {
                fprintf(stderr, "error: option '%s' requires a value, got '%s'\n", arg, value);
                return 1;
            }

            size_t len = strlen(value);
            if (len >= sizeof(opt->value)) {
                fprintf(stderr, "error: value for '%s' is too long (max %zu)\n", arg, sizeof(opt->value) - 1);
                return 1;
            }

            memcpy(opt->value, value, len + 1);
        }

        idx++;
    }

    for (u32 i = 0; i < opt_count; i++) {
        if (opts[i]->required && !opts[i]->is_present) {
            fprintf(stderr, "error: required option '%s' not provided\n", opts[i]->names);
            return 1;
        }
    }

    return cmd->action(opts, opt_count);
}

int cli_app_run(cli_app* app, const i32 argc, char** argv) {
    if (argc < 2) {
        if (app->command_count == 0) {
            if (app->default_action != NULL) {
                return (*(app->default_action))(NULL, 0);
            }
        }
        print_app_help(app);
        return 1;
    }

    const char* first_arg = argv[1];

    if (STREQ(first_arg, "--help") || STREQ(first_arg, "-h")) {
        print_app_help(app);
        return 0;
    }

    else if (STREQ(first_arg, "--version") || STREQ(first_arg, "-v")) {
        printf("%s %s\n", app->name, app->version);
    }

    cli_command* cmd = find_command(app, first_arg);
    if (cmd == NULL) {
        if (app->default_action != NULL) {
            // Parse options for default action
            u32 opt_count;
            cli_option** opts = get_command_options(app, NULL, &opt_count);
            cli_action def    = *(app->default_action);
            return def(opts, opt_count);
        }

        fprintf(stderr, "error: unknown command '%s'\n", first_arg);
        print_app_help(app);
        return 1;
    }

    return parse_command_args(app, cmd, argc, argv, 2);
}

#endif  // End of implementation