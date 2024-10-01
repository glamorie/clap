#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define ARRAY_FULL(t)(!t || t->length == t->capacity)
#define GET_AT(t, i)(t->values[i]);
#define PRINT_TAB() printf("    ");

typedef unsigned int slot_t;

typedef struct {
    size_t length;
    char* value;
}Flag;

static bool flagEquals(Flag* a, Flag* b){
    if (a->length != b->length){
        return false;
    }
    for (int i = 0; i < a->length; i ++){
        if (a->value[i] != b->value[i]){
            return false;
        }
    }
    return true;
}

typedef struct {
    size_t length;
    size_t capacity;
    void** values;
}Array;

static void appendValue(Array* array, void* value){
    if (!array || array->length == array->capacity){
        return;
    }
    array->values[array->length] = value;
    array->length++;
}

static Array* createArray(size_t capacity){
    if (!capacity){
        return NULL;
    }
    Array* array = malloc(sizeof(Array));
    if (!array){
        return NULL;
    }

    void** values = calloc(capacity, sizeof(void*));
    if (!values){
        free(array);
        return NULL;
    }
    array->values = values;
    array->capacity = capacity;
    array->length = 0;
    return array;
}

typedef struct {
    char* name;
    char* metadata;
    char* description;
    slot_t nargs;
    slot_t slot;
    bool optional;
}Positional;

typedef struct {
    Flag* flag;
    char alias;
    char* metadata;
    char* description;
    slot_t nargs;
    slot_t slot;
    bool optional;
}Argument;

typedef struct {
    Flag* flag;
    char alias;
    char* description;
    slot_t slot;
}Option;


typedef int(CommandCallback(void* result[], void* namespace));

typedef struct{
    Flag* name;
    char alias;
    char* description;
    char** usage;
    Array* positionals;
    Array* arguments;
    Array* options;
    slot_t fields;
    CommandCallback* callback;
}Command;

typedef struct{
    Flag* name;
    char alias;
    char* description;
    Array* commands;
    Array* groups;
}CommandGroup;


typedef int(SwitchCallback(void* namespace));

typedef struct{
    Flag* flag;
    char alias;
    char* description;
    SwitchCallback* callback;
    bool final;
}Switch;

typedef struct {
    char* name;
    char* version;
    char* description;
    char** usage;
    Array* commands;
    Array* groups;
    Array* switches;
    Command* main;
}CliApp;

typedef struct {
    slot_t a;
    slot_t b;
    slot_t c;
}Slots;

typedef struct {
    Flag* flag;
    char alias;
}FlagInfo;

static CliApp app = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static Command* currentCommand = NULL;
static CommandGroup* currentGroup = NULL;
static slot_t masterTrace = 0;
static slot_t argumentIndex = 1;
static char** argValues = NULL;
static size_t argCount = 0;
static void* appNamespace = NULL;
static bool isGreedy = false;

void addArgumentC(Command* command, const char flag[], char alias, const char description[], const char metadata[], slot_t nargs, bool optional){
    if (!command || ARRAY_FULL(command->arguments)){
        return;
    }
    Argument* arg = malloc(sizeof(Argument));
    if (!arg){
        return;
    }
    Flag* argflag = malloc(sizeof(Flag));
    if (!argflag){
        free(arg);
        return;
    }

    argflag->value = (char*)flag;
    argflag->length = strlen(flag);

    arg->flag = argflag;
    arg->alias = alias;
    arg->description = (char*)description;
    arg->metadata = (char*)metadata;
    arg->nargs = nargs;
    arg->optional = optional;
    arg->slot = command->fields;
    command->fields++;
    appendValue(command->arguments, arg);
}

void addPositionalC(Command* command, const char name[],  const char description[], const char metadata[], slot_t nargs, bool optional){
    if (!command || ARRAY_FULL(command->positionals)){
        return;
    }
    Positional* pos = malloc(sizeof(Positional));
    if (!pos){
        return;
    }
    pos->name = (char*)name;
    pos->description = (char*)description;
    pos->metadata = (char*)metadata;
    pos->nargs = nargs;
    pos->optional = optional;
    pos->slot = command->fields;
    command->fields++;
    appendValue(command->positionals, pos);
}

void addOptionC(Command* command, const char flag[], char alias, const char description[]){
    if (!command || ARRAY_FULL(command->options)){
        return;
    }
    Option* opt = malloc(sizeof(Option));
    if (!opt){
        return;
    }
    Flag* argflag = malloc(sizeof(Flag));
    if (!argflag){
        free(opt);
        return;
    }

    argflag->value = (char*)flag;
    argflag->length = strlen(flag);

    opt->flag = argflag;
    opt->alias = alias;
    opt->description = (char*)description;
    opt->slot = command->fields;
    command->fields++;
    appendValue(command->arguments, opt);
}

void addPositional(const char name[],  const char description[], const char metadata[], slot_t nargs, bool optional){
    if (!app.main){
        return;
    }
    addPositionalC(app.main, name, description, metadata, nargs, optional);
}

