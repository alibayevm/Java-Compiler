/***********************************************************************************
I pledge my Honor that I have not cheated, and will not cheat, on this assignment. *
Maxat Alibayev.                                                                    *
Assignment 5 LVL 3                                                                 *
************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ast.h"
#include "typecheck.h"
#include "symtbl.h"

/***************************************************
The function makes the first step of type checking.*
It checks that all classes names are unique.       *
****************************************************/
void check1();
/*******************************************************
The function makes the second step of type checking.   *
It checks that all classes and methods in symbol table *
are valid. It checks that:                             *
a) Superclass types >= 0                               *
b) each class type > its superclass type               *
c) class var fields' types >= -1                       *
d) local var types >= -1                               *
e) method return and param types >= -1                 *
********************************************************/
void check2();
/****************************************************
The function makes the third step of type checking. *
It checks that class hierarchy is acyclic.          *
*****************************************************/
void check3();
/*****************************************************
The function makes the fourth step of type checking. *
It checks that all field names within every class C  *
and C's superclasses are unique.                     *
******************************************************/
void check4();
/*********************************************************************
The function makes the fifth step of type checking.                  *
It checks that for all methods:                                      *
a) method name is unique within defining class                       *
b) methods in superclasses with same name have same signature        *
c) method's parameter and local var names are unique (have no dupls) *
d) expr-list body is well typed and has type that is subtype of the  *
   method's declared return type.                                    *
**********************************************************************/
void check5();
/*******************************************************************
The function makes the sixth step of type checking.                *
It checks that main-block locals' var names are unique (no dupls). *
********************************************************************/
void check6();
/******************************************************
The function makes the seventh step of type checking. *
It checks that main-block expr-list is well-typed.    *
*******************************************************/
void check7();

/******************************************************
The function returns the join of the types t1 and t2. *
*******************************************************/
int join(int t1, int t2);
/**************************************************************
The helper function which returns the type of the identifier. *
***************************************************************/
int idType(ASTree *t, int classContainingExpr, int methodContainingExpr);
/************************************************************
The helper function which is used by check4() function.     *
It checks the uniqueness of the var field in the superclass *
of the class where the variable was declared.               *
*************************************************************/
int field_uniq(int classContainingVar, int var, int currentClass);
/*****************************************************************
The helper function which is used by check5() function.          *
It looks for the methods with the same name in the superclasses. *
If they are found it checks their signatures.                    *
******************************************************************/
int override_meth(int classContainingMeth, int meth, int currentClass);
/********************************************************
The helper function which is used by check5() function. *
It checks the union of the param-list and var-decl-list *
for the variables' name uniqueness.                     *
*********************************************************/
void local_param_uniq(int classContainingVar, int methodContainingVar);


void typecheckProgram(){
	ASTree *t = wholeProgram;
	if(t == NULL){
		printf("ERROR the pointer to AST is invalid (on line %d)\n", t->lineNumber);
		exit(-1);
	}
	if(t->children == NULL || t->children->next == NULL || t->childrenTail == NULL){
		printf("ERROR\n");
		exit(-1);
	}

	check1();
	check2();
	check3();
	check4();
	check5();
	check6();
	check7();
}

int isSubtype(int sub, int super){
	
	//Case for the NULL type
	if(sub == -2 && super != -1){
		return 1;
	}

	int i = 0;
	while(i < numClasses){
		if(sub == super){
			return 1;
		}

		if(sub != 0){
			sub = classesST[sub].superclass;
		}
		else{
			return 0;
		}
		i++;
	}
	return 0;
}

int join(int t1, int t2){
	if(isSubtype(t1, t2)){
		return t2;
	}
	if(isSubtype(t2,t1)){
		return t1;
	}
	return join(classesST[t1].superclass, t2);
}

