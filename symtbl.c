/***********************************************************************************
I pledge my Honor that I have not cheated, and will not cheat, on this assignment. *
Maxat Alibayev.                                                                    *
Assignment 5 LVL 3                                                                 *
************************************************************************************/
#include "symtbl.h"
#include "ast.h"
#include <string.h>
#include <stdio.h>
int internalSTerror();
int typeNameToNumber(char *typeName);
void setupClasses();
void setupFields(ASTree *t, int n);
void setupMethods(ASTree *t, int n);
void setupParams(ASTree *t, int n, int mt);
void setupMethLocals(ASTree *t, int n, int mt);
void setupMainLocals();
void printST();

int internalSTerror()
{
	printError("internalSTerror\n");
}

void setupSymbolTables(ASTree *fullProgramAST)
{
	wholeProgram = fullProgramAST;
	mainExprs = wholeProgram->childrenTail->data;
	setupClasses();
	setupMainLocals();
}

int typeNameToNumber(char *typeName)
{
	if(strcmp(typeName, "Object") == 0)
	{
		return 0;
	}
	if(strcmp(typeName, "nat") == 0)
	{
		return -1;
	}
	if(strcmp(typeName, "null") == 0)
	{
		return -2;
	}
	int i;
	for(i = 1; i < numClasses; i++)
	{
		if(strcmp(typeName, classesST[i].className) == 0)
		{
			return i;
		}
	}
	return -4;

}

void setupClasses()
{
	int i;
	//class decl list
	ASTList *classes = wholeProgram->children->data->children;
	ASTree *class = NULL;
	char *superClassName;
	char *copyStr;

	numClasses = 1;
	while(classes != NULL && classes->data != NULL)
	{
		numClasses++;
		classes = classes->next;
	}
	classesST = calloc(numClasses, sizeof(ClassDecl));
	if(classesST == NULL)
	{
		internalSTerror();
	}
	
	classes = wholeProgram->children->data->children;
	for(i = 1; i < numClasses; i++)
	{
		class = classes->data;

		copyStr = malloc(strlen(class->children->data->idVal)+1);
		if(copyStr==NULL) printError("malloc in newAST()");
		strcpy(copyStr, class->children->data->idVal);
		classesST[i].className = copyStr;
		
		classesST[i].classNameLineNumber = class->lineNumber;
		classes = classes->next;
	}

	classes = wholeProgram->children->data->children;
	for(i = 1; i < numClasses; i++)
	{
		
		class = classes->data;
		superClassName = class->children->next->data->idVal;
		classesST[i].superclass = typeNameToNumber(superClassName);
		if(classesST[i].superclass == 0)
		{
			classesST[i].superclassLineNumber = -1;
		}
		else
		{
			classesST[i].superclassLineNumber = classesST[classesST[i].superclass].classNameLineNumber;
		}
		setupFields(class, i);
		setupMethods(class, i);
		classes = classes->next;
	}
	copyStr = malloc(strlen("Object") + 1);
	if(copyStr == NULL) printError("malloc in newAST()");
	strcpy(copyStr, "Object");
	classesST[0].className = copyStr;
	classesST[0].classNameLineNumber = -1;
	classesST[0].superclass = -3;
}

void setupFields(ASTree *t, int n)
{
	ASTList *fields = t->children->next->next->data->children;
	ASTree *field = NULL;
	classesST[n].numVars = 0;

	while(fields != NULL && fields->data != NULL)
	{
		classesST[n].numVars++;
		fields = fields->next;
	}
	classesST[n].varList = calloc(classesST[n].numVars, sizeof(VarDecl));
	if(classesST[n].varList == NULL)
	{
		internalSTerror();
	}

	int i;
	fields = t->children->next->next->data->children;
	char *copyStr;

	for(i = 0; i < classesST[n].numVars; i++)
	{
		field = fields->data;
		
		copyStr = malloc(strlen(field->childrenTail->data->idVal)+1);
		if(copyStr==NULL) printError("malloc in newAST()");
  		strcpy(copyStr, field->childrenTail->data->idVal);
		classesST[n].varList[i].varName = copyStr;
		classesST[n].varList[i].varNameLineNumber = field->lineNumber;

		classesST[n].varList[i].typeLineNumber = field->children->data->lineNumber;
		fields = fields->next;
	}
	fields = t->children->next->next->data->children;
	for(i = 0; i < classesST[n].numVars; i++)
	{
		field = fields->data;
		classesST[n].varList[i].type = typeNameToNumber(field->children->data->idVal);
		fields = fields->next;
	}
}

