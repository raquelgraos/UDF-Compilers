%option c++ prefix="udf_scanner_" outfile="udf_scanner.cpp"
%option stack noyywrap yylineno 8bit debug
%{ 
// make relevant includes before including the parser's tab file
#include <string>
#include <cdk/ast/sequence_node.h>
#include <cdk/ast/expression_node.h>
#include <cdk/ast/lvalue_node.h>
#include "udf_parser.tab.h"

// don't change this
#define yyerror LexerError
%}

%x X_COMMENT
%x X_STRING X_STRING_TERMINATOR
%x X_HEX_INT
%%
  yydebug=1; set_debug(1);

  /* ====================================================================== */
  /* ====[                         COMMENTS                           ]==== */
  /* ====================================================================== */

"//".*$                  ; /* ignore comments */

"/*"                     yy_push_state(X_COMMENT);
<X_COMMENT>"*/"          yy_pop_state();
<X_COMMENT>"/*"          yy_push_state(X_COMMENT); // nested comments
<X_COMMENT>.|\n          ;

  /* ====================================================================== */
  /* ====[                        Data types                          ]==== */
  /* ====================================================================== */

"int"                  return tINT_T;
"real"                 return tREAL_T;
"ptr"                  return tPTR_T;
"string"               return tSTRING_T;
"tensor"               return tTENSOR_T;
"void"                 return tVOID_T;
"auto"                 return tAUTO_T;

  /* ====================================================================== */
  /* ====[                      Function types                        ]==== */
  /* ====================================================================== */

"forward"              return tFORWARD;
"public"               return tPUBLIC;

  /* ====================================================================== */
  /* ====[                     Expression Operators                   ]==== */
  /* ====================================================================== */

">="                   return tGE; 
"<="                   return tLE; 
"=="                   return tEQ; 
"!="                   return tNE;
"&&"                   return tAND;
"||"                   return tOR;


  /* ====================================================================== */
  /* ====[                         Expressions                        ]==== */
  /* ====================================================================== */

"input"                return tINPUT;
"nullptr"              return tNULLPTR;
"objects"              return tOBJECTS;
"sizeof"               return tSIZEOF;

  /* ====================================================================== */
  /* ====[                        Tensor methods                      ]==== */
  /* ====================================================================== */
"capacity"             return tCAPACITY;
"rank"                 return tRANK;
"dims"                 return tDIMS;
"dim"                  return tDIM;
"reshape"              return tRESHAPE;
"**"                   return tCONTRACTION;

  /* ====================================================================== */
  /* ====[                         Instructions                       ]==== */
  /* ====================================================================== */

"if"                   return tIF; 
"elif"                 return tELIF;
"else"                 return tELSE;
"for"                  return tFOR; 
"break"                return tBREAK;
"continue"             return tCONTINUE;
"return"               return tRETURN;
"write"                return tWRITE;
"writeln"              return tWRITELN;
"process"  return tPROCESS;
"unless"   return tUNLESS;

  /* ====================================================================== */
  /* ====[                         Identifiers                        ]==== */
  /* ====================================================================== */

[A-Za-z][A-Za-z0-9_]*  yylval.s = new std::string(yytext); return tIDENTIFIER;

  /* ====================================================================== */
  /* ====[                           Strings                          ]==== */
  /* ====================================================================== */

\"                          yy_push_state(X_STRING); yylval.s = new std::string("");
<X_STRING>\"                yy_pop_state(); return tSTRING;
<X_STRING>\\\"              *yylval.s += yytext + 1;
<X_STRING>\\\\              *yylval.s += yytext + 1;
<X_STRING>\\t               *yylval.s += '\t';
<X_STRING>\\n               *yylval.s += '\n';
<X_STRING>\\r               *yylval.s += '\r';
<X_STRING>\\0               yy_push_state(X_STRING_TERMINATOR);
<X_STRING>\n                yyerror("Newline inside string");
<X_STRING>\\[[:xdigit:]]{1,2} {
  *yylval.s += (char)(unsigned char)strtoul(yytext + 1, NULL, 16);
}
<X_STRING>\\.               *yylval.s += yytext + 1;
<X_STRING>.                 *yylval.s += yytext;

<X_STRING_TERMINATOR>\"     yy_pop_state(); yy_pop_state(); return tSTRING;
<X_STRING_TERMINATOR>\\\"   ;
<X_STRING_TERMINATOR>\\\\   ;
<X_STRING_TERMINATOR>\n     yyerror("Newline after null character");
<X_STRING_TERMINATOR>.      ;


  /* ====================================================================== */
  /* ====[                         Hexadecimals                       ]==== */
  /* ====================================================================== */

"0x"                        yy_push_state(X_HEX_INT);
<X_HEX_INT>[[:xdigit:]]+    {
                              try {
                                yylval.i = std::stoi(yytext, nullptr, 16);
                                yy_pop_state();
                                return tINTEGER;
                              } catch (std::out_of_range&) {
                                yyerror("Hexadecimal integer out of range");
                              }
                            }
<X_HEX_INT>.|\n             yyerror("Invalid hexadecimal integer"); yy_pop_state();

  /* ====================================================================== */
  /* ====[                        Decimal numbers                     ]==== */
  /* ====================================================================== */

[0-9]+                      {
                              try {
                                yylval.i = std::stoi(yytext, nullptr, 10);
                                return tINTEGER;
                              } catch (std::out_of_range&) {
                                yyerror("Decimal integer out of range");
                              }
                            }
                      

  /* ====================================================================== */
  /* ====[                         Real numbers                       ]==== */
  /* ====================================================================== */
([0-9]+\.[0-9]*|[0-9]*.\[0-9]+)([eE][-+]?[0-9]+)?     {
                                                        try {
                                                          yylval.d = std::stod(yytext);
                                                          return tREAL;
                                                        } catch (std::out_of_range&) {
                                                          yyerror("Real number out of range");
                                                        }
                                                      }
[0-9]+([eE][-+]?[0-9]+)                             {
                                                        try {
                                                          yylval.d = std::stod(yytext);
                                                          return tREAL;
                                                        } catch (std::out_of_range&) {
                                                          yyerror("Real number out of range");
                                                        }
                                                      }


  /* ====================================================================== */
  /* ====[                    (other characters)                      ]==== */
  /* ====================================================================== */

[-()<>=+*/%;{}.,\[\]?~@]       return *yytext;

[ \t\n]+               ; /* ignore whitespace */

.                      yyerror("Unknown character");

  /* ====================================================================== */
  /* ====[                           THE END                          ]==== */
  /* ====================================================================== */

%%