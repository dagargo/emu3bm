%{
#include <string.h>
#include "sfz.tab.h"
#include "utils.h"

extern int yyerror(const char *msg);
extern int yylval;

%}

%option noyywrap
%option nounput
%option noinput
%option yylineno

%x value
%x string
%%

        /* Some tricks improve the token indentification and simplify the lexer and the parser. */
        /* 1. Spaces are not allowed in opcode expressions but allowed anywhere else. */
        /*    Returning SFZ_SPACE only in the middle of an opcode expression simplifies the grammar. */
        /* 2. As spaces are allowed inside a path, conditionals are used to recognise a SFZ_STRING. */
        /* 3. As there are lots of headers and opcodes and some are custom, there are no tokens for each. */

\r
\n                         { yylineno++; }

\/\/.*                     { }

<value>[[:space:]]+        { return SFZ_SPACE; }
[[:space:]]+               { }

\<[[:alpha:]]+\>           { return SFZ_HEADER; }

[[:alpha:]_]+[[:alnum:]_]* { BEGIN(value); return SFZ_OPCODE; }

<value>=                   { return SFZ_EQUAL; }

<value>[\+\-]?[[:digit:]]*\.[[:digit:]]+ { BEGIN(INITIAL); return SFZ_FLOAT; }
<value>[\+\-]?[[:digit:]]+               { BEGIN(INITIAL); return SFZ_INTEGER; }
<value>[^[:space:]=<>\n]                 { BEGIN(string); yymore(); }
<string>[^=<>\n]*[[:space:]\n]           { BEGIN(INITIAL); yyless(yyleng-1); return SFZ_STRING; }

. { emu_error ("Illegal character %s at line %d", yytext, yylineno); exit (1); }

%%
