%{
//-- don't change *any* of these: if you do, you'll break the compiler.
#include <algorithm>
#include <memory>
#include <cstring>
#include <cdk/compiler.h>
#include <cdk/types/types.h>
#include ".auto/all_nodes.h"
#define LINE                         compiler->scanner()->lineno()
#define yylex()                      compiler->scanner()->scan()
#define yyerror(compiler, s)         compiler->scanner()->error(s)
//-- don't change *any* of these --- END!

#define NIL (new cdk::nil_node(LINE))
%}

%parse-param {std::shared_ptr<cdk::compiler> compiler}

%union {
  //--- don't change *any* of these: if you do, you'll break the compiler.
  YYSTYPE() : type(cdk::primitive_type::create(0, cdk::TYPE_VOID)) {}
  ~YYSTYPE() {}
  YYSTYPE(const YYSTYPE &other) { *this = other; }
  YYSTYPE& operator=(const YYSTYPE &other) { type = other.type; return *this; }

  std::shared_ptr<cdk::basic_type> type;        /* expr type */
  //-- don't change *any* of these --- END!

  int                   i;          /* integer value */
  double                d;          /* double value */

  std::string          *s;          /* symbol name or string literal */

  cdk::basic_node      *node;       /* node pointer */
  cdk::sequence_node   *sequence;   /* sequence nodes */
  cdk::expression_node *expression; /* expression nodes */
  cdk::lvalue_node     *lvalue;     /* lvalue nodes */

  udf::block_node      *block;      /* block nodes */
  std::vector<size_t> *vector;
};

%token <i> tINTEGER
%token <d> tREAL
%token <s> tIDENTIFIER tSTRING

%token tINT_T tREAL_T tPTR_T tSTRING_T tTENSOR_T tVOID_T tAUTO_T
%token tFORWARD tPUBLIC tPRIVATE
%token tIF tELIF tELSE tFOR tBREAK tCONTINUE tRETURN tWRITE tWRITELN tPROCESS tUNLESS
%token tINPUT tNULLPTR tOBJECTS tSIZEOF
%token tCAPACITY tRANK tDIMS tDIM tRESHAPE tCONTRACTION
%token tGE tLE tEQ tNE tAND tOR

%type <type> type void_type
%type <node> decl vardecl fundecl fundef argdecl instr if_false return fordecl
%type <sequence> file decls argdecls opt_instrs instrs opt_vardecls vardecls exprs opt_exprs opt_init_for fordecls
%type <expression> expr opt_init_var integer real operand
%type <lvalue> lvalue
%type <s> string
%type <block> block
%type <vector> dims

%nonassoc tIF
%nonassoc tELIF tELSE

%right '='
%left tOR
%left tAND
%right '~'
%left tNE tEQ
%left '<' tLE tGE '>'
%left '+' '-'
%left '*' '/' '%'
%left tCONTRACTION
%right tUMINUS

%{
//-- The rules below will be included in yyparse, the main parsing function.
%}

%%
file      : /* empty */ { compiler->ast($$ = new cdk::sequence_node(LINE)); }
          | decls       { compiler->ast($$ = $1); }
          ;

decls     : decl       { $$ = new cdk::sequence_node(LINE, $1); }
          | decls decl { $$ = new cdk::sequence_node(LINE, $2, $1); }
          ;

decl      : vardecl ';' { $$ = $1; }
          | fundecl     { $$ = $1; }
          | fundef      { $$ = $1; }
          ;

vardecl   : tPUBLIC      type    tIDENTIFIER   opt_init_var { $$ = new udf::variable_declaration_node(LINE, tPUBLIC, $2, *$3, $4); delete $3; }
          | tFORWARD     type    tIDENTIFIER                { $$ = new udf::variable_declaration_node(LINE, tFORWARD, $2, *$3, nullptr); delete $3; }
          |              type    tIDENTIFIER   opt_init_var { $$ = new udf::variable_declaration_node(LINE, tPRIVATE, $1, *$2, $3); delete $2; }
          | tPUBLIC      tAUTO_T tIDENTIFIER   opt_init_var { $$ = new udf::variable_declaration_node(LINE, tPUBLIC, nullptr, *$3, $4); delete $3; }
          |              tAUTO_T tIDENTIFIER   opt_init_var { $$ = new udf::variable_declaration_node(LINE, tPRIVATE, nullptr, *$2, $3); delete $2; }              
          ;