int typeExpr(ASTree *t, int classContainingExpr, int methodContainingExpr)
{
	ASTree *child1 = NULL;
	ASTree *child2 = NULL;
	ASTree *child3 = NULL;

	if(t == NULL){
		printf("ERROR the pointer to AST is invalid\n");
		exit(-1);
	}
	if(t->children == NULL || t->childrenTail == NULL){
		printf("ERROR\n");
		exit(-1);
	}

	//Expression node may have up to 3 children
	if(t->children->data != NULL){
		child1 = t->children->data;		
		if(t->children->next != NULL){
			child2 = t->children->next->data;
			if(t->children->next->next != NULL){
				child3 = t->children->next->next->data;
			}
		}
	}

	switch(t->typ)
	{
		case EXPR_LIST: return typeExprs(t, classContainingExpr, methodContainingExpr);
		case DOT_METHOD_CALL_EXPR:{	
			int classType;
			classType = typeExpr(child1, classContainingExpr, methodContainingExpr);
			if(child2 == NULL || child3 == NULL){
				printf("Error in DOT_METHOD_CALL_EXPR AST  (on line %d)\n", t->lineNumber);
				exit(-1);
			}
			while(classType > 0){
				int numMeths = classesST[classType].numMethods;
				int i = 0;

				while(i < numMeths){

					if(strcmp(child2->idVal, classesST[classType].methodList[i].methodName) == 0){
						int j = 0;
						int t1;
						int t2;
						MethodDecl meth = classesST[classType].methodList[i];
						ASTList *arguments = child3->children;
						while(j < meth.numParams && arguments != NULL){
							t2 = meth.paramST[j].type;
							j++;
							if(arguments->data == NULL){break;}
							t1 = typeExpr(arguments->data, classContainingExpr, methodContainingExpr);
							if(isSubtype(t1, t2) == 0){
								printf("Wrong argument type (on line %d)\n", t->lineNumber);
								exit(-1);
							}
							arguments = arguments->next;
						}

						if((j != meth.numParams || arguments != NULL) && j != 0){
							printf("Number of arguments is different than expected (on line %d)\n", t->lineNumber);
							exit(-1);
						}
						else{
							if(j == 0){ 
								if(arguments->data != NULL){
									printf("Number of arguments is different than expected (on line %d)\n", t->lineNumber);
									exit(-1);
								}
							}
							child2->staticClassNum = classType;
							child2->staticMemberNum = i; //+ classesST[classType].numVars;
							return meth.returnType;
						}
					}
					i++;
				}
				classType = classesST[classType].superclass;
			}
			printf("Called method doesn't exist (on line %d)\n", t->lineNumber);
			exit(-1);
		}
		case METHOD_CALL_EXPR:
			if(child1 == NULL || child2 == NULL){
				printf("ERROR: invalid AST (on line %d)\n", t->lineNumber);
				exit(-1);
			}
			if(classContainingExpr < 0){
				printf("ERROR: Class containing expression is invalid (on line %d)\n", t->lineNumber);
				exit(-1);
			}

			int i;
			int class = classContainingExpr;
			while(class != 0){

				for(i = 0; i < classesST[class].numMethods; i++){

					if(strcmp(child1->idVal, classesST[class].methodList[i].methodName) == 0){
						int j = 0;
						int t1;
						int t2;
						MethodDecl meth = classesST[class].methodList[i];
						ASTList *arguments = child2->children;

						while(j < meth.numParams && arguments != NULL){
							t2 = meth.paramST[j].type;
							j++;
							if(arguments->data == NULL){break;}
							t1 = typeExpr(arguments->data, classContainingExpr, methodContainingExpr);
							if(isSubtype(t1, t2) == 0){
								printf("Wrong argument type in METHOD_CALL_EXPR (on line %d)\n", t->lineNumber);
								exit(-1);
							}
							arguments = arguments->next;
						}

						if((j != meth.numParams || arguments != NULL) && j != 0){
							printf("Number of arguments and expected parameters is different in METHOD_CALL_EXPR (on line %d)\n", t->lineNumber);
							exit(-1);
						}
						else{
							if(j == 0){ 
								if(arguments->data != NULL){
									printf("Number of arguments and exfnfwnjjbdk\n");
									exit(-1);
								}
							}
							child1->staticClassNum = class;
							child1->staticMemberNum = i;// + classesST[class].numVars;
							return meth.returnType;
						}
					}
				}
				class = classesST[class].superclass;
			}
			printf("Called method doesn't exist (on line %d)\n", t->lineNumber);
			exit(-1);
		case DOT_ID_EXPR:{
			int classType = typeExpr(child1, classContainingExpr, methodContainingExpr);
			int i;

			if(child2 == NULL){
				printf("Error with DOT_ID_EXPR AST (on line %d)\n", t->lineNumber);
				exit(-1);
			}

			while(classType > 0){
				i = 0;
				while(i < classesST[classType].numVars){
					if(strcmp(child2->idVal, classesST[classType].varList[i].varName) == 0){
						child2->staticClassNum = classType;
						child2->staticMemberNum = i;
						return classesST[classType].varList[i].type;
					}
					i++;
				}

				classType = classesST[classType].superclass;
			}
			printf("Undeclared variable on line %d\n", t->lineNumber);
			exit(-1);
		}
		case ID_EXPR:{
			if(child1 == NULL){
				printf("ERROR: invalid AST (on line %d)\n", t->lineNumber);
				exit(-1);
			}
			return idType(child1, classContainingExpr, methodContainingExpr);
		}
		case DOT_ASSIGN_EXPR:{
			int classType = typeExpr(child1, classContainingExpr, methodContainingExpr);
			int i;

			if(child2 == NULL || child3 == NULL){
				printf("Error in DOT_ASSIGN_EXPR AST (on line %d)\n", t->lineNumber);
				exit(-1);
			}

			while(classType > 0){

				for(i = 0; i < classesST[classType].numVars; i++){

					if(strcmp(child2->idVal, classesST[classType].varList[i].varName) == 0){

						int t1 = typeExpr(child3, classContainingExpr, methodContainingExpr);
						int t2 = classesST[classType].varList[i].type;

						if(isSubtype(t1, t2)){
							child2->staticClassNum = classType;
							child2->staticMemberNum = i;
							return t2;
						}
					}
				}
				classType = classesST[classType].superclass;
			}
			printf("Undeclared variable on line %d)\n", t->lineNumber);
			exit(-1);
		}
		case ASSIGN_EXPR:{
			if(child2 == NULL){
				printf("ERROR: invalid AST (on line %d)\n", t->lineNumber);
				exit(-1);
			}
			
			int t1 = typeExpr(child2, classContainingExpr, methodContainingExpr);
			int t2 = idType(child1, classContainingExpr, methodContainingExpr);
			
			if(isSubtype(t1, t2)){
				return t2;
			}
			else{
				printf("ASSIGN_EXPR error: types don't match (on line %d)\n", t->lineNumber);
				exit(-1);
			}
		}
		case PLUS_EXPR:{
			int t1 = typeExpr(child1, classContainingExpr, methodContainingExpr);
			int t2 = typeExpr(child2, classContainingExpr, methodContainingExpr);
			if(t1 == -1 && t2 == -1){
				return -1;
			}
			else{
				printf("NAT types expected in the PLUS_EXPR (on line %d)\n", t->lineNumber);
				exit(-1);
			}
		}
		case MINUS_EXPR:{
			int t1 = typeExpr(child1, classContainingExpr, methodContainingExpr);
			int t2 = typeExpr(child2, classContainingExpr, methodContainingExpr);
			if(t1 == -1 && t2 == -1){
				return -1;
			}
			else{
				printf("NAT types expected in the MINUS_EXPR (on line %d)\n", t->lineNumber);
				exit(-1);
			}
		}
		case TIMES_EXPR:{
			int t1 = typeExpr(child1, classContainingExpr, methodContainingExpr);
			int t2 = typeExpr(child2, classContainingExpr, methodContainingExpr);
			if(t1 == -1 && t2 == -1){
				return -1;
			}
			else{
				printf("NAT types expected in the TIMES_EXPR (on line %d)\n", t->lineNumber);
				exit(-1);
			}
		}
		case EQUALITY_EXPR:{
			int t1 = typeExpr(child1, classContainingExpr, methodContainingExpr);
			int t2 = typeExpr(child2, classContainingExpr, methodContainingExpr);
			if(isSubtype(t1, t2) || isSubtype(t2, t1)){
				return -1;
			}
			else{
				printf("Matched types expected in the EQUALITY_EXPR (on line %d)\n", t->lineNumber);
				exit(-1);
			}
		}
		case LESS_THAN_EXPR:{
			int t1 = typeExpr(child1, classContainingExpr, methodContainingExpr);
			int t2 = typeExpr(child2, classContainingExpr, methodContainingExpr);
			if(t1 == -1 && t2 == -1){
				return -1;
			}
			else{
				printf("NAT types expected in the LESS_THAN_EXPR (on line %d)\n", t->lineNumber);
				exit(-1);
			}
		}
		case NOT_EXPR:{
			int t1 = typeExpr(child1, classContainingExpr, methodContainingExpr);
			if(t1 == -1){
				return -1;
			}
			else{
				printf("NAT type expected in the NOT_EXPR (on line %d)\n", t->lineNumber);
				exit(-1);
			}
		}
		case AND_EXPR:{
			int t1 = typeExpr(child1, classContainingExpr, methodContainingExpr);
			int t2 = typeExpr(child2, classContainingExpr, methodContainingExpr);
			if(t1 == -1 && t2 == -1){
				return -1;
			}
			else{
				printf("NAT types expected in the AND_EXPR (on line %d)\n", t->lineNumber);
				exit(-1);
			}
		}
		case IF_THEN_ELSE_EXPR:{
			int t1 = typeExpr(child1, classContainingExpr, methodContainingExpr);
			int t2 = typeExprs(child2, classContainingExpr, methodContainingExpr);
			int t3 = typeExprs(child3, classContainingExpr, methodContainingExpr);
			if(t1 == -1){
				if(t2 == -1 && t3 == -1){
					return -1;
				}
				else{
					if((t2 >= 0 || t2 == -2) && (t3 >= 0 || t3 == -2)){
						return join(t2, t3);
					}
					else{
						printf("THEN and ELSE branches have mismatch types (on line %d)\n", t->lineNumber);
						exit(-1);
					}
				}
			}
			printf("IF condition must have type NAT (on line %d)\n", t->lineNumber);
			exit(-1);}			
		case WHILE_EXPR:{
			int t1 = typeExpr(child1, classContainingExpr, methodContainingExpr);
			int t2 = typeExprs(child2, classContainingExpr, methodContainingExpr);
			if(t1 == -1 && t2 >= -3){
				return -1;
			}
			else{
				printf("WHILE statement must have type NAT (on line %d)\n", t->lineNumber);
				exit(-1);
			}
		}
		case PRINT_EXPR:{
			if(typeExpr(child1, classContainingExpr, methodContainingExpr) == -1){
				return -1;
			}
			else{
				printf("PRINT_EXPR must have NAT type argument (on line %d)\n", t->lineNumber);
				exit(-1);
			}
		}
		case READ_EXPR: return -1;
		case THIS_EXPR:{
			if(classContainingExpr > 0){
				return classContainingExpr;
			}
			else{
				printf("THIS_EXPR can't be in main method (on line %d)\n", t->lineNumber);
				exit(-1);
			}
		}
		case NEW_EXPR:{
			if(child1 == NULL){
				printf("ERROR ins NEW_EXPR AST (on line %d)\n", t->lineNumber);
				exit(-1);
			}

			int i;
			for(i = 0; i < numClasses; i++){
				if(strcmp(child1->idVal, classesST[i].className) == 0){
					return i;
				}
			}
			printf("Allocating undeclared class object (on line %d)\n", t->lineNumber);
			exit(-1);
		}
		case NULL_EXPR: return -2;
		case NAT_LITERAL_EXPR: return -1;
		default:
			printf("ERROR: Invalid expression (on line %d)\n", t->lineNumber);
			exit(-1);
	}
}

