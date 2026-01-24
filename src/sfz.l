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
%%

\r                         { }
\n                         { yylineno++; }

\/\/.*                     { }

[[:space:]]+               { }

\<[[:alpha:]]+\>           { return SFZ_HEADER; }

[[:alpha:]_]+[[:alnum:]_]* { BEGIN(value); return SFZ_OPCODE; }

    /* A string might end with 2 spaces due to the internal string spaces and the ending one. */
    /* Therefore, it is required that all the value rules capture all the trailing spaces.    */
    /* Otherwise, the longest match (string) would apply and "1  " would be read as a string. */

<value>=                                                            { return SFZ_EQUAL; }
<value>[\+\-]?[[:digit:]]*\.[[:digit:]]+[[:space:]]*[[:space:]\r\n] { BEGIN(INITIAL); yyless(yyleng - 1); return SFZ_FLOAT; }
<value>[\+\-]?[[:digit:]]+[[:space:]]*[[:space:]\r\n]               { BEGIN(INITIAL); yyless(yyleng - 1); return SFZ_INTEGER; }
<value>[[:alpha:][:digit:].][^=\r\n]*[[:space:]\r\n]                { BEGIN(INITIAL); yyless(yyleng - 1); return SFZ_STRING; }

<INITIAL,value>. { emu_error ("Illegal character '%s' at line %d", yytext, yylineno); exit (1); }

%%
