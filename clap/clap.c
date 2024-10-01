#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


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