void setupMethods(ASTree *t, int n)
{
	ASTList *meths = t->childrenTail->data->children;
	ASTree *meth = NULL;
	classesST[n].numMethods = 0;
	char *copyStr;

	while(meths != NULL && meths->data != NULL)
	{
		classesST[n].numMethods++;
		meths = meths->next;
	}

	classesST[n].methodList = calloc(classesST[n].numMethods, sizeof(MethodDecl));
	if(classesST[n].methodList == NULL)
	{
		internalSTerror();
	}

	meths = t->childrenTail->data->children;
	int i;
	for(i = 0; i < classesST[n].numMethods; i++)
	{
		meth = meths->data;

		copyStr = malloc(strlen(meth->children->next->data->idVal)+1);
		if(copyStr == NULL) printError("malloc in newAST");
		strcpy(copyStr, meth->children->next->data->idVal);
		classesST[n].methodList[i].methodName = copyStr;

		classesST[n].methodList[i].methodNameLineNumber = meth->lineNumber;
		classesST[n].methodList[i].returnType = typeNameToNumber(meth->children->data->idVal);
		classesST[n].methodList[i].returnTypeLineNumber = meth->lineNumber;

		setupParams(meth, n, i);
		setupMethLocals(meth, n, i);

		classesST[n].methodList[i].bodyExprs = meth->childrenTail->data;

		meths = meths->next;
	}
}

void setupParams(ASTree *t, int n, int mt)
{
	ASTList *params = t->children->next->next->data->children;
	ASTree *param = NULL;
	classesST[n].methodList[mt].numParams = 0;

	while(params != NULL && params->data != NULL)
	{
		classesST[n].methodList[mt].numParams++;
		params = params->next;
	}

	classesST[n].methodList[mt].paramST = calloc(classesST[n].methodList[mt].numParams, sizeof(VarDecl));
	if(classesST[n].methodList[mt].paramST == NULL)
	{
		internalSTerror();
	}

	int i;
	params = t->children->next->next->data->children;
	char *copyStr;

	for(i = 0; i < classesST[n].methodList[mt].numParams; i++)
	{
		param = params->data;

		copyStr = malloc(strlen(param->children->next->data->idVal)+1);
		if(copyStr==NULL) printError("malloc in newAST()");
		strcpy(copyStr, param->children->next->data->idVal);
		classesST[n].methodList[mt].paramST[i].varName = copyStr;

		classesST[n].methodList[mt].paramST[i].varNameLineNumber = param->lineNumber;
		classesST[n].methodList[mt].paramST[i].type = typeNameToNumber(param->children->data->idVal);
		classesST[n].methodList[mt].paramST[i].typeLineNumber = param->lineNumber;

		params = params->next;
	}
}

void setupMethLocals(ASTree *t, int n, int mt)
{
	ASTList *locals = t->children->next->next->next->data->children;
	ASTree *local = NULL;
	classesST[n].methodList[mt].numLocals = 0;

	while(locals != NULL && locals->data != NULL)
	{
		classesST[n].methodList[mt].numLocals++;
		locals = locals->next;
	}

	classesST[n].methodList[mt].localST = calloc(classesST[n].methodList[mt].numLocals, sizeof(VarDecl));
	if(classesST[n].methodList[mt].localST == NULL)
	{
		internalSTerror();
	}

	int i;
	locals = t->children->next->next->next->data->children;
	char *copyStr;

	for(i = 0; i < classesST[n].methodList[mt].numLocals; i++)
	{
		local = locals->data;

		copyStr = malloc(strlen(local->children->next->data->idVal)+1);
		if(copyStr==NULL) printError("malloc in newAST()");
		strcpy(copyStr, local->children->next->data->idVal);
		classesST[n].methodList[mt].localST[i].varName = copyStr;

		classesST[n].methodList[mt].localST[i].varNameLineNumber = local->lineNumber;
		classesST[n].methodList[mt].localST[i].type = typeNameToNumber(local->children->data->idVal);
		classesST[n].methodList[mt].localST[i].typeLineNumber = local->lineNumber;

		locals = locals->next;
	}
}

void setupMainLocals()
{
	ASTList *locals = wholeProgram->children->next->data->children;
	ASTree *local = NULL;
	numMainBlockLocals = 0;

	while(locals != NULL && locals->data != NULL)
	{
		numMainBlockLocals++;
		locals = locals->next;
	}
	mainBlockST = calloc(numMainBlockLocals, sizeof(ClassDecl));
	if(mainBlockST == NULL)
	{
		internalSTerror();
	}

	int i;
	locals = wholeProgram->children->next->data->children;
	char *copyStr;
	for(i = 0; i < numMainBlockLocals; i++)
	{
		local = locals->data;

		copyStr = malloc(strlen(local->children->next->data->idVal)+1);
		if(copyStr==NULL) printError("malloc in newAST()");
		strcpy(copyStr, local->children->next->data->idVal);
		mainBlockST[i].varName = copyStr;

		mainBlockST[i].varNameLineNumber = local->lineNumber;
		mainBlockST[i].type = typeNameToNumber(local->children->data->idVal);
		mainBlockST[i].typeLineNumber = local->lineNumber;

		locals = locals->next;
	}
}