int typeExprs(ASTree *t, int classContainingExpr, int methodContainingExpr)
{
	if(t == NULL){
		printf("ERROR the pointer to AST is invalid (on line %d)\n", t->lineNumber);
		exit(-1);
	}
	if(t->children == NULL || t->childrenTail == NULL){
		printf("ERROR\n");
		exit(-1);
	}

	ASTList *expr_list = t->children;
	ASTree *expr_node;
	int typ;

	while(expr_list != NULL){
		expr_node = expr_list->data;
		if(expr_node == NULL){
			printf("ERROR\n");
			exit(-1);
		}
		typ = typeExpr(expr_list->data, classContainingExpr, methodContainingExpr);
		if(typ < -3){
			printf("22ERROR\n");
			exit(-1);
		}
		expr_list = expr_list->next;
	}
	if(classContainingExpr > 0){
		int t1 = classesST[classContainingExpr].methodList[methodContainingExpr].returnType;
		if (isSubtype(typ, t1)){
			return typ;
		}
		else{
			printf("Method returns a wrong type value (last expression on line %d)\n", expr_node->lineNumber);
			exit(-1);
		}
	}
	return typ;
}

int idType(ASTree *t, int classContainingVar, int methodContainingVar){
	int i = 0;
	int numVarDecls;
	MethodDecl meth;
	ClassDecl class;

	//expression is in main method
	if(classContainingVar < 0){ 
		while(i < numMainBlockLocals){
			if(strcmp(t->idVal, mainBlockST[i].varName) == 0){
				return mainBlockST[i].type;
			}
			i++;
		}
	}
	else{
		meth = classesST[classContainingVar].methodList[methodContainingVar];
		//method locals first
		numVarDecls = meth.numLocals;
		while(i < numVarDecls){
			if(strcmp(t->idVal, meth.localST[i].varName) == 0){
				t->staticClassNum = 0;
				t->staticMemberNum = 0;
				return meth.localST[i].type;
			}
			i++;
		}
		//method params second
		numVarDecls = meth.numParams;
		i = 0;
		while(i < numVarDecls){
			if(strcmp(t->idVal, meth.paramST[i].varName) == 0){
				t->staticClassNum = 0;
				t->staticMemberNum = 0;
				return meth.paramST[i].type;
			}
			i++;
		}
		//class and superclass fields last
		int clType = classContainingVar;
		while(clType != 0){
			class = classesST[clType];
			numVarDecls = class.numVars;
			i = 0;
			while(i < numVarDecls){
				if(strcmp(t->idVal, class.varList[i].varName) == 0)
				{
					t->staticClassNum = clType;
					t->staticMemberNum = i;
					return class.varList[i].type;
				}
				i++;
			}
			clType = class.superclass;
		}
	}
	printf("Undeclared variable (on line %d)\n", t->lineNumber);
	exit(-1);
}

