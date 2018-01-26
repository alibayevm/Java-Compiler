/*
I pledge my Honor that I have not cheated, and will not cheat, on this assignment.
Maxat Alibayev.
*/

/* DJ PARSER */

%code provides {
  #include <stdio.h>
  #include <stdlib.h>
  #include "lex.yy.c"
  #include "ast.h"
  #include "symtbl.h"
  #include "typecheck.h"
  #include "codegen.h"

  /* Symbols in this grammar are represented as ASTs */ 
  #define YYSTYPE ASTree *

  /* Declare global AST for entire program */
  ASTree *pgmAST;
  
  /* Function for printing generic syntax-error messages */
  void yyerror(const char *str) {
    printf("Syntax error on line %d at token %s\n",yylineno,yytext);
    printf("Exiting simulator.\n");
    exit(-1);
  }
}

%token MAIN CLASS EXTENDS NATTYPE IF ELSE WHILE
%token PRINTNAT READNAT THIS NEW NUL NATLITERAL 
%token ID ASSIGN PLUS MINUS TIMES EQUALITY LESS
%token AND NOT DOT SEMICOLON COMMA LBRACE RBRACE 
%token LPAREN RPAREN ENDOFFILE


%start pgm

%right ASSIGN
%left AND
%nonassoc EQUALITY LESS
%left PLUS MINUS
%left TIMES
%right NOT
%left DOT 

%%


pgm : class_decl_list MAIN LBRACE var_decl_list expr_list RBRACE ENDOFFILE 
      { 
        $$ = newAST(PROGRAM, $1, 0, NULL, yylineno);
        pgmAST = $$;
        appendToChildrenList($$, $4);
        appendToChildrenList($$, $5);
      
        return 0; 
      }
      ;

class_decl_list : class_decl_list class_decl
                  { appendToChildrenList($$, $2);}
                | 
                  { $$ = newAST(CLASS_DECL_LIST, NULL, 0, NULL, yylineno);}
                ;

class_decl : CLASS id EXTENDS id LBRACE var_decl_list method_decl_list RBRACE
             {
               $$ = newAST(CLASS_DECL, $2, 0, NULL, yylineno);
               appendToChildrenList($$, $4);
               appendToChildrenList($$, $6);
               appendToChildrenList($$, $7);
             }
           | CLASS id EXTENDS id LBRACE var_decl_list method_decl_list_emp RBRACE
             {
               $$ = newAST(CLASS_DECL, $2, 0, NULL, yylineno);
               appendToChildrenList($$, $4);
               appendToChildrenList($$, $6);
               appendToChildrenList($$, $7);
             }
           ;

var_decl_list : var_decl_list var_decl SEMICOLON
                { appendToChildrenList($$, $2);}
              |
                { $$ = newAST(VAR_DECL_LIST, NULL, 0, NULL, yylineno);}
              ;

var_decl : id id
           {
             $$ = newAST(VAR_DECL, $1, 0, NULL, yylineno);
             appendToChildrenList($$, $2);
           }
         | nattyp id
           {
             $$ = newAST(VAR_DECL, $1, 0, NULL, yylineno);
             appendToChildrenList($$, $2);
           }
         ;

method_decl_list_emp : 
                       { $$ = newAST(METHOD_DECL_LIST, NULL, 0, NULL, yylineno);}
                     ;

method_decl_list : method_decl_list method_decl 
                   { appendToChildrenList($$, $2);}
                 | method_decl
                   { $$ = newAST(METHOD_DECL_LIST, $1, 0, NULL, yylineno);}
                 ;

method_decl : id id LPAREN param_decl_list RPAREN LBRACE var_decl_list expr_list RBRACE
              {
                $$ = newAST(METHOD_DECL, $1, 0, NULL, yylineno);
                appendToChildrenList($$, $2);
                appendToChildrenList($$, $4);
                appendToChildrenList($$, $7);
                appendToChildrenList($$, $8);
              }
            | nattyp id LPAREN param_decl_list RPAREN LBRACE var_decl_list expr_list RBRACE
              {
                $$ = newAST(METHOD_DECL, $1, 0, NULL, yylineno);
                appendToChildrenList($$, $2);
                appendToChildrenList($$, $4);
                appendToChildrenList($$, $7);
                appendToChildrenList($$, $8);
              }
            ;

param_decl_list : param_decl_list_ne
                  { $$ = $1;}
                |
                  { $$ = newAST(PARAM_DECL_LIST, NULL, 0, NULL, yylineno);}
                ;

param_decl_list_ne : param_decl_list_ne COMMA param_decl
                     { appendToChildrenList($$, $3);}
                   | param_decl
                     { $$ = newAST(PARAM_DECL_LIST, $1, 0, NULL, yylineno);}
                   ;

param_decl : id id
             {
               $$ = newAST(PARAM_DECL, $1, 0, NULL, yylineno);
               appendToChildrenList($$, $2);
             }
           | nattyp id
             {
               $$ = newAST(PARAM_DECL, $1, 0, NULL, yylineno);
               appendToChildrenList($$, $2);
             }
           ;

expr_list : expr_list expr SEMICOLON
            { appendToChildrenList($$, $2);}
          | expr SEMICOLON
            { $$ = newAST(EXPR_LIST, $1, 0, NULL, yylineno);}
          ;

