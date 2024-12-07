#if !defined(CLAP_MACROS_H)
#define CLAP_MACROS_H

// Ansi colors and styles. Use to wrap the format string not the values being formated/
#define cblack(str)   "\x1b[30m"str"\x1b[0m"
#define cred(str)     "\x1b[31m"str"\x1b[0m"
#define cgreen(str)   "\x1b[32m"str"\x1b[0m"
#define cyellow(str)  "\x1b[33m"str"\x1b[0m"
#define cblue(str)    "\x1b[34m"str"\x1b[0m"
#define cmagenta(str) "\x1b[35m"str"\x1b[0m"
#define ccyan(str)    "\x1b[36m"str"\x1b[0m"
#define cwhite(str)   "\x1b[37m"str"\x1b[0m"
#define cbold(str)    "\x1b[1m"str"\x1b[0m"
#define cfaint(str)   "\x1b[2m"str"\x1b[0m"
#define citalic(str)  "\x1b[3m"str"\x1b[0m"
#define cunderline(str) "\x1b[4m"str"\x1b[0m"
#define cblink(str)   "\x1b[5m"str"\x1b[0m"
#define creverse(str) "\x1b[7m"str"\x1b[0m"
#define chide(str)    "\x1b[8m"str"\x1b[0m"

// Format specifiers 
#define fpos cbold(cyellow("[%s]")) // Positional arguments..
#define fargu cbold(cyellow("--%s")) // Named arguments
#define fuargu citalic(cblue("%s")) // Unknown Named arguments
#define fcommand cbold(cyellow("%s")) // Command / Command group names.
#define funkown citalic(cblue("\"%s\"")) // Unknown command
#define fnum cbold(cgreen("%lu")) // Numbers
#define fvalue cgreen("\"%s\"") // Values 
#define fapp cbold("%s") // App name
#define fvers cunderline(cbold("%s")) // App version
#define ftitle(fmt) cbold(fmt)  // Some title

#define endl ".\n"
#define TAB "        "

#define QUANTIFIER(n) n->length == 1 ? 0: 's'
#define AT(array, i) array->values[i]
#define PRINT_TAB() printf("    ");
#define EPRINTF(format, ...) fprintf(stderr, format , ##__VA_ARGS__)
#define PERROR(format, ...) fprintf(stderr, cbold("Error")": "format, ##__VA_ARGS__)
#define MEMORY_ERROR() fprintf(stderr, cbold("Error")": Memory Allocation failed!\n")
#define IS_FLAG_ESCAPE(v) (v[1] == '-' && v[2] == 0)
#define NARGS_QUANTIFIER(nargs) nargs? nargs == 1? "": "s" : "(s)"

#define IF_POSITIONAL(a, b) do \
    if(positional) {a} else { b}\
    while(0)

#endif // CLAP_MACROS_H