void check1()
{
	int i, j;
	int cLine1, cLine2;
	for(i = 0; i < numClasses; i++){
		for(j = i + 1; j < numClasses; j++){
			if(strcmp(classesST[i].className, classesST[j].className) == 0){
				cLine1 = classesST[i].classNameLineNumber;
				cLine2 = classesST[j].classNameLineNumber;
				printf("Class names have to be unique (on line %d and %d)\n", cLine1, cLine2);
				exit(-1);
			}
		}
	}
}

void check2()
{
	int i, j, k;
	int cLine, scLine, vLine, mLine;
	for (i = 1; i < numClasses; ++i){ //iterate through each class
		// check superclass type
		if(classesST[i].superclass < 0){
			scLine = classesST[i].superclassLineNumber;
			printf("Superclass type has to be >= 0 (on line %d)\n", scLine);
			exit(-1);
		}
		//class type must be != to its superclass type
		if(classesST[i].superclass == i)
		{
			cLine = classesST[i].classNameLineNumber;
			scLine = classesST[i].superclassLineNumber;
			printf("ClassType must be != to its superclassType (on line %d and %d)\n", cLine, scLine);
			exit(-1);
		}
		//check class variable fields
		for(j = 0; j < classesST[i].numVars; j++){
			if(classesST[i].varList[j].type < -1){
				vLine = classesST[i].varList[j].typeLineNumber;
				printf("Variable type has to be >= -1 (on line %d)\n", vLine);
				exit(-1);
			}
		}
		// Iterate thru each method of class i
		for(j = 0; j < classesST[i].numMethods; j++){
			//check the method's locals' type
			for(k = 0; k < classesST[i].methodList[j].numLocals; k++){
				if(classesST[i].methodList[j].localST[k].type < -1){
					vLine = classesST[i].methodList[j].localST[k].typeLineNumber;
					printf("Variable type has to be >= -1 (on line %d)\n", vLine);
					exit(-1);
				}
			}
			//check the method's params' type
			for(k = 0; k < classesST[i].methodList[j].numParams; k++){
				if(classesST[i].methodList[j].paramST[k].type < -1){
					vLine = classesST[i].methodList[j].paramST[k].typeLineNumber;
					printf("Parameter type has to be >= -1 (on line %d)\n", vLine);
					exit(-1);
				}
			}
			//check Method return type
			if(classesST[i].methodList[j].returnType < -1){
				mLine = classesST[i].methodList[j].returnTypeLineNumber;
				printf("Return type has to be >= -1 (on line %d)\n", mLine);
				exit(-1);
			}
		}
	}

	for(i = 0; i < numMainBlockLocals; i++){
		if(mainBlockST[i].type < -1){
			vLine = mainBlockST[i].typeLineNumber;
			printf("Variable type has to be >= -1 (on line %d)\n", vLine);
			exit(-1);
		}
	}
	//class type + superclass type ????
}

