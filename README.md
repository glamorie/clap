# clap
C library for parsing command line arguments.

## Features

> - Positional arguments
> - Multi-value arguments
> - Global options
> - Help message generation.
> - Multiple commands
> - Command Groups

## Initializing the application.
Applications must be initialized and configured before running to
set up the application metadata.
This includes 
> - App name
> - App version
> - App description
> - App usage

The library can also add default implementations of the `--help` and
`--version` flags if specified when initializing

```c
#include "clap/clap.h"

int main(int argc, const char* argv[]){
    initApp(
        "Cli App", // Name
        "1.0.0", // Version
        "Command line application using clap.", // Description
        (Slots){0, 0, 0}, // zero commands, zero command groups, zero switch.  
        true, // Add the default help switch.
        true, // Add the default version switch
        (const char*[]){
            "--name <STRING> --age <NUMBER> --gender <STRING>", // Usage
            NULL // Null terminated
        }
    );
}
```

## Configure a the default command.
This is the command executed when a specific one is not supplied.
Slots must be allocated for the positional arguments, named 
arguments and options and the callback.

```c
#include <stdio.h>
#include "clap/clap.h"

int cDefaultCommand(void* result[], void* appNamespace){
    char* name = result[0];
    char* age = result[1];
    char* gender = result[2];
    printf("Name: %s\nAge: %s\nGender: %s\n", name, gender, result);
    return 0;
}

int main(int argc, const char* argv[]){
    // Initialize
    addDefaultCommand(
        (Slots){0, 3, 0}, // zero positional arguments, 3 named arguments, zero options.
        cDefaultCommand // Callback
    )
}
```

## Other app commands
Unlike the default command, these must have a unique name and must be specified
when running the program.

```bash
program command-name --arguments values ...
```

They also have to be accounted for in the slots when initializing the app. 
The pointer to the command should be stored in a variable since it's required
to set up the arguments and options the command contains.

```c
#include <stdio.h>

int cInstall(void* result[], void* appNamespace){
    char** plugins = result[0];
    unsigned int i = 0;
    char* plugin;
    while (plugin = plugins[i]){
        // Do something
    }
    return 0;
}

int main(int argc, const char* argv[]){
    // Initialize

    Command* install = addCommand(
        "install", // command name
        'i', // command shorthand 
        "Install plugins for the program.", // command's description.
        (Slots){1, 0, 0}, // One positional argument
        cInstall, // Callback
        (const char*[]){
            "[...PLUGINS]",
            NULL,
        } // Usage
    );
}

```

## Command Groups.
These contain a collection of commands, that deal with a related tasks, for 
example, extension management.

```bash
program group command-in-group --argument value
```

Slots must be specified for commands and sub command groups that the group will
contain.

```c
#include "clap/clap.c"

int cInstallPlugins(void* result[], void* _);
int cUpdatePlugins(void* result[], void* _);
int cUninstallPlugins(void* result[], void* _);

int main(int argc, const char* argv[]){
    // Initialize 

    CommandGroup* plugins = addGroup( // Add a command group to the app
        "plugins", // name
        0, // no shorthand 
        "Manage plugins", // description
        (Slots){3, 0} // 3 commands, zero sub groups.
    );

    Command* pInstall = addCommandG(
        plugins, // Command group that owns the command.
        "install",
        'i',
        "Install specified plugin(s)",
        (Slots){1, 0, 0}, // 1 positional 
        cInstall, // callback
        NULL // usage 
    );
    Command* pUpdate = addCommandG(
        plugins,.
        "update",
        'u',
        "Update specified plugin(s)",
        (Slots){1, 0, 0},
        cInstall,
        NULL 
    );
    Command* pRemove = addCommandG(
        plugins,
        "remove",
        'r',
        "Update specified plugin(s)",
        (Slots){1, 0, 0}, 
        cInstall,
        NULL 
    );
}
```

You can also add a command group to another command group using the
`addGroupG` command, with the first argument being the pointer to
the group that owns the command group.

## Switches
In clap, switches are global options. They are not bound to a specific
command context. When supplied, they call a callback taking in the app
namespace as an argument. `Final` switches are switches whose 
callback execute and cause parser exit. An example of a `Final` switch is 
the `--help` option. It displays the help message and exits. If an error, 
symbolized by a non-zero exit value from the callback, occurs, the parser 
is stopped. Switches do not take in any value.

```c 
#include <stdbool.h> 
#include "clap/clap.h"

typedef struct{
    bool colored;
    bool summarize;
}Settings;

int sCompact(void* appNamespace){
    Settings* options = appNamespace;
    options->colored = false;
    options->summarized = true;
    return 0; 
}

int main(int argc, const char* argv[]){
    Settings settings = {true, false};
    // Initialize the app
    addSwitch(
        "compact", // long flag
        'c', // shorthand
        "Removes text decoration and summarizes logs.", // description 
        sCompact // switch callback
    );
}

```