void addArgument(const char flag[], char alias, const char description[], const char metadata[], slot_t nargs, bool optional){
    if (!app.main){
        return;
    }
    addArgumentC(app.main, flag, alias, description, metadata, nargs, optional);
}

void addOption(const char flag[], char alias, const char description[]){
    if (!app.main){
        return;
    }
    addOptionC(app.main, flag, alias, description);
}

Command* addCommand(const char name[], char alias, const char description[], Slots slots, CommandCallback* callback, const char* usage[]){
    if (ARRAY_FULL(app.commands)){
        return NULL;
    }
    Command* command = malloc(sizeof(Command));
    if (!command){
        return NULL;
    }
    Flag* flag = malloc(sizeof(Flag));
    if (!flag){
        free(command);
        return NULL;
    }
    flag->value = (char*)name;
    flag->length = strlen(name);
    command->fields = 0;
    command->name = flag;
    command->alias = alias;
    command->usage = (char**)usage;
    command->description = (char*)description;
    command->positionals = createArray(slots.a);
    command->arguments = createArray(slots.b);
    command->options = createArray(slots.c);
    appendValue(app.commands, command);
    return command;
}

Command* addCommandG(CommandGroup* group,  char name[], char alias, const char description[], Slots slots, CommandCallback* callback, const char* usage[]){
    if (!group || ARRAY_FULL(group->commands)){
        return NULL;
    }
    Command* command = malloc(sizeof(Command));
    if (!command){
        return NULL;
    }
    Flag* flag = malloc(sizeof(Flag));
    if (!flag){
        free(command);
        return NULL;
    }
    flag->value = (char*)name;
    flag->length = strlen(name);
    command->fields = 0;
    command->name = flag;
    command->alias = alias;
    command->usage = (char**)usage;
    command->description = (char*)description;
    command->positionals = createArray(slots.a);
    command->arguments = createArray(slots.b);
    command->options = createArray(slots.c);
    appendValue(group->commands, command);
    return command;
}


CommandGroup* addGroup(const char name[], char alias, const char description[], Slots slots){
    if (!app.groups || ARRAY_FULL(app.groups) || 0 < slots.a + slots.b){
        return NULL;
    }
    CommandGroup* group = malloc(sizeof(CommandGroup));
    if (!group){
        return NULL;
    }
    Flag* flag = malloc(sizeof(Flag));
    if (!flag){
        free(group);
        return NULL;
    }
    flag->value = (char*)name;
    flag->length = strlen(name);
    group->name = flag;
    group->alias = alias;
    group->description = (char*)description;
    group->commands = createArray(slots.a);
    group->groups = createArray(slots.b);
    appendValue(app.groups, group);
    return group;
}

CommandGroup* addGroupG(CommandGroup* group, const char name[], char alias, const char description[], Slots slots){
    if (!group || ARRAY_FULL(group->groups) || 0 < slots.a + slots.b){
        return NULL;
    }
    CommandGroup* grp = malloc(sizeof(CommandGroup));
    if (!grp){
        return NULL;
    }
    Flag* flag = malloc(sizeof(Flag));
    if (!flag){
        free(grp);
        return NULL;
    }
    flag->value = (char*)name;
    flag->length = strlen(name);
    grp->name = flag;
    grp->alias = alias;
    grp->description = (char*)description;
    grp->commands = createArray(slots.a);
    grp->groups = createArray(slots.b);
    appendValue(group->groups, grp);
    return grp;
}

void addSwitch(const char flag[], char alias, const char description[], bool final, SwitchCallback* callback){
    if (!app.switches || ARRAY_FULL(app.switches)){
        return;
    }
    Switch* sw = malloc(sizeof(Switch));
    if (!sw){
        return;
    }
    Flag* swflag = malloc(sizeof(Flag));
    if (!swflag){
        free(sw);
        return;
    }
    swflag->value = (char*)flag;
    swflag->length = strlen(flag);
    sw->flag = swflag;
    sw->alias = alias;
    sw->description = (char*)description;
    sw->callback = callback;
    sw->final = final;
    appendValue(app.switches, sw);
}

int helpSwitch(void* _);
int versionSwitch(void* _);

void initApp(const char name[], const char version[], const char description[], Slots slots, bool addHelpSwitch, bool addVersionSwitch, const char* usage[]){
    if (app.name){
        return;
    }
    app.name =(char*)name;
    app.version = (char*)version;
    app.description = (char*)description;
    app.commands = createArray(slots.a);
    app.groups = createArray(slots.b);
    app.switches = createArray(slots.c+ addHelpSwitch + addVersionSwitch);
    app.usage = (char**)usage;
    if (addHelpSwitch){
        addSwitch("--help", 'h', "Show context sensitive help message and exit.", true, helpSwitch);
    }
    if (addVersionSwitch){
        addSwitch("--version", 'v', "Show application version and exit.", true, versionSwitch);
    }
}

