#ifndef CLI_H
#define CLI_H

#define _GNU_SOURCE 1  // Enable POSIX 2008.1 extensions

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

#endif