type      : tINT_T                  { $$ = cdk::primitive_type::create(4, cdk::TYPE_INT); }
          | tREAL_T                 { $$ = cdk::primitive_type::create(8, cdk::TYPE_DOUBLE); }
          | tSTRING_T               { $$ = cdk::primitive_type::create(4, cdk::TYPE_STRING); }
          | tTENSOR_T '<' dims '>'  { $$ = cdk::tensor_type::create(*$3); delete $3; }
          | tPTR_T '<' type '>'     { $$ = cdk::reference_type::create(4, $3); }
          | tPTR_T '<' tAUTO_T  '>' { $$ = cdk::reference_type::create(4, nullptr); }
          ;

dims      : tINTEGER            { $$ = new std::vector<size_t>(); $$->push_back($1); }
          | dims ',' tINTEGER   { $$ = $1; $$->push_back($3); }
          ;

opt_init_var  : /* empty */  { $$ = NULL; }
              | '=' expr     { $$ = $2; }
              ;

fundecl   : tPUBLIC  type      tIDENTIFIER '(' argdecls ')' { $$ = new udf::function_declaration_node(LINE, tPUBLIC, $2, *$3, $5); delete $3; }
          | tFORWARD type      tIDENTIFIER '(' argdecls ')' { $$ = new udf::function_declaration_node(LINE, tFORWARD, $2, *$3, $5); delete $3; }
          |          type      tIDENTIFIER '(' argdecls ')' { $$ = new udf::function_declaration_node(LINE, tPRIVATE, $1, *$2, $4); delete $2; }
          | tPUBLIC  tAUTO_T   tIDENTIFIER '(' argdecls ')' { $$ = new udf::function_declaration_node(LINE, tPUBLIC, nullptr, *$3, $5); delete $3; }
          | tFORWARD tAUTO_T   tIDENTIFIER '(' argdecls ')' { $$ = new udf::function_declaration_node(LINE, tFORWARD, nullptr, *$3, $5); delete $3; }
          |          tAUTO_T   tIDENTIFIER '(' argdecls ')' { $$ = new udf::function_declaration_node(LINE, tPRIVATE, nullptr, *$2, $4); delete $2; }
          | tPUBLIC  void_type tIDENTIFIER '(' argdecls ')' { $$ = new udf::function_declaration_node(LINE, tPUBLIC, $2, *$3, $5); delete $3; }
          | tFORWARD void_type tIDENTIFIER '(' argdecls ')' { $$ = new udf::function_declaration_node(LINE, tFORWARD, $2, *$3, $5); delete $3; }
          |          void_type tIDENTIFIER '(' argdecls ')' { $$ = new udf::function_declaration_node(LINE, tPRIVATE, $1, *$2, $4); delete $2; }
          ;

fundef    : tPUBLIC  type      tIDENTIFIER '(' argdecls ')' block { $$ = new udf::function_definition_node(LINE, tPUBLIC, $2, *$3, $5, $7); delete $3; }
          |          type      tIDENTIFIER '(' argdecls ')' block { $$ = new udf::function_definition_node(LINE, tPRIVATE, $1, *$2, $4, $6); delete $2; }
          | tPUBLIC  tAUTO_T   tIDENTIFIER '(' argdecls ')' block { $$ = new udf::function_definition_node(LINE, tPUBLIC, nullptr, *$3, $5, $7); delete $3; }
          |          tAUTO_T   tIDENTIFIER '(' argdecls ')' block { $$ = new udf::function_definition_node(LINE, tPRIVATE, nullptr, *$2, $4, $6); delete $2; }
          | tPUBLIC  void_type tIDENTIFIER '(' argdecls ')' block { $$ = new udf::function_definition_node(LINE, tPUBLIC, $2, *$3, $5, $7); delete $3; }
          |          void_type tIDENTIFIER '(' argdecls ')' block { $$ = new udf::function_definition_node(LINE, tPRIVATE, $1, *$2, $4, $6); delete $2; }
          ;

argdecls  : /* empty */           { $$ = new cdk::sequence_node(LINE); }
          | argdecl               { $$ = new cdk::sequence_node(LINE, $1); }
          | argdecls ',' argdecl  { $$ = new cdk::sequence_node(LINE, $3, $1); }
          ;

