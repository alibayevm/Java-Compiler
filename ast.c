/*
I pledge my Honor that I have not cheated, and will not cheat, on this assignment.
Maxat Alibayev
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ast.h"


void printError(char *reason)
{
  printf("AST Error: %s\n", reason);
  exit(-1);
}

ASTree *newAST(ASTNodeType t, ASTree *child, unsigned int natAttribute, 
  char *idAttribute, unsigned int lineNum)
{
  ASTree *toReturn = malloc(sizeof(ASTree));
  if(toReturn==NULL) printError("malloc in newAST()");

  toReturn->typ = t;
  toReturn->lineNumber = lineNum;
  toReturn->natVal = natAttribute;
  toReturn->staticClassNum = 0;
  toReturn->staticMemberNum = 0;
  if(idAttribute == NULL)
  {
    toReturn->idVal = NULL;
  }
  else
  {
    char *copyStr = malloc(strlen(idAttribute)+1);
    if(copyStr==NULL) printError("malloc in newAST()");
    strcpy(copyStr, idAttribute);
    toReturn->idVal = copyStr;
  }

  ASTList *childList = malloc(sizeof(ASTList));
  if(childList==NULL) printError("malloc in newAST()");

  childList->data = child;
  childList->next = NULL;

  toReturn->children = childList;
  toReturn->childrenTail = childList;

  return toReturn;
}

void appendToChildrenList(ASTree *parent, ASTree *newChild)
{
  if(parent==NULL)
  {
    printError("append called with null parent");
  }
  
  if(parent->children==NULL || parent->childrenTail==NULL)
  {
    printError("append called with bad parent");
  }
    
  if(parent->childrenTail->data==NULL) //replace empty tail with new child
  {
    parent->childrenTail->data = newChild;
  }
  else
  { //tail of children is currently nonempty; append new child to list
    ASTList *newList = malloc(sizeof(ASTList));
    if(newList==NULL) printError("malloc in appendAST()");
    newList->data = newChild;
    newList->next = NULL;
    parent->childrenTail->next = newList;
    parent->childrenTail = newList;
  }
}

void printNodeTypeAndAttribute(ASTree *t)
{
  if(t==NULL) return;
  switch(t->typ) {
    case PROGRAM: printf("PROGRAM   (ends on line %d)", t->lineNumber); break;
    case CLASS_DECL_LIST: printf("CLASS_DECL_LIST   (ends on line %d)", t->lineNumber); break;
    case CLASS_DECL: printf("CLASS_DECL   (ends on line %d)", t->lineNumber); break;
    case VAR_DECL_LIST: printf("VAR_DECL_LIST   (ends on line %d)", t->lineNumber); break;
    case VAR_DECL: printf("VAR_DECL   (ends on line %d)", t->lineNumber); break;
    case METHOD_DECL_LIST: printf("METHOD_DECL_LIST   (ends on line %d)", t->lineNumber); break;
    case METHOD_DECL: printf("METHOD_DECL   (ends on line %d)", t->lineNumber); break;
    case PARAM_DECL_LIST: printf("PARAM_DECL_LIST   (ends on line %d)", t->lineNumber); break;
    case PARAM_DECL: printf("PARAM_DECL   (ends on line %d)", t->lineNumber); break;
    case NAT_TYPE: printf("NAT_TYPE   (ends on line %d)", t->lineNumber); break;
    case AST_ID: printf("AST_ID(%s) StaticMember = %d StaticClass = %d (ends on line %d)", t->idVal,t->staticMemberNum, t->staticClassNum,  t->lineNumber); break;
    case EXPR_LIST: printf("EXPR_LIST   (ends on line %d)", t->lineNumber); break;
    case DOT_METHOD_CALL_EXPR: printf("DOT_METHOD_CALL_EXPR   (ends on line %d)", t->lineNumber); break;
    case METHOD_CALL_EXPR: printf("METHOD_CALL_EXPR   (ends on line %d)", t->lineNumber); break;
    case DOT_ID_EXPR: printf("DOT_ID_EXPR   (ends on line %d)", t->lineNumber); break;
    case ID_EXPR: printf("ID_EXPR   (ends on line %d)", t->lineNumber); break;
    case DOT_ASSIGN_EXPR: printf("DOT_ASSIGN_EXPR   (ends on line %d)", t->lineNumber); break;
    case ASSIGN_EXPR: printf("ASSIGN_EXPR   (ends on line %d)", t->lineNumber); break;
    case PLUS_EXPR: printf("PLUS_EXPR   (ends on line %d)", t->lineNumber); break;
    case MINUS_EXPR: printf("MINUS_EXPR   (ends on line %d)", t->lineNumber); break;
    case TIMES_EXPR: printf("TIMES_EXPR   (ends on line %d)", t->lineNumber); break;
    case EQUALITY_EXPR: printf("EQUALITY_EXPR   (ends on line %d)", t->lineNumber); break;
    case LESS_THAN_EXPR: printf("LESS_THAN_EXPR   (ends on line %d)", t->lineNumber); break;
    case NOT_EXPR: printf("NOT_EXPR   (ends on line %d)", t->lineNumber); break;
    case AND_EXPR: printf("AND_EXPR   (ends on line %d)", t->lineNumber); break;
    case IF_THEN_ELSE_EXPR: printf("IF_THEN_ELSE_EXPR   (ends on line %d)", t->lineNumber); break;
    case WHILE_EXPR: printf("WHILE_EXPR   (ends on line %d)", t->lineNumber); break;
    case PRINT_EXPR: printf("PRINT_EXPR   (ends on line %d)", t->lineNumber); break;
    case READ_EXPR: printf("READ_EXPR   (ends on line %d)", t->lineNumber); break;
    case THIS_EXPR: printf("THIS_EXPR   (ends on line %d)", t->lineNumber); break;
    case NEW_EXPR: printf("NEW_EXPR   (ends on line %d)", t->lineNumber); break;
    case NULL_EXPR: printf("NULL_EXPR   (ends on line %d)", t->lineNumber); break;
    case NAT_LITERAL_EXPR: printf("NAT_LITERAL_EXPR(%d)   (ends on line %d)", t->natVal, t->lineNumber); break;
    case ARG_LIST: printf("ARG_LIST   (ends on line %d)", t->lineNumber); break;
    default: printError("unknown node type in printNodeTypeAndAttribute()");
  }
}

void printASTree(ASTree *t, int depth)
{
  if(t==NULL) return;
  printf("%d:",depth);
  int i=0;
  for(; i<depth; i++)
  {
    printf("  ");
  }
  printNodeTypeAndAttribute(t);
  printf("\n");
  //recursively print all children
  ASTList *childListIterator = t->children;
  while(childListIterator!=NULL)
  {
    printASTree(childListIterator->data, depth+1);
    childListIterator = childListIterator->next;
  }
}

void printAST(ASTree *t)
{
  printASTree(t, 0);
}
