#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

#define ARRAY_FULL(t)(!t || t->length == t->capacity)

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

#define GET_AT(t, i)(t->values[i]);
#define PRINT_TAB() printf("    ");
#define IS_FLAG_ESCAPE(v) (v[1] == '-' && v[2] == 0)
#define NARGS_QUANTIFIER(nargs) nargs? nargs == 1? "": "s" : "(s)"

static void putCommandBits(Command* command){
    if (command->positionals && command->positionals->length){
        printf("Positional%c\n", command->positionals->length == 1? 0:'s');
        
        for (int i = 0; i < command->positionals->length; i ++){
            Positional* pos = GET_AT(command->positionals, i);
            PRINT_TAB();
            printf("%s",pos->name);
            if (pos->metadata){
                putchar(' ');
                printf("%s",pos->metadata);
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
            printf("--%s",arg->flag->value);
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
            printf("--%s\n        %s\n", opt->flag->value, opt->description);
      
        }
        putchar('\n');
    }
}

static void putSwitches(){
    if (app.switches && app.switches->length){
        printf("Global Option%c\n", app.switches->length == 1? 0: 's');
        for (int i = 0; i < app.switches->length; i ++){
            Switch* sw = GET_AT(app.switches, i);
            PRINT_TAB();
            if (sw->alias){
                printf("-%c, ", sw->alias);
            }
            printf("--%s\n        %s\n", sw->flag->value, sw->description);
        }
        putchar('\n');
    }
}

static void putMasterBits(FixedArray* commands, FixedArray* groups){
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

static void putBreadCrumb(FILE* stream){
    for (int i = 0; i <= masterTrace; i ++){
        fprintf(stream, "%s ", argValues[i]);
    }
}

static void putTry(){
    fprintf(stderr, "\nTry  : ");
    putBreadCrumb(stderr);
    fprintf(stderr, "--help\n");
}

static void putUsage(char* usage[]){
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
        printf("%s",usage[i]);
        putchar('\n');
        i++;
    }
    putchar('\n');
}

static void helpCommand(Command* command){
    putUsage(command->usage);
    printf("%s\n\n", command->description);
    putCommandBits(command);
    putSwitches();       
}

static void helpGroup(CommandGroup* group){
    printf("Usage: ");
    putBreadCrumb(stdout);
    puts("[-h|--help]\n");
    printf("%s\n\n", group->description);
    putMasterBits(group->commands, group->groups);
    putSwitches();
}

static void helpApp(){
    printf("%s v%s\n\n%s\n\n", app.name, app.version, app.description);
    putUsage(app.usage);
    if (app.main){
        putCommandBits(app.main);
    }
    putMasterBits(app.commands, app.groups);
    putSwitches();
}

void printHelp(){
    if (!app.name){
        return;
    }
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

static int helpSwitch(void* _){
    printHelp();
    return 0;
}

static int versionSwitch(void* _){
    printf("v%s\n", app.version);
    return 0;
}

static void freeCommand(Command* command){
    if (command->name){
        free(command->name);
    }
    if (command->positionals){
        for (int i = 0; i < command->positionals->length; i ++){
            free((Positional*)(command->positionals, i));
        }
        free(command->positionals->values);
        free(command->positionals);
    }
    if (command->arguments){
        for (int i = 0; i < command->arguments->length; i ++){
            Argument* arg = GET_AT(command->arguments, i);
            free(arg->flag);
            free(arg);
        }
        free(command->arguments->values);
        free(command->arguments);
    }
    if (command->options){
        for (int i = 0; i < command->options->length; i ++){
            Option* opt = GET_AT(command->options, i);
            free(opt->flag);
            free(opt);
        }
        free(command->options->values);
        free(command->options);
    }
    free(command);
}

static void freeMasterBits(FixedArray* groups, FixedArray* commands){
    if (commands){
        for (int i = 0; i < commands->length; i ++){
            Command* command = GET_AT(commands, i);
            freeCommand(command);
        }
        free(commands->values);
        free(commands);
    }

    if (groups){
        for (int i = 0; i < groups->length; i ++){
            CommandGroup* group = GET_AT(group->groups, i);
            FixedArray* cmds = group->commands;
            FixedArray* grps = group->commands;
            free(group->name);
            free(group);
            freeMasterBits(grps, cmds);
        }
        free(groups->values);
        free(groups);
    }
}

static void freeApp(){
    if (app.main){
        freeCommand(app.main);
    }
    freeMasterBits(app.groups, app.groups);
    app.name = NULL;
    app.version = NULL; 
    app.description = NULL;
    app.commands = NULL;
    app.groups = NULL;
    app.switches = NULL;
    app.usage = NULL;
}

static void* findMatch(String* flag, FixedArray* array, bool checkAlias, bool hasHyphen){
    if (!array){
        return NULL;
    }
    if (!checkAlias){
        int x = hasHyphen? 2: 0;
        for (int i = 0; i < array->length; i ++){
            FlagInfo* other = array->values[i];
            if (flagEquals(flag, other->flag, x)){
                return (void*)other;
            }
        }
        return NULL;
    }
    for (int i = 0; i < array->length; i ++){
        FlagInfo* other = array->values[i];
        if (other->alias == flag->value[hasHyphen]){
            return (void*)other;
        }
    }
    return NULL;
}

static int parseValues(void* result[], const char name[], bool isPositional, uslot nargs, uslot slot){
    size_t amount = 0;
    char** values;
    if (nargs == 1){
        char* value = NULL;
        if (argumentIndex == argCount){
            goto noValuesError;
        }
        value = argValues[argumentIndex];
        if (!isGreedy && value[0] == '-'){
            if (!IS_FLAG_ESCAPE(value)){
                goto noValuesError;
            }
            argumentIndex++;
            if (argumentIndex == argCount){
                goto noValuesError;
            }
            value = argValues[argumentIndex];
        }
        result[slot] = (void*)strdup(value);
        argumentIndex++;
        isGreedy = false;
        return 0;
    }
    if (nargs == 0){        
        amount = 0;
        int startIndex = argumentIndex;
        bool escapedFlag = false;
        while (argumentIndex < argCount){
            char* value = argValues[argumentIndex];
            if (!isGreedy && value[0] == '-'){
                bool isEscape = IS_FLAG_ESCAPE(value);
                if (!escapedFlag){
                    if (isEscape){
                        if (!amount){
                            isGreedy = true;
                            continue;
                        }
                        escapedFlag = true;
                    }
                    break;
                }else if (isEscape){
                    escapedFlag = true;
                }
            }else {
                amount ++;

            }
            argumentIndex++;
        }
        
        if (!amount){
            goto noValuesError;
        }
        values = malloc(sizeof(char*)*(amount+1));
        if (!values){
            perror("memory");
            return 1;
        }

        values[amount] = NULL;
        for (int i = 0; i < amount; i ++){
            char* value = argValues[startIndex+i];
            if (IS_FLAG_ESCAPE(value)){
                continue;
            }
            values[i] = value;
        }
        result[slot] = (void*)values;
        isGreedy = false;
        return 0;
    }else {
        values = malloc((nargs+1)*sizeof(char*));
        if (!values){
            perror("memory");
            return 1;
        }
        values[nargs] = NULL;
        size_t amount = 0;
        bool escapedFlag = false;
        while (argumentIndex < argCount){
            if (amount == nargs){
                break;
            }
            char* value = argValues[argumentIndex];
            if (!isGreedy && value[0] == '-'){
                bool isEscape = IS_FLAG_ESCAPE(value);
                if (isEscape){
                    if (!amount){
                        isGreedy = true;
                    }else {
                        escapedFlag = true;
                    }
                }else {
                    if (escapedFlag){
                        escapedFlag = false;
                        values[amount] = value;
                        amount++;
                    }else{
                        break;
                    }
                }
            }else {
                values[amount] = value;
                amount ++;
            }
            argumentIndex++;
        }

        if (!amount){
            goto invalidAmountError;
        }

        if (amount != nargs){
            goto invalidAmountError;
        }
        result[slot] = (void*)values;
        isGreedy = false;
        return 0;
    }
    noValuesError:
        if (isPositional){
            fprintf(stderr,"Error: Expected %d values for positional argument [%s]\n", nargs, name);
        }else {
            fprintf(stderr,"Error: Expected %d values for argument --%s\n", nargs, name);
        }
    goto exitF;

    invalidAmountError:
        if (isPositional){
            fprintf(stderr,"Error: Expected %d values for positional argument [%s]\n", nargs,  name);
        }else {
            fprintf(stderr,"Error: Expected %d values for --%s\n", nargs, name);
        }
    goto exitF;

    exitF:
        free(values);
        putTry();
    return 1;
}

static int runCommand(){
    uslot fields = currentCommand->fields;
    void** result = calloc(fields, sizeof(void*));
    if (!result){
        perror("memory");
        return 1;
    }
    int currentPositional = 0;
    int code = 0;
    int check = true;
    Positional* pos = NULL;
    Argument* arg = NULL;
    Option* opt = NULL;
    Switch* sw = NULL;

    while (argumentIndex < argCount){
        String value = {.value = (char*)argValues[argumentIndex], .length=strlen(argValues[argumentIndex])};
        if (!isGreedy && value.value[0] =='-'){
            if (IS_FLAG_ESCAPE(value.value)){
                isGreedy = true;
                argumentIndex++;
                continue;
            }
            bool isAlias = value.length == 2;
            if (arg = findMatch(&value, currentCommand->arguments, isAlias, true)){
                argumentIndex++;
                pos = NULL;
                if (code = parseValues(result, arg->flag->value, false, arg->nargs, arg->slot)){
                    check = false;
                    break;
                }
                continue;
            } else if (opt = findMatch(&value, currentCommand->options, isAlias, true)){
                result[opt->slot] = (void*)true;
                
            } else if (sw = findMatch(&value, app.switches, isAlias, true)){
                if (code = sw->callback(appNamespace)){
                    check = false;
                    break;
                }
                if (sw->final){
                    check = false;
                    break;
                }
            } else {
                fprintf(stderr,"Error: Unrecognized option `%s`\n", value.value);
                putTry();
                code = 1; 
                break;
            }
        }else if (currentCommand->positionals && currentPositional++ < currentCommand->positionals->length){
            pos = GET_AT(currentCommand->positionals, currentPositional);
            arg = NULL;
            if (code = parseValues(result, pos->name, true, pos->nargs, pos->slot)){
                check = false;
                break;
            }
            currentPositional++;
            continue;
        }else {
            if (pos){
                fprintf(stderr,"Error: Positional argument [%s] Expected %d value%s\n", pos->name, pos->nargs, NARGS_QUANTIFIER(pos->nargs));
            }else if (arg){
                fprintf(stderr,"Error: Argument %s Expected --%d value%s\n", arg->flag->value, arg->nargs, NARGS_QUANTIFIER(arg->nargs));
            }else {
                fprintf(stderr,"Error: Unexpected value \"%s\"\n", value.value);
            }
            putTry();
            code = 1;
            check = false;
            break;
        }
        argumentIndex++;
    }

    if (!code && check){
        if (currentCommand->positionals){
            for (int i = 0; i < currentCommand->positionals->length; i ++){
                pos = GET_AT(currentCommand->positionals, i);
                if (pos->optional){
                    continue;
                }   
                if (!result[pos->slot]){
                    fprintf(stderr,"Error: Missing value for positional argument [%s]\n", pos->name);
                    putTry();
                    check = false;
                    break;
                }
            }
        }
        if (check && currentCommand->arguments){
            for (int i = 0; i < currentCommand->arguments->length; i ++){
                arg = GET_AT(currentCommand->arguments, i);
                if (arg->optional){
                    continue;
                }   
                if (!result[arg->slot]){
                    fprintf(stderr,"Error: Missing value for  argument --%s\n", arg->flag->value);
                    putTry();
                    check = false;
                    break;
                }
            }
        }
        CommandCallback* callback = currentCommand->callback;
        freeApp();
        if (check){
            code = callback(result, appNamespace);
        }
    }else {
        freeApp();
    }
    for (int i = 0; i < fields; i ++){
        if (!result[i]){
            continue;
        }
        free(result[i]);
        
    }
    free(result);
    return code;
}

int runApp(size_t argc, const char* argv[], void* namespace){
    argCount = argc;
    argValues = (char**)argv;
    appNamespace = namespace;
    int code = 0;
    while (argumentIndex < argCount){
        String value = {.value = (char*)argValues[argumentIndex], .length=strlen(argValues[argumentIndex])};
        if (!isGreedy && value.value[0] == '-'){
            if (IS_FLAG_ESCAPE(value.value)){
                isGreedy = true;
                argumentIndex++;
                continue;
            }
            Switch* sw;
            if (sw = findMatch(&value, app.switches,value.length == 2, true)){
                if (code = sw->callback(namespace)){
                    break;
                }
                if (sw->final){
                    break;
                }
            }else if (app.main){
                currentCommand = app.main;
                break;
            }else {
                fprintf(stderr,"Error: Unrecognised option `%s`\n", value.value);
                code = 1;
                break;
            }
        }else if(isGreedy && !masterTrace){
            if (app.main){
                currentCommand = app.main;
                break;
            }
            fprintf(stderr,"Error: Unexpected value \"%s\"\n", value.value);

        }else {
            if (currentCommand = findMatch(&value, !currentGroup? app.commands : currentGroup->commands ,value.length == 1, false)){
                masterTrace++;
                break;
            } else if (currentGroup = findMatch(&value, !currentGroup? app.groups : currentGroup->groups , value.length == 1, false)){
                masterTrace++;
            } else if (app.main && app.main->positionals && app.main->positionals->length){
                currentCommand = app.main;
                break;
            
            }else {
                if (app.commands && app.commands->length){
                    fprintf(stderr,"Error: Unrecognised command `%s`\n", value.value);
                }else {
                    fprintf(stderr,"Error: Unexpected value `%s`\n", value.value);
                }
                putTry();
                code = 1;
                break;
            }
        }
        argumentIndex++;
    }

    if (!code){
        if (!currentCommand){
            if (currentGroup){
                fprintf(stderr,"Error: Missing command name.\n");
                code = 1;
            }else {
                currentCommand = app.main;
            }
        }
        if (currentCommand){
            code = runCommand();
        }
    }else{
        freeApp();

    }
    return code;
}
