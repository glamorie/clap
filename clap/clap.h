#ifndef CLAP_H
#define CLAP_H

/**
 * Contains information about a command.
 */
typedef struct Command Command;

/**
 * Structure for containing the metadata about a command group
 */
typedef struct CommandGroup CommandGroup;

/**
 * The function called when arguments are successfully parsed.
 * @param result An array of values collected from the cli arguments.
 * They are in the order in which the arguments are defined.
 * @param appNamespace This is some custom data that is passed
 * on to switch callbacks to change the application state.
 */
typedef int(CommandCallback(void* result[], void* appNamespace));

/**
 * The function called when a switch is parsed.
 * @param appNamespace The application custom state data.
 */
typedef int(SwitchCallback(void* appNamespace));

/**
 * Used for defining the space allocated for the 
 * arguments, commands and command groups.
 */
typedef struct {
    unsigned int a;
    unsigned int b;
    unsigned int c;
}Slots;

/**
 * Adds a positional argument to the default app command.
 * @param name name of the positional argument. 
 * @param description argument's description.
 * @param metadata some string hinting the kind of value(s) the 
 * argument should receive.
 * @param nargs the number of values the argument takes in. Zero
 * should be used if an argument takes in any amount provided.
 * @param optional whether the argument is not a mandatory argument.
 */
void addPositional(
    const char name[],  
    const char description[], 
    const char metadata[], 
    unsigned int nargs, 
    bool optional
);

/**
 * Adds a named argument to the default app command.
 * @param flag the argument's flag.
 * @param alias the argument's shorthand.
 * @param description the argument's description.
 * @param metadata hint for what kind of values the argument expects.
 * @param nargs the number of values the argument expects. Zero if 
 * it takes in any number supplied.
 * @param optional whether it not mandatory.
 */
void addArgument(
    const char flag[], 
    char alias, 
    const char description[], 
    const char metadata[], 
    unsigned int nargs, 
    bool optional
);

/***
 * Add an option flag to the default app command.
 * @param flag the option's flag.
 * @param alias the argument's shorthand.
 * @param description the argument's description.
 */
void addOption(
    const char flag[], 
    char alias, 
    const char description[]
);

/**
 * Adds a positional argument to a specific app.
 * @param command pointer to the command.
 * @param name name of the positional argument. 
 * @param description argument's description.
 * @param metadata some string hinting the kind of value(s) the 
 * argument should receive.
 * @param nargs the number of values the argument takes in. Zero
 * should be used if an argument takes in any amount provided.
 * @param optional whether the argument is not a mandatory argument.
 */
void addPositionalC(
    Command* command, 
    const char name[], 
    const char description[], 
    const char metadata[], 
    unsigned int nargs, 
    bool optional
);

/**
 * Adds a named argument to a specific command.
 * @param command pointer to the command.
 * @param flag the argument's flag.
 * @param alias the argument's shorthand.
 * @param description the argument's description.
 * @param metadata hint for what kind of values the argument expects.
 * @param nargs the number of values the argument expects. Zero if 
 * it takes in any number supplied.
 * @param optional whether it not mandatory.
 */
void addArgumentC(
    Command* command, 
    const char flag[], 
    char alias, 
    const char description[], 
    const char metadata[], 
    unsigned int nargs, 
    bool optional
);

/***
 * Add an option flag to a specific command.
 * @param command pointer to the command.
 * @param flag the option's flag.
 * @param alias the argument's shorthand.
 * @param description the argument's description.
 */
void addOptionC(
    Command* command, 
    const char flag[], 
    char alias, 
    const char description[]
);

/**
 * Initialises the application.
 * @param name app name.
 * @param version app version.
 * @param description app description.
 * @param slots a= number of commands in the app, b= number of command groups
 * int he app and c= number of switches in the app excluding the help and version.
 * @param addHelpSwitch whether to add the -h, --help switch for the app.
 * @param addVersionFlag whether to add the -v, --version switch.
 * @param usage a null terminated array of strings demonstrating how to use the app.
 */
void initApp(
    const char name[], 
    const char version[], 
    const char description[], 
    Slots slots, bool addHelpSwitch, 
    bool addVersionSwitch, 
    const char* usage[]
);

/**
 * Adds the default app command, that's called when none is specified.
 * @param slots a= number of positional arguments, b= number of named arguments, c=
 * number of option arguments.
 * @param callback the function called when the values have been successfully parsed.
 */
void addDefaultCommand(Slots slots, CommandCallback* callback);

/***
 * Adds a command to the app.
 * @param name the name of the command.
 * @param alias a letter or symbol to be used as a shorthand for the command or the null
 * character if the command doesn't have an alias.
 * @param description command description.
 * @param slots a= number of positional arguments, b= number of named arguments, c=
 * number of option arguments.
 * @param callback the function called when the values have been successfully parsed.
 * @param usage a null terminated array of strings demonstrating how to use the command.
 * @returns a pointer to a command.
 */
Command* addCommand(
    const char name[], 
    char alias, 
    const char description[], 
    Slots slots, 
    CommandCallback* callback, 
    const char* usage[]
);

/***
 * Adds a command group to the app.
 * @param name group's name.
 * @param alias group shorthand.
 * @param description group description.
 * @param slots a= number of commands in the group, b= number of sub-groups in
 * the group.
 * @returns a pointer to the command group.
 */
CommandGroup* addGroup(const char name[], char alias, const char description[], Slots slots);

/**
 * Adds a switch to the application. 
 * @param flag the flag of the switch. Must start with double hyphens.
 * @param alias a shorthand for the switch.
 * @param description switch description.
 * @param final whether the switch exit's after its callback is executed.
 * @param callback the function executed when the switch is supplied.
 */
void addSwitch(
    const char flag[], 
    char alias, 
    const char description[], 
    bool final, 
    SwitchCallback* callback
);

/**
 * Adds a command to a command group.
 * @param group The group in which the command is a member.
 * @param name the command's name.
 * @param alias the command's shorthand.
 * @param description the command's description
 * @param slots a= number of commands in the app, b= number of command groups
 * int he app and c= number of switches in the app excluding the help and version.
 * @param callback function executed when the values are collected successfully.
 * @param usage a null terminated array of strings demonstrating how to use the command
 * @returns A pointer to the command.
 */
Command* addCommandG(
    CommandGroup* group,  
    char name[], 
    char alias, 
    const char description[], 
    Slots slots, 
    CommandCallback* callback, 
    const char* usage[]
);

/**
 * Adds a sub command group to a command group.
 * @param group the command group in which the group is a member.
 * @param name the name of the command group.
 * @param alias the shorthand for the command.
 * @param description command group's description.
 * @param slots a= number of commands in the group, b= number of sub-groups in
 * the group.  
 */
CommandGroup* addGroupG(
    CommandGroup* group, 
    const char name[], 
    char alias, 
    const char description[], 
    Slots slots
);

/**
 * Runs the cli app.
 * @param argc the length of the command line values.
 * @param argv the command line values.
 * @param appNamespace the app namespace.
 */
int runApp(unsigned long int argc, const char* argv[], void* appNamespace);
#endif