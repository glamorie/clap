#if !defined(CLAP_MAIN_H)
#define CLAP_MAIN_H
// MIT License

// Copyright (c) 2024 Glitchie

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "iclap.h"

/**
 * @brief Prints context-sensitive help message.
 * @param ctx The parser's context.
 */
void print_help(clap_context_t* ctx);

/**
 * @brief Default callback for the help switch.
 * @param ctx The parser's context.
 */
int default_help_switch_fn(const clap_context_t* ctx);
/**
 * @brief Default callback for the version switch.
 * @param ctx The parser's context.
 */
int default_version_switch_fn(const clap_context_t* ctx);

/**
 * @brief Parses the command line arguments and runs the 
 * callback of the command called.
 * @param app Pointer to the application.
 * @param argc Length of the command line arguments.
 * @param argv Pointer to the command line arguments.
 * @param data The custom data to be passed down to the
 * switch and command callbacks to manage shared application
 * state.
 */
int clap_run(clap_app_t* app, size_t argc, const char* argv[], void* data);

/**
 * @brief Sets up the variables used in the rest of the 
 * macros to track index and slots of arguments and commands
 * as they get setup.
 */
#define clap_begin() unsigned  clap_index = 0; unsigned clap_slot = 0; 

/**
 * @brief Initialises the application.
 * @param name App's name.
 * @param version App's version.
 * @param description App's description.
 * @param ... Usage strings for the application.
 * @return Pointer to the application.
 */
#define clap_app(name, version, description, ...) &(clap_app_t){ \
    name, description, version, (char*[]){__VA_ARGS__, NULL}, NULL, NULL, NULL, NULL}

/**
 * @brief Sets upm the main command for the application.
 * @param app Pointer to the application.
 * @param callback Command's callback function.
 */
#define clap_main(app, callback) app->main = &(clap_command_t){ \
    NULL, 0, NULL, NULL, NULL, NULL, NULL, callback}

/**
 * @brief Used to initialize the clap string type.
 * @param value The char array.
 * @return Pointer to the string.
 */
#define clap_string(value) &(clap_string_t){value , sizeof(value) - 1}

/**
 * @brief Called when command configuration begins. Resets the declared 
 * index and slot integers which are used to set the index and
 * allocate the slots of the arguments in the result array.
 */
#define clap_begin_command() do {clap_index = 0; clap_slot = 0;} while(0)

/**
 * @brief Called when setting up the named arguments of a command.
 * @param command Pointer to the command being configured.
 * @param number The number of named arguments the command expects.
 */
#define clap_begin_arguments(command, number) do { \
        clap_index = 0; \
        void* buffer[number]; \
        command->arguments = &(clap_array_t){buffer, number}; \
    } while(0)

/**
 * @brief Called when setting up the positional arguments of a command.
 * @param command Pointer to the command being set up.
 * @param number The number of positional arguments the command 
 * expects.
 */
#define clap_begin_positionals(command, number) do { \
        clap_index = 0; \
        void* buffer[number]; \
        command->positionals = &(clap_array_t){buffer, number}; \
    } while(0)

/**
 * @brief Called when setting up option arguments for a command.
 * @param command Pointer to the command being set up.
 * @param number The number of option arguments for the command.
 */
#define clap_begin_options(command, number) do { \
        clap_index = 0; \
        void* buffer[number]; \
        command->options = &(clap_array_t){buffer, number}; \
    } while(0)

/**
 * @brief Called when setting adding commands to a parent, ie
 * the app or a command group.
 * @param parent App/group the command being set up.
 * @param number Number of commands the app/group contains.
 */
#define clap_begin_commands(parent, number) do { \
        clap_index = 0; \
        void* buffer[number]; \
        parent->commands = &(clap_array_t){buffer, number}; \
    } while(0)

/**
 * @brief Called when setting up command groups for an app
 * or another command goup.
 * @param parent App/group being set up.
 * @param number Number of command groups the app/parent group
 * contains.
 */