void check3()
{
	int i,j;
	int cLine, cLine2;
	for (i = 0; i < numClasses; i++){
		if(i == classesST[i].superclass){
			cLine = classesST[i].classNameLineNumber;
			printf("Class can't be its own superclass (on line %d)\n", cLine);
			exit(-1);
		}
		for (j = i + 1; j < numClasses; ++j){
			if(isSubtype(i, j) && isSubtype(j, i)){
				cLine = classesST[i].classNameLineNumber;
				cLine2 = classesST[j].classNameLineNumber;
				printf("Classes are in cycle (on line %d and %d)\n", cLine, cLine2);
				exit(-1);
			}
		}
	}
}

//Can a class redeclare the variable from its superclass???
void check4()
{
	int i, j, k;
	int vLine1, vLine2;

	for(i = 1; i < numClasses; i++) //iterate through each class
	{
		for(j = 0; j < classesST[i].numVars; j++) //iterate through the var_decl_list of class i
		{
			
			for(k = j + 1; k < classesST[i].numVars; k++)
			{
				if(strcmp(classesST[i].varList[j].varName, classesST[i].varList[k].varName) == 0)
				{
					vLine1 = classesST[i].varList[j].varNameLineNumber;
					vLine2 = classesST[i].varList[k].varNameLineNumber;
					printf("Fields are not unique (on line %d and %d)\n", vLine1, vLine2);
					exit(-1);
				}
			}

			if(classesST[i].superclass != 0)
			{
				field_uniq(i, j, classesST[i].superclass);
			}
		}
	}
}

