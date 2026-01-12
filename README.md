# clic

clic is a **c**ommand **l**ine **i**nterface application framework for **C**. It's purpose is to make the process of building command-line applications in C easier and more efficient. It provides the framework for parsing arguments and dispatching the appropriate actions. You just provide the blueprint.

## Usage

**clic** is header-only. Simply download and include [cli.h](cli.h) in your application. You'll also need to define `CLI_IMPLEMENTATION` in at least *one* C file before including `cli.h`.

```c
#define CLI_IMPLEMENTATION
#include <cli.h>
```

> [!NOTE]
> It's actually recommended to add **clic** as a git submodule so you can pull updates directly in your project without having to replace files or update build system configurations.

### Basic example

This example shows a basic CLI application with no commands and no options.

```c
#define CLI_IMPLEMENTATION
#include <cli.h>

static int do_thing(cli_option** opts, u32 opt_count) {
    printf("Doing thing\n");
    return 0; // success
}

int main(int argc, char* argv[]) {
    cli_app* app = cli_app_create("do-thing", "1.0.0", "Does a thing", &do_thing);

    int result = cli_app_run(app, argc, argv);
    cli_app_destroy(app);

    return result;
}
```

### Adding options

You can add global options to your application like so:

```c
#define CLI_IMPLEMENTATION
#include <cli.h>

static int do_thing_with_opts(cli_option** opts, u32 opt_count) {
    if (opt_count < 1) return 1;

    const char* opt1 = NULL;
    bool opt2 = false;

    for (u32 i = 0; i < opt_count; i++) {
        if (!opts[i]->is_present) continue;
        if (strstr(opts[i]->names, "--opt1")) opt1 = opts[i]->value;
        if (strstr(opts[i]->names, "--opt2")) opt2 = (strcmp(opts[i]->value, "true") == 0) ? true : false;
    }

    // do something with opts

    return 0; // success
}

int main(int argc, char* argv[]) {
    cli_app* app = cli_app_create("do-thing", "1.0.0", "Does a thing", &do_thing);

    // (app, names, required, is_flag, help_text)
    cli_app_add_option(app, "-o1, --opt1", true,     false, "The first option");
    cli_app_add_option(app, "-o2, --opt2", false,    true,  "The second option");

    int result = cli_app_run(app, argc, argv);
    cli_app_destroy(app);

    return result;
}
```

### Adding commands

```c
#define CLI_IMPLEMENTATION
#include <cli.h>

static int cmd1_action(cli_option** opts, u32 opt_count) {}

int main(int argc, char* argv[]) {
    // Pass NULL to default_action since we have commands
    // You can still pass a default action if you have one
    cli_app* app = cli_app_create("do-thing", "1.0.0", "Does a thing", NULL);

    // (app, names, action, help_text)
    cli_command* cmd1 = cli_app_add_command(app, "cmd1, c1", cmd1_action, "Performs some action");

    // You can add options to commands as well
    // (app, cmd, names, required, is_flag, help_text)
    cli_cmd_add_option(app, cmd1, "-o1, --opt1", true, false, "The first option");

    int result = cli_app_run(app, argc, argv);
    cli_app_destroy(app);

    return result;
}
```