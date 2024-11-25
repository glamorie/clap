#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "iclap.h"
#include "imacros.h"

#define console_width 80 // TODO: implement function for getting the console width.

static const char* metadata[] = {
    "TEXT","INTEGER","FLOAT",
    "FILE","DIRECTORY", "PATH"};

typedef unsigned short ushort_t;

static bool string_compare(clap_string_t* a, clap_string_t* b, ushort_t padding){
    if (a->length != (b->length + padding)){
        return false;
    }
    for (int i = 0; i < b->length; i++) {
        if (a->value[i + padding] != b->value[i]){
            return false;
        }
    }
    return true;   
}

void print_desc(const char* text, unsigned offset, size_t width) {
    char c = 0;
    size_t line = offset;
    size_t start = 0;
    size_t length = 0;
    bool metword = false;
    for (size_t i = 0; c = text[i]; i++) {
        if (!isspace(c)) {
            if (!metword) {
                start = i;
                metword = true;
                length = 1;
            } else {
                length++; 
            }
            continue;
        } 
        if (!metword) {
            continue;
        }
        if (line == 0 || line + length > width) {
            printf("\n" TAB "%.*s", (int)length, &text[start]);
            line = 8 + length + 1; 
        } else {
            printf(" %.*s", (int)length, &text[start]);
            line += length + 1;
        }
        metword = false;
        length = 0;
    }
    if (metword) {
        if (line == 0 || line + length > width) {
            printf("\n" TAB "%.*s", (int)length, &text[start]);
        } else {
            printf(" %.*s", (int)length, &text[start]);
        }
    }
    putchar('\n');
}

unsigned get_meta_length(clap_value_t t){
    switch (t){
        case CLAP_STRING:return 7;
        case CLAP_INTEGER: return 10;
        case CLAP_FLOAT: return 8;
        case CLAP_FILE: return 7;
        case CLAP_DIRECTORY: return 12; 
        case CLAP_PATH: return 7;
    }
}

unsigned arg_flag_offset(clap_argument_t* arg){
    return 4 + 
    (arg->alias? 4 : 0 ) +
    (arg->flag->length + 2) +
    1 + 
    get_meta_length(arg->type) + 1;
}

unsigned pos_flag_offset(clap_positional_t* pos){
    return 4 +
    pos->name->length +
    1 + 
    get_meta_length(pos->type) +1;
}

typedef struct {
    clap_string_t* name;
    char alias;
}flag_info_t;

unsigned other_flag_offset(flag_info_t* arg, bool hyphenated){
    return 4 +
    (arg->alias? 1 + hyphenated + 1 + 1: 0 ) +
    (arg->name->length + hyphenated + hyphenated) + 1;
}

unsigned get_arg_offsets(clap_array_t* array){
    unsigned offset = 0;
    for (size_t i = 0; i < array->length; i++){
        unsigned value = arg_flag_offset(AT(array, i));
        if (value > offset) offset = value; 
    }
    return offset;
}

unsigned get_pos_offset(clap_array_t* array){
    unsigned offset = 0;
    for (size_t i = 0; i < array->length; i++){
        unsigned value = pos_flag_offset(AT(array, i)); 
        if (value > offset) offset = value;
    }
    return offset;
}

unsigned get_other_offsets(clap_array_t* array, bool hyphenated){
    unsigned offset = 0;
    for (size_t i = 0; i < array->length; i++){
        flag_info_t* flag = (flag_info_t*)array->values[i];
        unsigned value = other_flag_offset(flag, hyphenated);
        if (value > offset) offset = value;
    }
    return offset;
}

void bridge_offset(unsigned offset, unsigned foffset){
    unsigned dif = offset - foffset;
    if (!dif){
        return;
    }
    for (unsigned i = 0; i < dif; i++) {
        putchar(' ');
    }
}