#define clap_begin_groups(parent, number) do { \
        clap_index = 0; \
        void* buffer[number]; \
        parent->groups = &(clap_array_t){buffer, number}; \
    } while(0)

/**
 * @brief Called when setting up switches for the app.
 * @param app Pointer to the app the switches are for.
 * @param number The number of switches the app contains
 * exclusive of the help and the version switch. The two
 * are added automatically.
 */
#define clap_setup_switches(app, number) do { \
        clap_index = 0; \
        void* buffer[number+2]; \
        app->switches = &(clap_array_t){buffer, number+2}; \
        app->switches->values[number] = &(clap_switch_t){ \
        clap_string("help"), 'h', "Show context-sensitive help and exit.", true, default_help_switch_fn}; \
        app->switches->values[number + 1] = &(clap_switch_t){ \
        clap_string("version"), 'v', "Show app version and exit.", true, default_version_switch_fn}; \
    } while(0) 

/**
 * @brief Sets up a named argument in a command.
 * @param name Argument's name.
 * @param alias Argument's shorthand.
 * @param description Argument's description.
 * @param nargs Number of values the argument expects.
 * Zero if the argument can take in an unbound amount.
 * @param type The type of values the argument expects.
 * @param required Whether the argument's values are 
 * compulsory.
 * @note Should be appear after the `clap_arguments_setup` macro
 * has been called.
 */
#define clap_argument(command, name, alias, description, nargs, type, required) \
    command->arguments->values[clap_index++] = &(clap_argument_t){ clap_string(name), \
    alias, description, nargs, clap_slot++, required, type}

/**
 * @brief Sets up a positonal argument for a command.
 * @param name Argument's name.
 * @param description Argument's description.
 * @param nargs Number of values the argument requires.
 * zero if the values are unbound.
 * @param type Type of values the argument expects.
 * @param required Whether the values are compulsory.
 * @note Should be appear after `clap_positionals_setup`
 */
#define clap_positional(command, name, description, nargs, type, required) \
    command->positionals->values[clap_index++] = &(clap_positional_t){ clap_string(name), \
    description, nargs, clap_slot++, required, type}

/**
 * @brief Sets up an option argument for a command.
 * @param name Argument's name.
 * @param alias Argument's shorthand.
 * @param description Argument's description.
 * @note Should be appear after `clap_options_setup`
 */
#define clap_option(command, name, alias, description) \
    command->options->values[clap_index++] = &(clap_option_t){ clap_string(name), \
    alias, description, clap_slot++}

/**
 * @brief Initialises a command.
 * @param name Command's name.
 * @param alias Command's shorthand.
 * @param description Command's description.
 * @param callback Command's callback function.
 * @param ... The usage strings of the command.
 * @return Pointer to the command group
 */
#define clap_command(name, alias, description, callback, ...) &(clap_command_t){ \
    clap_string(name), alias, description, (char*[]){__VA_ARGS__, NULL}, NULL, NULL, NULL, callback}

/**
 * @brief Initialises a command group.
 * @param name Group's name.
 * @param alias Group's shorthand.
 * @param description Group's description.
 * @return Pointer to the command group.
 */
#define clap_group(name, alias, description) &(clap_group_t){ \
    clap_string(name), alias, description, NULL, NULL}

/**
 * @brief Adds an already configured command to its
 * parent app or group.
 * @param parent The app/group the command is part
 * of.
 * @param command Pointer to the command being added.
 * @note Should appear after `clap_begin_commands`.
 */
#define clap_add_command(parent, command) parent->commands->values[clap_index++] = command

/**
 * @brief Adds an already configured group to a parent
 * app or group.
 * @param parent The app/group the group is a part of.
 * @param group Pointer to the group being adde.d
 * @note Should appear after `clap_begin_groups` 
 */
#define clap_add_group(parent, group) parent->groups->values[clap_index++] = group

#endif // CLAP_MAIN_H
