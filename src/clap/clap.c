#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "iclap.h"
#include "imacros.h"

#define console_width get_console_width()
static size_t term_width = 0;

#ifdef _WIN32
    #include <Windows.h>
    #define export __declspec(dllexport)
    size_t get_console_width() {
        if (term_width){
            return term_width;
        }
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_ERROR_HANDLE), &csbi)) {
            term_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            return term_width;
        }
        term_width = 100;
        return term_width; 
    }
#else
    #include <sys/ioctl.h>
    #define export  __attribute__((visibility("default")))
    #include <unistd.h>
    static size_t get_console_width() {
        if (term_width){
            return term_width;
        }
        struct winsize w;
        if (ioctl(STDERR_FILENO, TIOCGWINSZ, &w) == 0) {
            term_width = w.ws_col;
            return term_width;
        }
        term_width = 100;
        return term_width; 
    }
#endif

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
            printf("%s [%s] ",pos->name->value, metadata[pos->type]);
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
    if (!(commands || groups)){
        return;
    }
    printf(ftitle("Command%c:\n"), 
        commands && groups ? 's': (
            commands? QUANTIFIER(commands) :
            groups? QUANTIFIER(groups) : 0
        ));
    unsigned offset = 
        (commands? get_other_offsets(commands, false) : 0) +
        (groups? get_other_offsets(groups, false) : 0);

    if (commands && commands->length){
        for (int i = 0; i < commands->length; i ++){
            clap_command_t* command = AT(commands, i);
            PRINT_TAB();
            if (command->alias){
                printf("%c| ", command->alias);
            }
            printf("%s ", command->name->value);
            bridge_offset(offset, other_flag_offset((void*)command, false));
            print_desc(command->description, offset, console_width);
        }
    }
    if (groups && groups->length){
        for (int i = 0; i < groups->length; i ++){
            clap_group_t* grp = AT(groups, i);
            PRINT_TAB();
            if (grp->alias){
                printf("%c| ", grp->alias);
            }
            printf("%s ", grp->name->value);
            bridge_offset(offset, other_flag_offset((void*)grp, false));
            print_desc(grp->description, offset, console_width);
        }
    }
    putchar('\n');
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