int field_uniq(int classContainingVar, int var, int currentClass)
{
	if(currentClass == 0)
	{
		return 0;
	}
	int i, j;
	int vLine1, vLine2;
	ClassDecl c1, c2;

	for(i = 0; i < classesST[currentClass].numVars; i++)
	{
		c1 = classesST[currentClass];
		c2 = classesST[classContainingVar];
		if(strcmp(c1.varList[i].varName, c2.varList[var].varName) == 0)
		{
			vLine1 = c1.varList[i].varNameLineNumber;
			vLine2 = c2.varList[var].varNameLineNumber;
			printf("Fields are not unique (on line %d and %d)\n", vLine1, vLine2);
			exit(-1);
		}
	}
	return field_uniq(classContainingVar, var, classesST[currentClass].superclass);
}

void check5()
{
	int i, j, k;
	int mLine1, mLine2;
	MethodDecl m1, m2;
	ASTList *classes;
	ASTList *meths;
	if(wholeProgram->children->data != NULL) //CLASS_DECL_LIST node
	{
		classes = wholeProgram->children->data->children;
	}

	for(i = 1; i < numClasses; i++)
	{
		if(classes->data == NULL)
		{
			printf("ERROR with AST\n");
			exit(-1);
		}
		if(classes->data->childrenTail->data != NULL)
		{
			meths = classes->data->childrenTail->data->children;
		}

		for(j = 0; j < classesST[i].numMethods; j++) //iterate through each method in the class
		{
			if(meths->data == NULL)
			{
				printf("ERROR with AST\n");
				exit(-1);
			}

			for(k = j + 1; k < classesST[i].numMethods; k++)
			{
				m1 = classesST[i].methodList[j];
				m2 = classesST[i].methodList[k];
				if(strcmp(m1.methodName, m2.methodName) == 0)
				{
					mLine1 = m1.methodNameLineNumber;
					mLine2 = m2.methodNameLineNumber;
					printf("Method names are not unique (on line %d and %d)\n", mLine1, mLine2);
					exit(-1);
				}
			}
			if(classesST[i].superclass != 0)
			{
				override_meth(i, j, i);
			}

			local_param_uniq(i, j);
			typeExprs(meths->data->childrenTail->data, i, j);
			meths = meths->next;
		}
		classes = classes->next;
	}
}