## Command Arguments.
Commands can contain arguments that collect values from the user, or not. In the command callback, the index of the values collected for the argument are 
determine by the order in which the arguments were added to the command.
There are three types of arguments.

- ### Options
These are arguments that toggle store a boolean value. When passed to the
program, they set the boolean to true.

```c
#include "clap/clap.c"

int main(int argc, const char* argv[]){
    addOption( // Add to the default command
        "recursive", // long flag
        'r', // shorthand
        "Remove the directory and its sub-directories recursively." // description
    )

    addOptionC(
        someCommand, // Add to the default command
        "recursive", // long flag
        'r', // shorthand
        "Remove the directory and its sub-directories recursively." // description
    )
}
```
In the callback the value is retrieved as follows.

```c

int cCommand(void* result[], void* _){
    bool recursive = result[0];
    if (recursive) {
        puts("Recursively deleting the directory");
    };
};
```

- ### Named arguments
These are arguments that require a flag to collect the values.
They can take in more than one value. The number of arguments is specifed 
in the `nargs` parameter where:

> **NOTE:** Zero means that the number of arguments is not fixed.

The arguments can also be optional.
Metadata can be added to the argument to hint out the kind of values the 
argument expects.

```c
#include "clap/clap.c"

int main(int argc, const char* argv[]){
    addArgument( // Add to the default command
        "name", // long flag
        'n', // shorthand
        "Your'e name.", // description
        "STRING", // metadata,
        1, // nargs
        false // whether optional.
    )
    addArgumentC(
        someCommand,  // Add to the default command
        "name", // long flag
        'n', // shorthand
        "Your'e name.", // description
        "STRING", // metadata,
        1, // nargs
        false // whether optional.
    )
}
```
For arguments with one value, the value can just be retrieve using its index, 
which again, is determined by the order in which the arguments and options are added to the command.

```c
int cCommand(void* result[], void* _){
    char* name = result[0];
    printf("Name: %s\n", name);
};
```

For arguments with a specific amount of arguments `N`, the values are returned as a string array of length `N`.

```c
int cCommand(void* result[], void* _){
    char** values = result[0];
    unsigned int length = 4;
    for (unsigned int i = 0; i < length; i++){
        printf("%s\n", values[i]);
    }
};
```

If the values the argument expects has no fixed length, then its returned as
a null terminated string array.

```c
int cCommand(void* result[], void* _){
    char** files = result[0];
    for (unsigned int i = 0; !values[i] ; i++){
        printf("%s\n", values[i]);
    };
}
```

If the argument happens to be optional, it should be checked.

```c

int cCommand(void* result[], void* _){
    char* distFolder = result[0];
    if (!distFolder){
        distFolder = "dist";
    }
}
```

- ### Positional Arguments.
This are similar to named arguments in all aspects other than the 
fact that they do require a flag. They contain descriptions and optional 
metadata, as well as the ability to be optional. 

```c
int main(int argc, const char* argv[]){
    addPositional( // Add a positional argument to the main command
        "file", // name of the argument
        "The file to run", // description
        "FILENAME", // metadata
        1, // number of arguments
        false // whether optional
    );

    addPositionalC(
        someCommand, // The command to add the argument
        "file", // name of the argument
        "The file to run", // description
        "FILENAME", // metadata
        1, // number of arguments
        false // whether optional
    );
}
```

## Running the application 
Once the commands and their arguments have been configured, 
the application can be run using the `runApp` function.
It runs the parser. If an parser error occurs, it logs the error
to stderr and returns `1`. If the parser is successful, the exit 
code from the command callback, or if a switch callback happens 
to return a non-zero value.


```c
int main(int argc, const char* argv[]){
    // setup
    return runApp(
        argc, // number of arg values
        argv, // arg values
        NULL // program namespace
    )
} 
```

## Limitations 

- ### Key-value argument pairs.
These is where arguments and their values are passed as one 
string separated with `=`.

```bash
command --argument=value
```

This will result to an error since the argument won't ne matched.

- ### Type conversion

The library doesn't support parsing integers and floats as a type. The values 
collected are stored as strings. This is because accounting for 
all integer sizes, and whether or not the values required are 
floats requires extra information about the arguments to be stored. 
The values collected should be validated in the callback functions.


- ### Default values 

The library doesn't support explicitly setting a default value for 
an argument. This is would be anti-intuitive since an argument can 
be made optional, and checked for null in the callback, then the 
default value assigned there.

```c
int cCommand(void* result[], void* _){
    char* optional = result[3];
    if (!optional){
        optional = "default value";
    }
}
```

## License
[MIT](LICENSE)