export void print_help(const clap_context_t* ctx){
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

static bool get_unknown_length(clap_context_t* ctx, void* result[], char* name, slot_t slot, bool positional, clap_value_t type){
    clap_array_t* prev = result[slot];
    if (prev){
        free(prev->values);
        free(prev);
        prev = NULL; 
    }
    clap_array_t* array = malloc(sizeof(clap_array_t));
    if (!array){
        MEMORY_ERROR();
        return true;
    }
    bool requires_validation = type > 2;
    size_t amount = 0;
    size_t start = ctx->index;
    bool greedy = false;
    char* value;
    while (ctx->index < ctx->argc){
        value = ctx->argv[ctx->index];
        ctx->index ++;
        if (value[0] != '-' || ctx->greedy){
            amount ++;
            if (!requires_validation){
                continue;
            }
            goto validate_values;
        }else if (greedy){
            greedy = false;
            goto validate_values;
        }else if (IS_FLAG_ESCAPE(value)){
            if (!amount){
                ctx->greedy = true;
                continue;
            }
            greedy = true;
            continue;
        }
        break;
        validate_values: {
            if (!ivalidate(value, name, positional, type)){
                goto graceful_exit;
            }
        }
    }
    if (!amount){
        IF_POSITIONAL(
            PERROR("Positional argument "fpos" expected at least one value but recieved none"endl, name);,
            PERROR("Argument "fargu" expected at least one value but recieved none"endl, name);
        );
        goto graceful_exit;
    }
    void** values = type_alloc(type, amount);
    if (!values){
        MEMORY_ERROR();
        free(array);
        return true;
    }
    size_t true_index = 0;
    for (size_t i = start; i < ctx->index; i ++){
        value = ctx->argv[start+i];
        if (ctx->greedy){
            if (!requires_validation){
                goto convert_values;
            }
            values[true_index] = value;
            true_index++;
            continue;
        }else if (value[0] == '-' && IS_FLAG_ESCAPE(value)){
            continue;
        }
        if (requires_validation){
            values[true_index] = value;
            true_index ++;
            continue;
        }
        convert_values:{
            if (!iconvert(&values[true_index], value, name, positional, type)){
                goto graceful_exit;
            }
            true_index++;
        }
    }
    graceful_exit: {
        free(array->values);
        free(array);
        print_try(ctx);
        return true;        
    }
}

static bool get_values(clap_context_t* ctx, void* result[], char* name, slot_t slot, size_t nargs, bool positional, clap_value_t type){
    if (!nargs){
        return get_unknown_length(ctx, result, name, slot, positional, type);
    }else if (nargs == 1){
        return get_one_value(ctx, result, name, slot, positional, type);
    }
    return get_known_length(ctx, result, name, nargs, slot, positional, type);
}

static void* find_match(clap_string_t* flag, clap_array_t* array, bool is_alias, bool is_flag){
    if (!array){
        return NULL;
    }
    if (!is_alias){
        int x = is_flag? 2: 0;
        for (int i = 0; i < array->length; i ++){
            flag_info_t* other = array->values[i];
            if (string_compare(flag, other->name, x)){
                return (void*)other;
            }
        }
        return NULL;
    }
    for (int i = 0; i < array->length; i ++){
        flag_info_t* other = array->values[i];
        if (other->alias == flag->value[is_flag]){
            return (void*)other;
        }
    }
    return NULL;
}

static int run_command(clap_context_t* ctx){
    clap_command_t* command = ctx->command;
    size_t fields =
        (command->arguments? command->arguments->length : 0) + 
        (command->positionals? command->positionals->length : 0) +
        (command->options? command->options->length : 0) 
    ;
    void** result = calloc(fields, sizeof(void*));
    if (!result){
        MEMORY_ERROR();
        return 1;
    }
    int current_positional = 0;
    int code = 0;
    clap_positional_t* pos = NULL;
    clap_argument_t* arg = NULL;
    clap_option_t* opt = NULL;
    clap_switch_t* sw = NULL;

    while (ctx->index < ctx->argc){
        clap_string_t value = {.value = (char*)ctx->argv[ctx->index], .length=strlen(ctx->argv[ctx->index])};
        if (!ctx->greedy && value.value[0] =='-'){
            if (IS_FLAG_ESCAPE(value.value)){
                ctx->greedy = true;
                ctx->index++;
                continue;
            }
            bool is_alias = value.length == 2;
            if (arg = find_match(&value, command->arguments, is_alias, true)){
                ctx->index++;
                pos = NULL;
                if (code = get_values(ctx, result, arg->flag->value, arg->slot, arg->amount, false, arg->type)){
                    goto graceful_exit;
                }
                continue;
            } else if (opt = find_match(&value, command->options, is_alias, true)){
                result[opt->slot] = (void*)true;
                ctx->index ++;
                continue;        
            } else if (sw = find_match(&value, ctx->app->switches, is_alias, true)){
                if (code = sw->callback(ctx)){
                    goto graceful_exit;
                }
                if (sw->exits){
                    goto graceful_exit;
                }
                ctx->index ++;
                continue;
            } 
            PERROR("Unrecognized option "funkown endl, value.value);
            print_try(ctx);
            code = 1; 
            goto graceful_exit;
            
        }else if (command->positionals && current_positional < command->positionals->length){
            pos = AT(command->positionals, current_positional);
            arg = NULL;
            if (code = get_values(ctx, result, pos->name->value, pos->slot, pos->amount, true, pos->type)){
                goto graceful_exit;
            }
            current_positional++;
            continue;
        }
        if (pos){
            PERROR("Positional argument "fpos" expected "fnum" value%s" endl, pos->name->value, pos->amount, NARGS_QUANTIFIER(pos->amount));
        }else if (arg){
            PERROR("Argument "fargu" expected "fnum" value%s" endl, arg->flag->value, arg->amount, NARGS_QUANTIFIER(arg->amount));
        }else {
            PERROR("Unexpected value \"%s\"\n", value.value);
        }
        print_try(ctx);
        code = 1;
        goto graceful_exit;
    }
    if (command->positionals){
        for (int i = 0; i < command->positionals->length; i ++){
            pos = AT(command->positionals, i);
            if (!pos->required){
                continue;
            }   
            if (!result[pos->slot]){
                PERROR("Missing value%s for positional argument "fpos endl, NARGS_QUANTIFIER(pos->amount), pos->name);
                print_try(ctx);
                code = 1;
                goto graceful_exit;
            }
        }
    }
    if (command->arguments){
        for (int i = 0; i < command->arguments->length; i ++){
            arg = AT(command->arguments, i);
            if (!arg->required){
                continue;
            }   
            if (!result[arg->slot]){
                PERROR("Missing value%s for argument "fargu endl, NARGS_QUANTIFIER(arg->amount), arg->flag->value);
                print_try(ctx);
                code = 1;
                goto graceful_exit;
            }
        }
    }
    clap_command_callback_t callback = command->callback;
    if (!code){
        code = callback(result, ctx->data);
    }
    graceful_exit: {
        for (int i = 0; i < fields; i ++){
            if (!result[i]){
                continue;
            }
            free(result[i]);   
        }
        free(result);
        return code;
    }
}

export int clap_run(clap_app_t* app, size_t argc, const char* argv[], void* data){
    clap_context_t  ctx = {
        .app = app,
        .command = NULL,
        .group = NULL,
        .data = data,
        .index = 1,
        .trace = 0,
        .argc = argc,
        .argv = (char**)argv,
        .greedy = false
    };
    int code = 0;
    while (ctx.index < ctx.argc){
        clap_string_t value = {.value = (char*)ctx.argv[ctx.index], .length=strlen(ctx.argv[ctx.index])};
        if (!ctx.greedy && value.value[0] == '-'){
            if (IS_FLAG_ESCAPE(value.value)){
                ctx.greedy = true;
                ctx.index++;
                continue;
            }
            clap_switch_t* sw;
            if (sw = find_match(&value, app->switches, value.length == 2, true)){
                if (code = sw->callback(&ctx)){
                    break;
                }
                if (sw->exits){
                    return 0;
                }
            }else if (app->main){
                ctx.command = app->main;
                break;
            }else {
                PERROR("Unrecognised option "fuargu endl, value.value);
                goto print_try_and_exit;
            }
        }else if(ctx.greedy && !ctx.trace){
            if (app->main){
                ctx.command = app->main;
                break;
            }
            PERROR("Unexpected value "funkown endl, value.value);
            goto print_try_and_exit;
        }else {
            if (ctx.command = find_match(&value, ctx.group? ctx.group->commands : app->commands ,value.length == 1, false)){
                ctx.trace++;
                ctx.index++;
                break;
            } else if (ctx.group = find_match(&value, ctx.group? ctx.group->groups : app->groups , value.length == 1, false)){
                ctx.trace++;
            } else if (app->main && app->main->positionals && app->main->positionals->length){
                ctx.command = app->main;
                break;
            
            }else {
                if (app->commands && app->commands->length){
                    PERROR("Unrecognised command "fcommand endl, value.value);
                }else {
                    PERROR("Unexpected value "fargu endl, value.value);
                }
                goto print_try_and_exit;
            }
        }
        ctx.index++;
    }
    if (code){
        return code;
    }
    if (!ctx.command){
        if (ctx.group){
            PERROR("Missing command name" endl);
            goto print_try_and_exit;
        }else if (argc == 1) {
            ctx.command = app->main;       
        }
    }
    if (ctx.command){
        return run_command(&ctx);
    }
    print_try_and_exit:{ 
        print_try(&ctx);
        return 1;
    }
}

export int default_help_switch_fn(const clap_context_t* ctx){
    print_help(ctx);
    return 0;
}

export int default_version_switch_fn(const clap_context_t* ctx){
    printf(fapp" "fvers "\n", ctx->app->name, ctx->app->version);
    return 0;
}