argdecl   : type tIDENTIFIER    { $$ = new udf::variable_declaration_node(LINE, tPRIVATE, $1, *$2, nullptr); delete $2; }
          ;

void_type : tVOID_T   { $$ = cdk::primitive_type::create(0, cdk::TYPE_VOID); }
          ;

block     : '{' opt_vardecls opt_instrs '}'   { $$ = new udf::block_node(LINE, $2, $3); }
          ;

instr     : expr ';'                                                      { $$ = new udf::evaluation_node(LINE, $1); }
          | tWRITE exprs ';'                                              { $$ = new udf::write_node(LINE, $2, false); }
          | tWRITELN exprs ';'                                            { $$ = new udf::write_node(LINE, $2, true); }
          | tBREAK                                                        { $$ = new udf::break_node(LINE); }
          | tCONTINUE                                                     { $$ = new udf::continue_node(LINE); }
          | return                                                        { $$ = $1; }
          | tIF '(' expr ')' instr              %prec tIF                          { $$ = new udf::if_node(LINE, $3, $5); }
          | tIF '(' expr ')' instr if_false                               { $$ = new udf::if_else_node(LINE, $3, $5, $6); }
          | tFOR '(' opt_init_for ';' opt_exprs ';' opt_exprs ')' instr   { $$ = new udf::for_node(LINE, $3, $5, $7, $9); }
          | block                                                         { $$ = $1; }
          | tPROCESS '(' expr ',' expr ',' expr ',' tIDENTIFIER ')' { $$ = new udf::process_node(LINE, $3, $5, $7, *$9); delete $9; }
          | tUNLESS '(' expr ',' expr ',' expr ',' tIDENTIFIER ')' { $$ = new udf::unless_node(LINE, $3, $5, $7, *$9); delete $9; }
          ;

return    : tRETURN      ';'  { $$ = new udf::return_node(LINE, nullptr); }
          | tRETURN expr ';'  { $$ = new udf::return_node(LINE, $2); }
          ;

if_false  : tELIF '(' expr ')' instr           %prec tIF      { $$ = new udf::if_node(LINE, $3, $5); }     
          | tELIF '(' expr ')' instr if_false  %prec tELSE    { $$ = new udf::if_else_node(LINE, $3, $5, $6); }
          | tELSE instr                                       { $$ = $2; }
          ;

opt_init_for  : /* empty */                   { $$ = new cdk::sequence_node(LINE, NIL); }
              | tAUTO_T tIDENTIFIER '=' expr  { $$ = new cdk::sequence_node(LINE, new udf::variable_declaration_node(LINE, tPRIVATE, nullptr, *$2, $4)); delete $2; }
              | fordecls                      { $$ = $1; }
              | exprs                         { $$ = $1; }
              ;

fordecls  : fordecl               { $$ = new cdk::sequence_node(LINE, $1); }
          | fordecls ',' fordecl   { $$ = new cdk::sequence_node(LINE, $3, $1); }
          ;

fordecl   : type tIDENTIFIER '=' expr   { $$ = new udf::variable_declaration_node(LINE, tPRIVATE, $1, *$2, $4); delete $2; }
          ;

opt_instrs: /* empty */ { $$ = NULL; }
          | instrs      { $$ = $1; }
          ;

instrs    : instr         { $$ = new cdk::sequence_node(LINE, $1); }
          | instrs instr  { $$ = new cdk::sequence_node(LINE, $2, $1); }
          ;
          
opt_vardecls  : /* empty */  { $$ = NULL; }
              | vardecls     { $$ = $1; }
              ;

vardecls  : vardecl ';'               { $$ = new cdk::sequence_node(LINE, $1); }    
          | vardecls vardecl ';'  { $$ = new cdk::sequence_node(LINE, $2, $1); }
          ;

opt_exprs : /* empty */  { $$ = new cdk::sequence_node(LINE); }
          | exprs        { $$ = $1; }

exprs     : expr             { $$ = new cdk::sequence_node(LINE, $1); }
          | exprs ',' expr   { $$ = new cdk::sequence_node(LINE, $3, $1); }
          ;

