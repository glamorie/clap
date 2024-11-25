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

#if !defined(ICLAP_H)
#define ICLAP_H 
#include <stddef.h>
#include <stdbool.h>

typedef unsigned short slot_t;

/**
 * @brief Used to store a collection.
 * @param values array of pointers to the 
 * values.
 * @param length length of the array.
 */
typedef struct {
    void** values;
    size_t length;    
}clap_array_t;

/**
 * @brief Used to store a comparable string.
 * @param value pointer to the char array.
 * @param length flag's length. 
 */
typedef struct {
    char* value;
    size_t length;
}clap_string_t;

/**
 * @brief Used to store the types of values 
 * arguments expect.
 */
typedef enum {
    CLAP_STRING,
    CLAP_INTEGER,
    CLAP_FLOAT, 
    CLAP_FILE,
    CLAP_DIRECTORY,
    CLAP_PATH
}clap_value_t;

/**
 * @brief Contains the information about a positional
 * argument.
 * @param name argument's name.
 * @param description argument's description.
 * @param amount number of values the argument expects.
 * Zero means it can take in any number.
 * @param slot the position the values will be placed in
 * the result array.
 * @param required whether the argument must recieve 
 * values.
 * @param type the type of value the argument expects. 
 *   
 */
typedef struct {
    clap_string_t* name;
    char* description;
    size_t amount;
    slot_t slot;
    bool required;
    clap_value_t type;
}clap_positional_t;

/**
 * @brief Contains the information about a positional
 * argument.
 * @param flag argument's long flag.
 * @param alias argument's shorthand.
 * @param description argument's description.
 * @param amount number of values the argument expects.
 * Zero means it can take in any number.
 * @param slot the position the values will be placed in
 * the result array.
 * @param required whether the argument must recieve 
 * values.
 * @param type the type of value the argument expects. 
 *   
 */
typedef struct {
    clap_string_t* flag;
    char alias;
    char* description;
    size_t amount;
    slot_t slot;
    bool required;
    clap_value_t type;
}clap_argument_t;

/**
 * @brief Contains the information about option arguments.
 * @param flag argument's long flag.
 * @param alias argument's shorthand.
 * @param description argument's description.
 * @param amount number of values the argument expects.
 * Zero means it can take in any number.
 * @param slot the position the values will be placed in
 * the result array.
*/
typedef struct {
    clap_string_t* flag;
    char alias;
    char* description;
    slot_t slot;
}clap_option_t;


/**
 * @brief Function called when a command is parsed.
 * @param result array containing the values parsed
 * in as specified by the argument's slots.
 * @param data custom data passed as an argument to
 * app switches for configuring some application settings.
 */
typedef int(*clap_command_callback_t)(void* result[], void* data);

/**
 * @brief Stores information about a command.
 * @param name the command's name.
 * @param alias the command's shorthand.
 * @param description the command's description.
 * @param usage the command's usage strings.
 * @param positionals array containing the positional
 * arguments.
 * @param arguments array containing the named
 * arguments..
 * @param options array containing the options.
 * @param callback function called when the command is
 * parsed.
 */
typedef struct {
    clap_string_t* name;
    char alias;
    char* description;
    char** usage;
    clap_array_t* positionals;
    clap_array_t* arguments;
    clap_array_t* options;
    clap_command_callback_t callback;
}clap_command_t;

/**
 * @brief Structure containing the state of the parser.
 */
typedef struct clap_context_t clap_context_t;

/**
 * @brief Function called when a switch is parsed.
 * @param ctx parser's state when the switch is parsed.
 * @return Exit code. If a non-zero value is returned, the parser exits
 * and returns that value.
 */
typedef int(*clap_switch_callback_t)(const clap_context_t* ctx);

/**
 * @brief Stores information about a command group.
 * @param name group's name.
 * @param alias group's alias.
 * @param description group's description.
 * @param commands array containing the member commands.
 * @param groups array containing sub-groups.
 */
typedef struct {
    clap_string_t* name;
    char alias;
    char* description;
    clap_array_t* commands;
    clap_array_t* groups;
}clap_group_t;

/**
 * @brief Stores information about a switch (global option).
 * @param name switch's name.
 * @param alias switch's alias.
 * @param description switch's description.
 * @param exits whether the parser should call the callback and
 * exit when switch is parsed eg in --help.
 */
typedef struct {
    clap_string_t* name;
    char alias;
    char* description;
    bool exits;
    clap_switch_callback_t callback;
}clap_switch_t;

/**
 * @brief Stores information about the application.
 * @param name application's name.
 * @param version application's version.
 * @param description application's description.
 * @param usage application's usage strings.
 * @param commands array containing the application's
 * top-level commands.
 * @param groups array containing the application's
 * command groups.
 * @param switches array containing the application's
 * switches (global options).
 * @param main application's main command.
 */
typedef struct clap_app_t{
    char* name;
    char* description;
    char* version;
    char** usage;
    clap_array_t* commands;
    clap_array_t* groups;
    clap_array_t* switches;
    clap_command_t* main;
}clap_app_t;

/**
 * @brief Contain's the parser's state.
 * @param data shared data used in switches.
 * @param app pointer to the app.
 * @param command pointer to the command
 * being parsed. Can be null.
 * @param group pointer to the group being parsed.
 * @param argv the command line arguments.
 * @param argc the number of command line arguments.
 * @param index the current index during parsing.
 * @param trace the index to the latest parent, ie
 * group / command.
 * @param greedy whether the values with hyphens
 * have been escaped.
 */
struct clap_context_t {
    void* data;
    clap_app_t* app;
    clap_command_t* command;
    clap_group_t* group;
    char** argv;
    size_t argc;
    size_t index;
    slot_t trace;
    bool greedy;
};

#endif // ICLAP_H
