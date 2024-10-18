#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef unsigned uslot;
typedef unsigned short ushort;

typedef struct {
    size_t length;
    char* value;
}String;

static bool flagEquals(String* input, String* flag, ushort padding){
    if (input->length != (flag->length + padding)){
        return false;
    }
    for (int i = 0; i < flag->length; i++) {
        if (input->value[i + padding] != flag->value[i]){
            return false;
        }
    }
    return true;
}

typedef struct {
    size_t length;
    size_t capacity;
    void** values;
}FixedArray;

static FixedArray* createArray(size_t capacity){
    if (!capacity){
        return NULL;
    }
    FixedArray* array = malloc(sizeof(FixedArray));
    if (!array){
        return NULL;
    }
    array->values = malloc(sizeof(void*)*capacity);
    if (!array->values){
        free(array);
        return NULL;
    }
    array->length = 0;
    array->capacity = 0;
    return array;
}

#define appendValue(array, value) do {                \
    (array)->values[(array)->length] = (value);       \
    (array)->length++;                                  \
} while(0)

typedef struct {
    char* name;
    char* metadata;
    char* description;
    uslot nargs;
    uslot slot;
    bool optional;
}Positional;

typedef struct {
    String* flag;
    char alias;
    char* metadata;
    char* description;
    uslot nargs;
    uslot slot;
    bool optional;
}Argument;

typedef struct {
    String* flag;
    char alias;
    char* description;
    uslot slot;
}Option;

typedef int(CommandCallback(void* result[], void* namespace));

typedef struct{
    String* name;
    char alias;
    char* description;
    char** usage;
    FixedArray* positionals;
    FixedArray* arguments;
    FixedArray* options;
    uslot fields;
    CommandCallback* callback;
}Command;

typedef struct{
    String* name;
    char alias;
    char* description;
    FixedArray* commands;
    FixedArray* groups;
}CommandGroup;

typedef int(SwitchCallback(void* namespace));

typedef struct{
    String* flag;
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
    FixedArray* commands;
    FixedArray* groups;
    FixedArray* switches;
    Command* main;
}CliApp;

typedef struct {
    uslot a;
    uslot b;
    uslot c;
}Slots;

typedef struct {
    String* flag;
    char alias;
}FlagInfo;
    
static CliApp app = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
static Command* currentCommand = NULL;
static CommandGroup* currentGroup = NULL;
static uslot masterTrace = 0;
static uslot argumentIndex = 1;
static char** argValues = NULL;
static size_t argCount = 0;
static void* appNamespace = NULL;
static bool isGreedy = false;

bool addArgumentC(Command* command, const char flag[], char alias, const char description[], const char metadata[], uslot nargs, bool optional){
    if (!command || ARRAY_FULL(command->arguments)){
        return false;
    }
    Argument* arg = malloc(sizeof(Argument));
    if (!arg){
        return true;
    }
    String* argflag = malloc(sizeof(String));
    if (!argflag){
        free(arg);
        return true;
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
    return false;
}

bool addPositionalC(Command* command, const char name[],  const char description[], const char metadata[], uslot nargs, bool optional){
    if (!command || ARRAY_FULL(command->positionals)){
        return false;
    }
    Positional* pos = malloc(sizeof(Positional));
    if (!pos){
        return true;
    }
    pos->name = (char*)name;
    pos->description = (char*)description;
    pos->metadata = (char*)metadata;
    pos->nargs = nargs;
    pos->optional = optional;
    pos->slot = command->fields;
    command->fields++;
    appendValue(command->positionals, pos);
    return false;
}

bool addOptionC(Command* command, const char flag[], char alias, const char description[]){
    if (!command || ARRAY_FULL(command->options)){
        return false;
    }
    Option* opt = malloc(sizeof(Option));
    if (!opt){
        return true;
    }
    String* argflag = malloc(sizeof(String));
    if (!argflag){
        free(opt);
        return true;
    }

    argflag->value = (char*)flag;
    argflag->length = strlen(flag);

    opt->flag = argflag;
    opt->alias = alias;
    opt->description = (char*)description;
    opt->slot = command->fields;
    command->fields++;
    appendValue(command->arguments, opt);
    return false;
}

bool addPositional(const char name[],  const char description[], const char metadata[], uslot nargs, bool optional){
    if (!app.main){
        return false;
    }
    return addPositionalC(app.main, name, description, metadata, nargs, optional);
}

bool addArgument(const char flag[], char alias, const char description[], const char metadata[], uslot nargs, bool optional){
    if (!app.main){
        return false;
    }
    return addArgumentC(app.main, flag, alias, description, metadata, nargs, optional);
}

bool addOption(const char flag[], char alias, const char description[]){
    if (!app.main){
        return false;
    }
    return addOptionC(app.main, flag, alias, description);
}

Command* addCommand(const char name[], char alias, const char description[], Slots slots, CommandCallback* callback, const char* usage[]){
    if (ARRAY_FULL(app.commands)){
        return NULL;
    }
    Command* command = malloc(sizeof(Command));
    if (!command){
        return NULL;
    }
    String* flag = malloc(sizeof(String));
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
    String* flag = malloc(sizeof(String));
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
    String* flag = malloc(sizeof(String));
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
    String* flag = malloc(sizeof(String));
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

bool addSwitch(const char flag[], char alias, const char description[], bool final, SwitchCallback* callback){
    if (!app.switches || ARRAY_FULL(app.switches)){
        return false;
    }
    Switch* sw = malloc(sizeof(Switch));
    if (!sw){
        return true;
    }
    String* swflag = malloc(sizeof(String));
    if (!swflag){
        free(sw);
        return true;
    }
    swflag->value = (char*)flag;
    swflag->length = strlen(flag);
    sw->flag = swflag;
    sw->alias = alias;
    sw->description = (char*)description;
    sw->callback = callback;
    sw->final = final;
    appendValue(app.switches, sw);
    return false;
}

int helpSwitch(void* _);
int versionSwitch(void* _);

bool initApp(const char name[], const char version[], const char description[], Slots slots, bool addHelpSwitch, bool addVersionSwitch, const char* usage[]){
    if (app.name){
        return true;
    }
    app.name =(char*)name;
    app.version = (char*)version;
    app.description = (char*)description;
    app.commands = createArray(slots.a);
    app.groups = createArray(slots.b);
    app.switches = createArray(slots.c+ addHelpSwitch + addVersionSwitch);
    app.usage = (char**)usage;
    if (addHelpSwitch){
        addSwitch("help", 'h', "Show context sensitive help message and exit.", true, helpSwitch);
    }
    if (addVersionSwitch){
        addSwitch("version", 'v', "Show application version and exit.", true, versionSwitch);
    }
    return false;
}

bool addDefaultCommand(Slots slots, CommandCallback* callback){
    if (app.main){
        return false;
    }
    Command* main = malloc(sizeof(Command));
    if (!main){
        return true;
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
    return false;
}