expr      : integer                               { $$ = $1; }
          | real                                  { $$ = $1; }
          | string                                { $$ = new cdk::string_node(LINE, $1); }
          | tNULLPTR                              { $$ = new udf::nullptr_node(LINE); }
          /* Unary exprs */
          | '-' expr %prec tUMINUS                { $$ = new cdk::unary_minus_node(LINE, $2); }
          | '+' expr %prec tUMINUS                { $$ = $2; }
          | '~' expr                              { $$ = new cdk::not_node(LINE, $2); }
          /* Arithmetic exprs */
          | expr '+' expr                         { $$ = new cdk::add_node(LINE, $1, $3); }
          | expr '-' expr                         { $$ = new cdk::sub_node(LINE, $1, $3); }
          | expr '*' expr                         { $$ = new cdk::mul_node(LINE, $1, $3); }
          | expr '/' expr                         { $$ = new cdk::div_node(LINE, $1, $3); }
          | expr '%' expr                         { $$ = new cdk::mod_node(LINE, $1, $3); }
          /* Logical exprs */
          | expr '<' expr                         { $$ = new cdk::lt_node(LINE, $1, $3); }
          | expr '>' expr                         { $$ = new cdk::gt_node(LINE, $1, $3); }
          | expr tGE expr                         { $$ = new cdk::ge_node(LINE, $1, $3); }
          | expr tLE expr                         { $$ = new cdk::le_node(LINE, $1, $3); }
          | expr tNE expr                         { $$ = new cdk::ne_node(LINE, $1, $3); }
          | expr tEQ expr                         { $$ = new cdk::eq_node(LINE, $1, $3); }
          | expr tAND expr                        { $$ = new cdk::and_node(LINE, $1, $3); }
          | expr tOR expr                         { $$ = new cdk::or_node(LINE, $1, $3); }
          /* Others */
          | tIDENTIFIER '(' opt_exprs ')'         { $$ = new udf::function_call_node(LINE, *$1, $3); delete $1; }
          | '(' expr ')'                          { $$ = $2; }
          | tINPUT                                { $$ = new udf::input_node(LINE); }
          | tOBJECTS '(' expr ')'                 { $$ = new udf::objects_node(LINE, $3); }
          | lvalue '?'                            { $$ = new udf::address_node(LINE, $1); }
          | tSIZEOF '(' expr ')'                  { $$ = new udf::sizeof_node(LINE, $3); }
          /* Left values */
          | lvalue                                { $$ = new cdk::rvalue_node(LINE, $1); }
          /* Assignments */
          | lvalue '=' expr                       { $$ = new cdk::assignment_node(LINE, $1, $3); }
          /* Tensor exprs */
          | '[' exprs ']'                         { $$ = new udf::tensor_node(LINE, $2); }
          | operand '.' tCAPACITY                 { $$ = new udf::tensor_capacity_node(LINE, $1); }
          | operand '.' tRANK                     { $$ = new udf::tensor_rank_node(LINE, $1); }
          | operand '.' tDIMS                     { $$ = new udf::tensor_dims_node(LINE, $1); }
          | operand '.' tDIM '(' expr ')'         { $$ = new udf::tensor_dim_node(LINE, $1, $5); }
          | operand '.' tRESHAPE '(' exprs ')'    { $$ = new udf::tensor_reshape_node(LINE, $1, $5); }
          | operand tCONTRACTION operand          { $$ = new udf::tensor_contraction_node(LINE, $1, $3); }
          ;

lvalue    : tIDENTIFIER                 { $$ = new cdk::variable_node(LINE, *$1); delete $1; }
          | operand '@' '(' exprs ')'   { $$ = new udf::tensor_index_node(LINE, $1, $4); }
          | operand '[' expr ']'        { $$ = new udf::index_node(LINE, $1, $3); }
          ;

operand : tIDENTIFIER                       { $$ = new cdk::rvalue_node(LINE, new cdk::variable_node(LINE, *$1)); delete $1; }
          | '(' expr ')'                    { $$ = $2; }
          | tIDENTIFIER '(' opt_exprs ')'   { $$ = new udf::function_call_node(LINE, *$1, $3); delete $1; }
          ;

integer   : tINTEGER    { $$ = new cdk::integer_node(LINE, $1); }
          ;

real      : tREAL   { $$ = new cdk::double_node(LINE, $1); }
          ;

string    : tSTRING           { $$ = $1; }  
          | string tSTRING    { $$ = $1; $$->append(*$2); delete $2; }
          ;

%%
