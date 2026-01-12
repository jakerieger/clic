#include "../src/cli.h"
#include <time.h>
#include <ctype.h>

static i32 cmd_info(cli_option** opts, u32 opt_count) {
    const char* path = NULL;
    bool verbose     = false;

    for (u32 i = 0; i < opt_count; i++) {
        if (!opts[i]->is_present)
            continue;
        if (strstr(opts[i]->names, "--path"))
            path = opts[i]->value;
        if (strstr(opts[i]->names, "--verbose"))
            verbose = true;
    }

    struct stat st;
    if (stat(path, &st) != 0) {
        perror(path);
        return 1;
    }

    const char* type = S_ISDIR(st.st_mode)   ? "directory"
                       : S_ISREG(st.st_mode) ? "file"
                       : S_ISLNK(st.st_mode) ? "symlink"
                                             : "other";

    printf("%s\n", path);
    printf("  Type: %s\n", type);
    printf("  Size: %ld bytes\n", (long)st.st_size);

    if (verbose) {
        printf("  Mode: %o\n", st.st_mode & 0777);
        printf("  Links: %ld\n", (long)st.st_nlink);
        printf("  Inode: %ld\n", (long)st.st_ino);

        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
        printf("  Modified: %s\n", timebuf);
    }

    return 0;
}

static i32 cmd_count(cli_option** opts, u32 opt_count) {
    const char* path = NULL;
    bool lines = false, words = false, chars = false;

    for (u32 i = 0; i < opt_count; i++) {
        if (!opts[i]->is_present)
            continue;
        if (strstr(opts[i]->names, "--path"))
            path = opts[i]->value;
        if (strstr(opts[i]->names, "--lines"))
            lines = true;
        if (strstr(opts[i]->names, "--words"))
            words = true;
        if (strstr(opts[i]->names, "--chars"))
            chars = true;
    }

    // Default to all if none specified
    if (!lines && !words && !chars) {
        lines = words = chars = true;
    }

    FILE* f = fopen(path, "r");
    if (!f) {
        perror(path);
        return 1;
    }

    u64 line_count = 0, word_count = 0, char_count = 0;
    int c, prev = ' ';

    while ((c = fgetc(f)) != EOF) {
        char_count++;
        if (c == '\n')
            line_count++;
        if (!isspace(c) && isspace(prev))
            word_count++;
        prev = c;
    }
    fclose(f);

    if (lines)
        printf("  Lines: %lu\n", line_count);
    if (words)
        printf("  Words: %lu\n", word_count);
    if (chars)
        printf("  Chars: %lu\n", char_count);

    return 0;
}

int main(int argc, char* argv[]) {
    cli_app* app = cli_app_create("filetool", "1.0.0", "A file utility showcasing the clic library", NULL);

    // `info` command
    cli_command* info = cli_app_add_command(app, "info, i", cmd_info, "Display information about a file or directory");
    cli_cmd_add_option(app, info, "-p, --path", true, false, "Path to inspect");
    cli_cmd_add_option(app, info, "-v, --verbose", false, true, "Show extended information");

    // `count` command
    cli_command* count = cli_app_add_command(app, "count, c", cmd_count, "Count lines, words, and characters");
    cli_cmd_add_option(app, count, "-p, --path", true, false, "File to analyze");
    cli_cmd_add_option(app, count, "-l, --lines", false, true, "Count lines only");
    cli_cmd_add_option(app, count, "-w, --words", false, true, "Count words only");
    cli_cmd_add_option(app, count, "-c, --chars", false, true, "Count characters only");

    int result = cli_app_run(app, argc, argv);
    cli_app_destroy(app);

    return result;
}