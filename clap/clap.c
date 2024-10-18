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

static void appendValue(FixedArray* array, void* value){
    if (!array || array->length == array->capacity){
        return;
    }
    array->values[array->length] = value;
    array->length ++;
}

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