expr : expr DOT id LPAREN arg_list RPAREN
       {
        $$ = newAST(DOT_METHOD_CALL_EXPR, $1, 0, NULL, yylineno);
        appendToChildrenList($$, $3);
        appendToChildrenList($$, $5);
       }
     | id LPAREN arg_list RPAREN
       {
        $$ = newAST(METHOD_CALL_EXPR, $1, 0, NULL, yylineno);
        appendToChildrenList($$, $3);
       }
     | expr DOT id
       {
        $$ = newAST(DOT_ID_EXPR, $1, 0, NULL, yylineno);
        appendToChildrenList($$, $3);
       }
     | id
       {
        $$ = newAST(ID_EXPR, $1, 0, NULL, yylineno);
       }
     | expr DOT id ASSIGN expr
       {
        $$ = newAST(DOT_ASSIGN_EXPR, $1, 0, NULL, yylineno);
        appendToChildrenList($$, $3);
        appendToChildrenList($$, $5);
       }
     | id ASSIGN expr
       {
        $$ = newAST(ASSIGN_EXPR, $1, 0, NULL, yylineno);
        appendToChildrenList($$, $3);
       }
     | expr PLUS expr
       {
        $$ = newAST(PLUS_EXPR, $1, 0, NULL, yylineno);
        appendToChildrenList($$, $3);
       }
     | expr MINUS expr
       {
        $$ = newAST(MINUS_EXPR, $1, 0, NULL, yylineno);
        appendToChildrenList($$, $3);
       }
     | expr TIMES expr
       {
        $$ = newAST(TIMES_EXPR, $1, 0, NULL, yylineno);
        appendToChildrenList($$, $3);
       }
     | expr EQUALITY expr
       {
        $$ = newAST(EQUALITY_EXPR, $1, 0, NULL, yylineno);
        appendToChildrenList($$, $3);
       }
     | expr LESS expr
       {
        $$ = newAST(LESS_THAN_EXPR, $1, 0, NULL, yylineno);
        appendToChildrenList($$, $3);
       }
     | NOT expr
       {
        $$ = newAST(NOT_EXPR, $2, 0, NULL, yylineno);
       }
     | expr AND expr
       {
        $$ = newAST(AND_EXPR, $1, 0, NULL, yylineno);
        appendToChildrenList($$, $3);
       }
     | IF LPAREN expr RPAREN LBRACE expr_list RBRACE ELSE LBRACE expr_list RBRACE
       {
        $$ = newAST(IF_THEN_ELSE_EXPR, $3, 0, NULL, yylineno);
        appendToChildrenList($$, $6);
        appendToChildrenList($$, $10);
       }
     | WHILE LPAREN expr RPAREN LBRACE expr_list RBRACE
       {
        $$ = newAST(WHILE_EXPR, $3, 0, NULL, yylineno);
        appendToChildrenList($$, $6);
       }
     | PRINTNAT LPAREN expr RPAREN
       {
        $$ = newAST(PRINT_EXPR, $3, 0, NULL, yylineno);
       }
     | READNAT LPAREN RPAREN
       {
        $$ = newAST(READ_EXPR, NULL, 0, NULL, yylineno);
       }
     | THIS
       {
        $$ = newAST(THIS_EXPR, NULL, 0, NULL, yylineno);
       }
     | NEW id LPAREN RPAREN
       {
        $$ = newAST(NEW_EXPR, $2, 0, NULL, yylineno);
       }
     | NUL
       {
        $$ = newAST(NULL_EXPR, NULL, 0, "null", yylineno);
       }
     | NATLITERAL
       {
        int nat = atoi(yytext);
        $$ = newAST(NAT_LITERAL_EXPR, NULL, nat, NULL, yylineno);
       }
     | LPAREN expr RPAREN
       {
        $$ = $2;
       }
     ;

arg_list : arg_list_ne
           { $$ = $1;}
         |
           { $$ = newAST(ARG_LIST, NULL, 0, NULL, yylineno);}
         ;

arg_list_ne : arg_list_ne COMMA expr
              { appendToChildrenList($$, $3);}
            | expr
              { $$ = newAST(ARG_LIST, $1, 0, NULL, yylineno);}
            ;

id : ID
     { $$ = newAST(AST_ID, NULL, 0, yytext, yylineno);}
   ;

nattyp : NATTYPE
         { $$ = newAST(NAT_TYPE, NULL, 0, "nat", yylineno);}
       ;

%%

int main(int argc, char **argv) {
  if((argc!=2) || (strcmp(argv[0], "./dj2dism") != 0)) 
  {
    printf("Usage: dj2dism filename.dj1\n");
    exit(-1);
  }
  int length = strlen(argv[1]);
  if( (argv[1][length - 3] != '.') || (argv[1][length - 2] != 'd') || (argv[1][length - 1] != 'j'))
  {
    printf("%c   %c   %c\n", argv[1][length - 3], argv[1][length - 2], argv[1][length - 1]);
    printf("Usage: dj2dism filename.dj2\n");
    exit(-1);
  }
  
  yyin = fopen(argv[1],"r");
  if(yyin==NULL) {
    printf("ERROR: could not open file %s\n",argv[1]);
    exit(-1);
  }
  
  yyparse();

  setupSymbolTables(pgmAST);
  typecheckProgram();

  char *pgmName = malloc(length+3);
  if(pgmName == NULL) printError("malloc failed in dj.y");
  int i;
  for(i = 0; i < length - 1; i++)
  {
    pgmName[i] = argv[1][i];

  }
  pgmName[length - 1] = 'i';
  pgmName[length] = 's';
  pgmName[length + 1] = 'm'; 
  
  FILE* dismFile;
  dismFile = fopen(pgmName, "w");
  if(dismFile == NULL){
    printf("ERROR: could not open file %s\n", pgmName);
    exit(-1);
  }

  generateDISM(dismFile);
  //printAST(pgmAST);
  fclose(dismFile);
  return 0;
}