static void print_command_bits(clap_command_t* command){
    if (command->positionals && command->positionals->length){
        printf(ftitle("Positional%c\n"), QUANTIFIER(command->positionals));
        unsigned offset = get_pos_offset(command->positionals);
        for (int i = 0; i < command->positionals->length; i ++){
            clap_positional_t* pos = AT(command->positionals, i);
            PRINT_TAB();
            printf("%s [%s] ",pos->name, metadata[pos->type]);
            bridge_offset(offset, pos_flag_offset(pos));
            print_desc(pos->description, offset, console_width);
        }
        putchar('\n');
    }
    if (command->arguments && command->arguments->length){
        printf(ftitle("Argument%c\n"), QUANTIFIER(command->arguments));
        unsigned offset = get_arg_offsets(command->arguments);
        for (int i = 0; i < command->arguments->length; i ++){
            clap_argument_t* arg = AT(command->arguments, i);
            PRINT_TAB();
            if (arg->alias){
                printf("-%c, ", arg->alias);
            }
            printf("--%s [%s] ",arg->flag->value, metadata[arg->type]);
            bridge_offset(offset, arg_flag_offset(arg));
            print_desc(arg->description, offset, console_width);
        }
        putchar('\n');
    }
    if (command->options && command->options->length){
        printf(ftitle("Option%c\n"), QUANTIFIER(command->options));
        unsigned offset = get_other_offsets(command->options, true);
        for (int i = 0; i < command->options->length; i ++){
            clap_option_t* opt = AT(command->options, i);           
            PRINT_TAB();
            if (opt->alias){
                printf("-%c, ", opt->alias);
            }
            printf("--%s ", opt->flag->value);
            bridge_offset(offset, other_flag_offset((void*)opt, true));
            print_desc(opt->description, offset, console_width);
        }
        putchar('\n');
    }
}

static void print_switches(clap_app_t* app){
    if (app->switches && app->switches->length){
        printf(ftitle("Global Option%c\n"), QUANTIFIER(app->switches));
        unsigned offset = get_other_offsets(app->switches, true);
        for (int i = 0; i < app->switches->length; i ++){
            clap_switch_t* sw = AT(app->switches, i);
            PRINT_TAB();
            if (sw->alias){
                printf("-%c, ", sw->alias);
            }
            printf("--%s ", sw->name->value);
            bridge_offset(offset, other_flag_offset((void*)sw, true));
            print_desc(sw->description, offset, console_width);
        }
    }
}

static void print_parent_bits(clap_array_t* commands, clap_array_t* groups){
    if (commands && commands->length){
        printf(ftitle("Command%c\n"), QUANTIFIER(commands));
        unsigned offset = get_other_offsets(commands, false);
        for (int i = 0; i < commands->length; i ++){
            clap_command_t* command = AT(commands, i);
            PRINT_TAB();
            if (command->alias){
                printf("%c/", command->alias);
            }
            printf("%s ", command->name->value);
            bridge_offset(offset, other_flag_offset((void*)command, false));
            print_desc(command->description, offset, console_width);
        }
        putchar('\n');
    }
    if (groups && groups->length){
        printf(ftitle("Command Group%c\n"), QUANTIFIER(groups));
        unsigned offset = get_other_offsets(groups, false);
        for (int i = 0; i < groups->length; i ++){
            clap_group_t* grp = AT(groups, i);
            PRINT_TAB();
            if (grp->alias){
                printf("%c/", grp->alias);
            }
            printf("%s ", grp->name->value);
            bridge_offset(offset, other_flag_offset((void*)grp, false));
            print_desc(grp->description, offset, console_width);
        }
        putchar('\n');
    }
}

static void print_breadcrumb(const clap_context_t* ctx, FILE* stream){
    for (int i = 0; i <= ctx->trace; i ++){
        fprintf(stream, "%s ", ctx->argv[i]);
    }
}

static void print_try(const clap_context_t* ctx){
    EPRINTF("\n"ftitle("Try  ")": ");
    print_breadcrumb(ctx, stderr);
    EPRINTF("--help\n");
}

static void print_usage(const clap_context_t* ctx, char* usage[]){
    if (!usage || !usage[0]){
        printf(ftitle("Usage: "));
        print_breadcrumb(ctx, stdout);
        puts("[-h|--help]\n");
        return;
    }
    if (!usage[1]){
        printf(ftitle("Usage: "));
        print_breadcrumb(ctx, stdout);
        printf("%s\n\n", usage[0]);
        return;
    }
    
    puts(ftitle("Usage:"));
    int i = 0;
    while (usage[i]){
        PRINT_TAB();
        print_breadcrumb(ctx, stdout);
        printf("%s",usage[i]);
        putchar('\n');
        i++;
    }
    putchar('\n');
}

static void help_command(const clap_context_t* ctx){
    clap_command_t* command = ctx->command;
    print_usage(ctx, command->usage);
    printf("%s\n\n", command->description);
    print_command_bits(command);
    print_switches(ctx->app);       
}

static void help_group(const clap_context_t* ctx){
    clap_group_t* group = ctx->group;
    printf(ftitle("Usage: "));
    print_breadcrumb(ctx, stdout);
    puts("[COMMAND]\n");
    printf("%s\n\n", group->description);
    print_parent_bits(group->commands, group->groups);
    print_switches(ctx->app);
}

static void help_app(const clap_context_t* ctx){
    clap_app_t* app = ctx->app;
    printf(fapp" "fvers"\n\n%s\n\n", app->name, app->version, app->description);
    print_usage(ctx, app->usage);
    if (app->main){
        print_command_bits(app->main);
    }
    print_parent_bits(app->commands, app->groups);
    print_switches(ctx->app);
}