int override_meth(int classContainingMeth, int meth, int currentClass)
{
	if(currentClass == 0)
	{
		return 0;
	}

	int i, j;
	int mLine1, mLine2;
	MethodDecl m1, m2;
	for(i = 0; i < classesST[currentClass].numMethods; i++)
	{
		m1 = classesST[currentClass].methodList[i];
		m2 = classesST[classContainingMeth].methodList[meth];

		if(strcmp(m1.methodName, m2.methodName) == 0)
		{
			if((m1.numParams != m2.numParams) || (m1.returnType != m2.returnType))
			{
				mLine1 = m1.methodNameLineNumber;
				mLine2 = m2.methodNameLineNumber;
				printf("Overriden methods have different signatures (on line %d and %d)\n", mLine1, mLine2);
				exit(-1);
			}
			for(j = 0; j < m1.numParams; j++)
			{
				if(m1.paramST[j].type != m2.paramST[j].type)
				{
					mLine1 = m1.methodNameLineNumber;
					mLine2 = m2.methodNameLineNumber;
					printf("Overriden methods have different signatures (on line %d and %d)\n", mLine1, mLine2);
					exit(-1);
				}
			}
		}
	}
	return override_meth(classContainingMeth, meth, classesST[currentClass].superclass);
}

void local_param_uniq(int classContainingVar, int methodContainingVar)
{
	int i,j;
	int vLine1, vLine2;
	MethodDecl meth = classesST[classContainingVar].methodList[methodContainingVar];

	for(i = 0; i < meth.numLocals; i++)
	{
		for(j = i + 1; j < meth.numLocals; j++)
		{
			if(strcmp(meth.localST[i].varName, meth.localST[j].varName) == 0)
			{
				vLine1 = meth.localST[i].varNameLineNumber;
				vLine2 = meth.localST[j].varNameLineNumber;
				printf("Mehod locals are not unique (on line %d and %d)\n", vLine1, vLine2);
				exit(-1);
			}
		}
	}
	for(i = 0; i < meth.numParams; i++)
	{
		for(j = i + 1; j < meth.numParams; j++)
		{
			if(strcmp(meth.paramST[i].varName, meth.paramST[j].varName) == 0)
			{
				vLine1 = meth.paramST[i].varNameLineNumber;
				vLine2 = meth.paramST[j].varNameLineNumber;
				printf("Mehod parameters are not unique (on line %d and %d)\n", vLine1, vLine2);
				exit(-1);
			}
		}
	}
	for(i = 0; i < meth.numLocals; i++)
	{
		for (j = 0; j < meth.numParams; j++)
		{
			if(strcmp(meth.localST[i].varName, meth.paramST[j].varName) == 0)
			{
				vLine1 = meth.localST[i].varNameLineNumber;
				vLine2 = meth.paramST[j].varNameLineNumber;
				printf("Method local redeclare its parameter (on line %d and %d)\n", vLine1, vLine2);
				exit(-1);
			}
		}
	}
}

void check6()
{
	int i, j;
	int vLine1, vLine2;
	for(i = 0; i < numMainBlockLocals; i++)
	{
		for(j = i + 1; j < numMainBlockLocals; j++)
		{
			if(strcmp(mainBlockST[i].varName, mainBlockST[j].varName) == 0)
			{
				vLine1 = mainBlockST[i].varNameLineNumber;
				vLine2 = mainBlockST[j].varNameLineNumber;
				printf("Main method variables are not unique (on line %d and %d)\n", vLine1, vLine2);
				exit(-1);
			}
		}
	}
}

void check7()
{
	if(wholeProgram->childrenTail->data == NULL)
	{
		printf("Main method expression list cannot be empty\n");
		exit(-1);
	}
	typeExprs(wholeProgram->childrenTail->data, -1, 0);
}