void addMainCommand(Slots slots, CommandCallback* callback){
    if (app.main){
        return;
    }
    Command* main = malloc(sizeof(Command));
    if (!main){
        return;
    }
    main->fields = 0;
    main->name = NULL;
    main->alias = 0;
    main->description = NULL;
    main->positionals = createArray(slots.a);
    main->arguments = createArray(slots.b);
    main->options = createArray(slots.c);
    main->callback = callback;
    app.main = main;
}

void putCommandBits(Command* command){
    if (command->positionals && command->positionals->length){
        printf("Positional%c\n", command->positionals->length == 1? 0:'s');
        
        for (int i = 0; i < command->positionals->length; i ++){
            Positional* pos = GET_AT(command->positionals, i);
            PRINT_TAB();
            putWord(pos->name);
            if (pos->metadata){
                putchar(' ');
                putWord(pos->metadata);
            }
            printf("\n        %s\n", pos->description);

        }
        putchar('\n');
    }
    if (command->arguments && command->arguments->length){
        printf("Argument%c\n", command->arguments->length == 1? 0:'s');
        
        for (int i = 0; i < command->arguments->length; i ++){
            Argument* arg = GET_AT(command->arguments, i);
            PRINT_TAB();
            if (arg->alias){
                printf("-%c, ", arg->alias);
            }
            putWord(arg->flag->value);
            if (arg->metadata){
                printf(" %s", arg->metadata);
            }
            printf("\n        %s\n", arg->description);
        }
        putchar('\n');
    }

    if (command->options && command->options->length){
        printf("Option%c\n", command->options->length == 1? 0:'s');
        
        for (int i = 0; i < command->options->length; i ++){
            Option* opt = GET_AT(command->options, i);           
            PRINT_TAB();
            if (opt->alias){
                printf("-%c, ", opt->alias);
            }
            printf("%s\n        %s\n", opt->flag->value, opt->description);
      
        }
        putchar('\n');
    }
}

void putSwitches(){
    if (app.switches && app.switches->length){
        printf("Global Option%c\n", app.switches->length == 1? 0: 's');
        for (int i = 0; i < app.switches->length; i ++){
            Switch* sw = GET_AT(app.switches, i);
            PRINT_TAB();
            if (sw->alias){
                printf("-%c, ", sw->alias);
            }
            printf("%s\n        %s\n", sw->flag->value, sw->description);
        }
        putchar('\n');
    }
}

void putMasterBits(Array* commands, Array* groups){
    if (commands && commands->length){
        printf("Command%c\n", commands->length == 1? 0:'s');
        for (int i = 0; i < commands->length; i ++){
            Command* command = GET_AT(commands, i);
            PRINT_TAB();
            if (command->alias){
                printf("%c| ", command->alias);
            }
            printf("%s\n        %s\n", command->name->value, command->description);
        }
        putchar('\n');
    }
    if (groups && groups->length){
        printf("Command Group%c\n", groups->length == 1? 0:'s');
        for (int i = 0; i < groups->length; i ++){
            Command* grp = GET_AT(groups, i);
            PRINT_TAB();
            if (grp->alias){
                printf("%c| ", grp->alias);
            }
            printf("%s\n        %s\n", grp->name->value, grp->description);
        }
        putchar('\n');
    }
}

void putBreadCrumb(FILE* stream){
    for (int i = 0; i <= masterTrace; i ++){
        fprintf(stream, "%s ", argValues[i]);
    }
}

void putTry(){
    fprintf(stderr, "\nTry  : ");
    putBreadCrumb(stderr);
    fprintf(stderr, "--help\n");
}

void putUsage(char* usage[]){
    if (!usage || !usage[0]){
        printf("Usage: ");
        putBreadCrumb(stdout);
        puts("[-h|--help]\n");
        return;
    }
    if (!usage[1]){
        printf("Usage: ");
        putBreadCrumb(stdout);
        printf("%s\n\n", usage[1]);
        return;
    }
    
    puts("Usage:");
    int i = 0;
    while (usage[i]){
        PRINT_TAB();
        putBreadCrumb(stdout);
        putWord(usage[i]);
        putchar('\n');
        i++;
    }
    putchar('\n');
}

void helpCommand(Command* command){
    putUsage(command->usage);
    printf("%s\n\n", command->description);
    putCommandBits(command);
    putSwitches();       
}

void helpGroup(CommandGroup* group){
    printf("Usage: ");
    putBreadCrumb(stdout);
    puts("[-h|--help]\n");
    printf("%s\n\n", group->description);
    putMasterBits(group->commands, group->groups);
    putSwitches();
}

void helpApp(){
    printf("%s v%s\n\n%s\n\n", app.name, app.version, app.description);
    putUsage(app.usage);
    if (app.main){
        putCommandBits(app.main);
    }
    putMasterBits(app.commands, app.groups);
    putSwitches();
}

void printHelp(){
    if (currentCommand && currentCommand != app.main){
        helpCommand(currentCommand);
        return;
    }
    if (currentGroup){
        helpGroup(currentGroup);
        return;
    }
    helpApp();
}

int helpSwitch(void* _){
    printHelp();
}

int versionSwitch(void* _){
    printf("v%s\n", app.version);
}