void print_help(const clap_context_t* ctx){
    if (!ctx->app){
        return;
    }
    if (ctx->command && ctx->command != ctx->app->main){
        help_command(ctx);
        return;
    }
    if (ctx->group){
        help_group(ctx);
        return;
    }
    help_app(ctx);
}

#ifdef _WIN32
    #include <Windows.h>
    #define SETUP_COLORS() enable_virtual_terminal_processing()

    static void enable_virtual_terminal_processing(){
        HWND out = GetStdHandle(STD_ERROR_HANDLE);
        if (out == INVALID_HANDLE_VALUE){
            return;
        }
        DWORD mode;
        if (!GetConsoleMode(out, &mode)){
            return;
        }
        mode |= 0x0004;
        SetConsoleMode(out, mode);
    }

    static bool file_exists(const char* path, bool* exists){
        DWORD attributes = GetFileAttributesA(path);
        if (attributes == INVALID_FILE_ATTRIBUTES){
            return false;
        }
        *exists = true;
        return !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    static bool directory_exists(const char* path, bool* exists){
        DWORD attributes = GetFileAttributesA(path);
        if (attributes == INVALID_FILE_ATTRIBUTES){
            return false;
        }
        *exists = true;
        return (attributes & FILE_ATTRIBUTE_DIRECTORY);
    }

    static bool path_exists(const char* path){
        DWORD attributes = GetFileAttributesA(path);
        return (attributes == INVALID_FILE_ATTRIBUTES);
    }

#else 
    #include <sys/stat.h>
    #include <stdio.h>
    #define SETUP_COLOR()

    static bool file_exists(const char* path, bool* exists) {
        struct stat attributes;
        if (stat(path, &attributes) != 0) {
            return 0;
        }
        *exists = true;
        return S_ISREG(attributes.st_mode);
    }

    static bool directory_exists(const char* path, bool* exists) {
        struct stat attributes;
        if (stat(path, &attributes) != 0) {
            return 0;
        }
        *exists = true;
        return S_ISDIR(attributes.st_mode);
    }

    static bool path_exists(const char* path) {
        struct stat attributes;
        return stat(path, &attributes) == 0;
    }
#endif

static bool iconvert(void* destination, const char* value, char* name, bool positional, clap_value_t type){
    switch (type){
        case CLAP_INTEGER: {
            char* error;
            long* data = destination;
            *data = strtol(value, &error, 10);
            if (!*error){
                break;
            }
            IF_POSITIONAL(
            PERROR("Positional argument "fpos" expected integer but recieved "fvalue endl, name, value);,
            PERROR("Argument "fargu" expected integer but recieved "fvalue endl, name, value);
            );
            return false;
        }
        case CLAP_FLOAT: {
            char* error;
            double* data = destination;
            *data = strtod(value, &error);
            if (!*error){
                break;
            }
            IF_POSITIONAL(
            PERROR("Positional argument "fpos" expected integer but recieved "fvalue endl, name, value);,
            PERROR("Argument "fargu" expected integer but recieved "fvalue endl, name, value);
            );
            return false;
        }
    }
    return true;
}

static void* mconvert(const char* value, char* name, bool positional, clap_value_t type, bool* ismem){
    void* destination;
    switch (type){
        case CLAP_INTEGER: {
            destination = malloc(sizeof(long));
            break;
        }
        case CLAP_FLOAT: {
            destination = malloc(sizeof(long));
            break;
        }
    }
    if (!destination){
        MEMORY_ERROR();
        *ismem = true;
        return NULL;
    }
    if (iconvert(destination, value, name, positional, type)){
        return destination;
    }
    free(destination);
    return NULL;
}

static const char* ivalidate(const char* value, char* name, bool positional, clap_value_t type){
    switch (type){
        case CLAP_FILE:{
            bool exists = false;
            if (file_exists(value, &exists)){
                return value;
            }
            if (!exists){
                break;
            }
            IF_POSITIONAL(
            PERROR("Path "fvalue" provided for positional argument "fpos" is a directory, not a file" endl, value, name);,
            PERROR("Path "fvalue" provided for argument "fargu" is a directory, not a file" endl, value, name);
            );
            return NULL;
        }
        case CLAP_DIRECTORY:{
            bool exists = false;
            if (directory_exists(value, &exists)){
                return value;
            }
            if (!exists){
                break;
            }
            IF_POSITIONAL(
            PERROR("Path "fvalue" provided for positional argument "fpos" is a file, not a directory." endl, value, name);,
            PERROR("Path "fvalue" provided for argument "fargu" is a file, not a directory" endl, value, name);
            );
            return NULL;
        }
        case CLAP_PATH:{
            bool exists = false;
            if (path_exists(value)){
                return value;
            }
            break;
        }
    }
    IF_POSITIONAL(
    PERROR("Path "fvalue" provided for positional argument "fpos" does not exist" endl, value, name);,
    PERROR("Path "fvalue" provided for argument "fargu" does not exist" endl, value, name);
    );
    return NULL;
}

static char* validate(const char* value, char* name, bool positional, clap_value_t type, bool* ismem){
    char* data = strdup(value);
    if (!data){
        MEMORY_ERROR();
        *ismem = true;
        return NULL;
    }
    if (ivalidate(value, name, positional, type)){
        return data;       
    }
    free(data);
    return NULL;
}

static bool get_one_value(clap_context_t* ctx, void* result[], char* name, slot_t slot, bool positional, clap_value_t type){
    if (ctx->index == ctx->argc){
        goto missing_value_error;
    }
    char* value = ctx->argv[ctx->index];
    if (!ctx->greedy && value[0] == '-' && IS_FLAG_ESCAPE(value)){
        ctx->index ++;
        if (ctx->index == ctx->argc){
            goto missing_value_error;
        }
        value = ctx->argv[ctx->index];
    }
    void* data;
    if (type == CLAP_STRING){
        data = strdup(value);
        if (!data){
            MEMORY_ERROR();
            return true;
        }
    }else if (type < 3){
        bool allocation_failed = false;
        data = mconvert(value, name, positional, type, &allocation_failed);
        if (allocation_failed){
            return true;
        }
        if (!data){
            print_try(ctx);
            return true;
        }
    }else {
        bool allocation_failed = false;
        data = validate(value, name, positional, type, &allocation_failed);
        if (allocation_failed){
            return true;
        }
        if (!data){
            print_try(ctx);
            return true;
        }
    }
    result[slot] = data;
    ctx->index ++;
    ctx->greedy = false;
    return false;
    missing_value_error: {
        IF_POSITIONAL(
            PERROR("Missing value for positional argument "fpos endl, name);,
            PERROR("Missing value for  argument "fargu endl, name);
        );
        print_try(ctx);
        return true;
    }   
}

static void* type_alloc(clap_value_t type, size_t length){
    void* data;
    switch (type){
        CLAP_INTEGER: {
            data = malloc(sizeof(long)* length);
            break;
        }
        CLAP_FLOAT: {
            data = malloc(sizeof(long)* length);
            break;
        }
        default: {
            data = malloc(sizeof(char*)* length);
        }
    }
    if (data){
        return data;
    }
    MEMORY_ERROR();
    return NULL;
}

static bool get_known_length(clap_context_t* ctx, void* result[], char* name, size_t amount,slot_t slot, bool positional, clap_value_t type){
    clap_array_t* prev = result[slot];
    if (prev){
        free(prev->values);
        free(prev);
        prev = NULL;
    }
    clap_array_t* array = malloc(sizeof(array));
    if (!array){
        MEMORY_ERROR();
        return true;
    }
    void** values = type_alloc(type, amount);
    if (!values){
        free(array);
        return true;
    }
    size_t number = 0;
    char* value;
    bool greedy = false;
    while (ctx->index < ctx->argc){
        if (number == amount){
            ctx->index ++;
            break;
        }
        value =  ctx->argv[ctx->index];
        ctx->index ++;
        if (value[0] == '-' && ctx->greedy){
            goto validate_and_set_value;
        }
        if (greedy){
            greedy = false;
            goto validate_and_set_value;
        }
        if (IS_FLAG_ESCAPE(value)){
            if (!number){
                ctx->greedy = true;
                continue;
            }
            greedy = true;
            continue;
        }
        break;
        validate_and_set_value: {
            if (type == CLAP_STRING){
                values[number] = value; 
                number ++;
                continue;
            }else if (type < 3){
                if (!iconvert(&values[number], value, name, positional, type)){
                    goto graceful_exit;
                }
                number ++;
                continue;
            }
            if (!ivalidate(value, name, positional, type)){
                goto graceful_exit;
            }
            values[number] = value;
            number ++;
        }
    }
    if (number != amount){
        IF_POSITIONAL(
            PERROR("Positional argument "fpos" expected "fnum" arguments but recived "fnum endl, name, amount, number);,
            PERROR("Argument "fargu" expected "fnum" arguments but recived "fnum endl, name, amount, number);
        );
        print_try(ctx);
        return true;
    }    
    array->values = values;
    array->length = amount;
    result[slot] = (void*)array;
    ctx->greedy = false;
    return false; 
    graceful_exit:{
        free(array->values);
        free(array);
        print_try(ctx);
        return true;
    }
}
