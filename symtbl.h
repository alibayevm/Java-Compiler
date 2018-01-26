/* File symtbl.h: Enhanced symbol-table data structures for DJ */

#ifndef SYMTBL_H
#define SYMTBL_H

#include "ast.h"

/* METHOD TO SET UP GLOBALS (INCLUDING SYMBOL TABLES) */
/* Using the parameter AST representing the whole program, allocate
   and build the global symbol-table data.

   NOTE on typing conventions:
   This compiler represents types as integers.
   The DJ type denoted by int i is:
     The ith class declared in the source program, if i > 0
     Object, if i = 0
     nat, if i = -1
     "any-object" (i.e., the type of "null"), if i = -2
     "no-object" (i.e., the type of Object's superclass), if i = -3
     Undefined (i.e., an illegal type), if i < -3
   When i>=0, the symbol table for type (i.e., class) i is
   at classesST[i].

   NOTE on typechecking:
   This method does NOT perform any checks on the symbol tables
   it builds, although the entry for the Object class
   (in classesST[0]) will of course be free of errors.
   E.g., variable/method/class names may appear multiple times
   in the tables, and types may be invalid.
   If the DJ program declares a variable of an undefined type,
   that type will appear as -4 in the symbol table. */
void setupSymbolTables(ASTree *fullProgramAST);

/* HELPER METHOD TO CONVERT TYPE NAMES TO NUMBERS */
/* Returns the number for a given type name.
   Returns: -4 if type does not exist, -1 for nat, 0 for Object,
     1 for first class declared in program,
     2 for 2nd class declared in program, etc.
     (-2 and -3 are reserved for "any-object" and "no-object" types)
   Determines number for given type name using the global
   variable wholeProgram (defined below).  */
int typeNameToNumber(char *typeName);

/* TYPEDEFS FOR ENHANCED SYMBOL TABLES */
/* Encapsulate all information relevant to a DJ variable:
   the variable name, source-program line number on which the variable is
   declared, variable type, and line number on which the type appears. */
typedef struct vdecls {
  char *varName;
  int varNameLineNumber;
  int type;
  int typeLineNumber;
} VarDecl;

/* Encapsulate all information relevant to a DJ method:
   the method name, return type, parameter names, parameter types, 
   local variables, and method body. */
typedef struct mdecls {
  char *methodName;
  int methodNameLineNumber;
  int returnType;
  int returnTypeLineNumber;

  //An array of this method's parameters
  int numParams; //size of the array
  VarDecl *paramST; //the array itself

  //An array of this method's local variables
  int numLocals; //size of the array
  VarDecl *localST; //the array itself

  //The method's executable body
  ASTree *bodyExprs; 
} MethodDecl;

/* Encapsulate all information relevant to a DJ class:
   the class name, superclass, and arrays of information
   about the class's variables and methods. */
typedef struct classinfo {
  char *className;
  int classNameLineNumber;
  int superclass;
  int superclassLineNumber;

  //array of variable-field information--the ith element of 
  //the array encapsulates information about the ith
  //variable field in this class
  int numVars;  //size of the array
  VarDecl *varList; //the array itself

  //array of method information--the ith element of the array
  //encapsulates information about the ith method in this class
  int numMethods;  //size of the array
  MethodDecl *methodList; //the array itself
} ClassDecl;

/* GLOBALS THAT PROVIDE EASY ACCESS TO PARTS OF THE AST */
/* THESE GLOBALS GET SET IN setupSymbolTables */
// The entire program's AST
ASTree *wholeProgram;
     
// The expression list in the main block of the DJ program
ASTree *mainExprs; 

// Array (symbol table) of locals in the main block
int numMainBlockLocals;  //size of the array
VarDecl *mainBlockST;  //the array itself
   
// Array (symbol table) of class declarations
// Note that the minimum array size is 1,
//   due to the always-present Object class
int numClasses;  //size of the array
ClassDecl *classesST;  //the array itself

#endif
