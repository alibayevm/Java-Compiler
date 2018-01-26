/***********************************************************************************
I pledge my Honor that I have not cheated, and will not cheat, on this assignment. *
Maxat Alibayev.                                                                    *
Assignment 6 LVL 3                                                                 *
************************************************************************************/
#include "codegen.h"
#include "ast.h"
#include "typecheck.h"
#include "symtbl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_DISM_ADDR 65535

FILE *fout;

unsigned int labelNumber = 0;


void codeGenExpr(ASTree *t, int classNumber, int methodNumber);
void codeGenExprs(ASTree *expList, int classNumber, int methodNumber);
int getNumObjectFields(int type);
void incSP();
void decSP();
void genPrologue(int classNumber, int methodNumber);
void genEpilogue(int classNumber, int methodNumber);
void genBody(int classNumber, int methodNumber);
void getDynamicMethodInfo(int staticClass, int staticMethod, int dynamicType, 
	int *dynamicClassToCall, int *dynamicMethodToCall, int index);
void genVTable();

void generateDISM(FILE *outputFile)
{
	int i, j;

	fout = outputFile;
	fprintf(fout, "    mov   7   %d   ; initialize FP\n", MAX_DISM_ADDR);
	fprintf(fout, "    mov   6   %d   ; initialize SP\n", MAX_DISM_ADDR);
	fprintf(fout, "    mov   5   1   ; initialize HP\n");
	fprintf(fout, "    mov   0   0   ; allocate stack space for main locals\n");

	for(i = 0; i < numMainBlockLocals; i++)
	{
		fprintf(fout, "    str   6   0   0   ; Var decl\n");
		decSP();
	}

	fprintf(fout, "    mov   0   0   ; BEGIN MAIN BLOCK BODY\n");

	codeGenExprs(wholeProgram->childrenTail->data, -1, 0);

	fprintf(fout, "    hlt   0   ; NORMAL TERMINATION AT END OF MAIN BLOCK\n");

	for(i = 0; i < numClasses; i++)
	{
		for(j = 0; j < classesST[i].numMethods; j++)
		{
			genPrologue(i, j);
			genBody(i, j);
			genEpilogue(i, j);
		}
	}

	genVTable();
}

int getNumObjectFields(int type)
{
	if(type == 0)
	{
		return 0;
	}

	return classesST[type].numVars + getNumObjectFields(classesST[type].superclass);
}

void incSP()
{
	fprintf(fout, "    mov   1   1\n");
	fprintf(fout, "    add   6   6   1   ; SP++\n");
}

void decSP()
{
	fprintf(fout, "    mov   1   1\n");
	fprintf(fout, "    sub   6   6   1   ; SP--\n");
	fprintf(fout, "    blt   5   6   #%d \n", labelNumber);
	fprintf(fout, "    mov   1   77\n");
	fprintf(fout, "    hlt   1   ; out of stack memory\n");
	fprintf(fout, "#%d: mov   0   0\n", labelNumber);

	labelNumber++;
}


void codeGenExpr(ASTree *t, int classNumber, int methodNumber)
{
	ASTree *child1 = NULL;
	ASTree *child2 = NULL;
	ASTree *child3 = NULL;

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
		case EXPR_LIST: 
		{
			codeGenExprs(t, classNumber, methodNumber);
			break;
		}
		case DOT_METHOD_CALL_EXPR:
		{
			int args = 0;
			int ra = labelNumber;
			labelNumber++;
			
			fprintf(fout, "    mov   0   0   ; DOT_METH_CALL %s.%s\n", child1->idVal, child2->idVal);
			fprintf(fout, "    mov   1   #%d\n", ra); 
			fprintf(fout, "    str   6   0   1   ; Push RA onto the stack\n");
			decSP();

			codeGenExpr(child1, classNumber, methodNumber);
			
			fprintf(fout, "    lod   1   6   1\n");
			fprintf(fout, "    lod   2   1   0\n");		
			fprintf(fout, "    blt   0   1   #%d\n", labelNumber);
			fprintf(fout, "    mov   1   88 ; NULL DEREFERENCE  \n");
			fprintf(fout, "    hlt   1\n");
			fprintf(fout, "    #%d: mov   0   0\n", labelNumber);
			labelNumber++;

			
			 
			fprintf(fout, "    mov   1   %d\n", child2->staticClassNum);
			fprintf(fout, "    str   6   0   1   ; push staticClassNum\n");
			decSP();
			fprintf(fout, "    mov   1   %d\n", child2->staticMemberNum);
			fprintf(fout, "    str   6   0   1   ; push staticMethodNum\n");
			decSP();

			
			ASTList *arguments = child3->children;
			while(arguments != NULL)
			{
				if(arguments->data == NULL) {break;}
				codeGenExpr(arguments->data, classNumber, methodNumber);
				args++;
				arguments = arguments->next;
			}

			fprintf(fout, "    mov   1   %d\n", args);
			fprintf(fout, "    str   6   0   1   ; Push #arguments\n");
			decSP();

			fprintf(fout, "    jmp   0   #vTable\n");
			fprintf(fout, "#%d: mov   0   0   ; END OF DOT_METH_CALL\n", ra);
			break;
		}
		case METHOD_CALL_EXPR:
		{
			int args = 0;
			int ra = labelNumber;
			labelNumber++;

			fprintf(fout, "    mov   0   0   ; METH_CALL\n");
			fprintf(fout, "    mov   1   #%d\n", ra);
			fprintf(fout, "    str   6   0   1   ; Push RA onto the stack\n");
			decSP();

			fprintf(fout, "    lod   1   7   -1\n");
			fprintf(fout, "    lod   2   1   0\n");
			fprintf(fout, "    str   6   0   1   ; Push dynamic caller address\n");
			decSP();

			fprintf(fout, "    lod   1   6   1\n");
			fprintf(fout, "    blt   0   1   #%d\n", labelNumber);
			fprintf(fout, "    mov   1   88\n");
			fprintf(fout, "    hlt   1   ; METH_CALL\n");
			fprintf(fout, "    #%d: mov   0   0\n", labelNumber);
			labelNumber++;

			fprintf(fout, "    mov   1   %d\n", child1->staticClassNum);
			fprintf(fout, "    str   6   0   1   ; push staticClassNum\n");
			decSP();
			fprintf(fout, "    mov   1   %d\n", child1->staticMemberNum);
			fprintf(fout, "    str   6   0   1   ; push staticMethodNum\n");
			decSP();

			ASTList *arguments = child2->children;
			while(arguments != NULL)
			{
				if(arguments->data == NULL) {break;}
				codeGenExpr(arguments->data, classNumber, methodNumber);
				args++;
				arguments = arguments->next;
			}
			
			fprintf(fout, "    mov   1   %d\n", args);
			fprintf(fout, "    str   6   0   1   ; Push #arguments\n");
			decSP();
			fprintf(fout, "    jmp   0   #vTable\n");
			fprintf(fout, "#%d: mov   0   0   ; END OF METH_CALL\n", ra);
			break;
		}
		case DOT_ID_EXPR:
		{
			fprintf(fout, "    mov   0   0   ; DOT_ID\n");
			int classNum = classesST[child2->staticClassNum].superclass;
			int memNum = 0;
			while(classNum != 0)
			{
				memNum = memNum + classesST[classNum].numVars;
				classNum = classesST[classNum].superclass;
			}
			// get offset of the id
			memNum = memNum + child2->staticMemberNum + 1;

			codeGenExpr(child1, classNumber, methodNumber); //pushes dynamic caller address

			fprintf(fout, "    lod   1   6   1\n");
			fprintf(fout, "    blt   0   1   #%d\n", labelNumber);
			fprintf(fout, "    mov   1   88   ; NULL DEREFERENCE\n");
			fprintf(fout, "    hlt   1\n");
			fprintf(fout, "    #%d: mov   0   0\n", labelNumber);
			labelNumber++;

			fprintf(fout, "    lod   2   6   1   ; R[2] = dynamic caller address\n");
			fprintf(fout, "    mov   1   %d   ; static member number of %s = %d\n", memNum, child2->idVal, child2->staticMemberNum);
			fprintf(fout, "    sub   2   2   1   ; R[2] = l-value\n");
			fprintf(fout, "    lod   1   2   0   ; R[1] = r-value\n");
			fprintf(fout, "    str   6   1   1   ; Push the r-value\n");
			fprintf(fout, "    mov   0   0   ; END OF DOT_ID\n");
			break;
		}
		case ID_EXPR:
		{
			fprintf(fout, "    mov   0   0   ; ID_EXPR\n");
			int i;
			if(classNumber < 0)
			{
				//main local
				for(i = 0; i < numMainBlockLocals; i++)
				{
					if(strcmp(child1->idVal, mainBlockST[i].varName) == 0)
					{
						//find l-value
						fprintf(fout, "    mov   1   %d\n", i);
						fprintf(fout, "    sub   1   7   1   ; R[1] = l-value\n");
						//push r-value
						fprintf(fout, "    lod   2   1   0   ; R[2] = r-value\n");
						fprintf(fout, "    str   6   0   2   ; Push r-value\n");
						decSP();
					}
				}
			}
			else
			{
				// If it is a local of the given method
				if(child1->staticClassNum == 0 && child1->staticMemberNum == 0)
				{
					for(i = 0; i < classesST[classNumber].methodList[methodNumber].numParams; i++)
					{
						if(strcmp(child1->idVal, classesST[classNumber].methodList[methodNumber].paramST[i].varName) == 0)
						{
							int index = 4 + i;
							//find l-value
							fprintf(fout, "    mov   1   %d\n", index);
							fprintf(fout, "    sub   1   7   1   ; R[1] = l-value\n");
							//push r-value
							fprintf(fout, "    lod   2   1   0   ; R[2] = r-value\n");
							fprintf(fout, "    str   6   0   2   ; Push r-value\n");
							decSP();
						}
					}
					for(i = 0; i < classesST[classNumber].methodList[methodNumber].numLocals; i++)
					{
						if(strcmp(child1->idVal, classesST[classNumber].methodList[methodNumber].localST[i].varName) == 0)
						{
							int index = i + classesST[classNumber].methodList[methodNumber].numParams + 5;
							//find l-value
							fprintf(fout, "    mov   1   %d\n", index);
							fprintf(fout, "    sub   1   7   1   ; R[1] = l-value\n");
							//push r-value
							fprintf(fout, "    lod   2   1   0   ; R[2] = r-value\n");
							fprintf(fout, "    str   6   0   2   ; Push r-value\n");
							decSP();
						}
					}
				}
				//if it is a field of the class called in the classNumber methodNumber
				else
				{
					int memNum = 0;
					int classNum = classesST[child1->staticClassNum].superclass;
					while(classNum != 0)
					{
						memNum = memNum + classesST[classNum].numVars;
						classNum = classesST[classNum].superclass;
					}
					//offset
					memNum = memNum + child1->staticMemberNum + 1;

					fprintf(fout, "    lod   1   7   -1   ; R[1] = dynamic caller address\n");
					fprintf(fout, "    mov   2   %d   ; offset   staticMemNum of %s = %d\n", memNum, child1->idVal, child1->staticMemberNum);
					fprintf(fout, "    sub   1   1   2   ; R[1] = l-value of field\n");
					fprintf(fout, "    lod   2   1   0   ; R[2] = r-value of field\n");
					fprintf(fout, "    str   6   0   2   ; Push r-value\n");
					decSP();
				}
			}
			fprintf(fout, "    mov   0   0   ; END OF ID_EXPR\n");
			break;
		}
		case DOT_ASSIGN_EXPR:
		{
			fprintf(fout, "    mov   0   0   ; DOT_ASSIGN\n");
			int classNum = classesST[child2->staticClassNum].superclass;
			int memNum = 0;

			// get number of all fields inherited by this class
			while(classNum != 0)
			{
				memNum = memNum + classesST[classNum].numVars;
				classNum = classesST[classNum].superclass;
			}
			// get offset of the id
			memNum = memNum + child2->staticMemberNum + 1;

			codeGenExpr(child3, classNumber, methodNumber);
			codeGenExpr(child1, classNumber, methodNumber); //pushes dynamic caller address

			fprintf(fout, "    lod   1   6   1\n");
			fprintf(fout, "    blt   0   1   #%d\n", labelNumber);
			fprintf(fout, "    mov   1   88\n");
			fprintf(fout, "    hlt   1   ; DOT_ASSIGN\n");
			fprintf(fout, "    #%d: mov   0   0\n", labelNumber);
			labelNumber++;

			fprintf(fout, "   lod   2   6   1   ; R[2] = dynamic caller address\n");
			fprintf(fout, "   mov   1   %d\n", memNum);
			fprintf(fout, "   sub   2   2   1   ; R[2] = l-value\n");
			fprintf(fout, "   lod   1   6   2   ; R[1] = result of expr\n");
			fprintf(fout, "   str   2   0   1   ; update the r-value at l-value address\n");

			incSP();
			fprintf(fout, "    mov   0   0   ; END OF DOT_ASSIGN\n");
			break;
		}
		case ASSIGN_EXPR:
		{
			fprintf(fout, "    mov   0   0   ; ASSIGN\n");
			int i;
			codeGenExpr(child2, classNumber, methodNumber);
			if(classNumber < 0)
			{
				//main local
				for(i = 0; i < numMainBlockLocals; i++)
				{
					if(strcmp(child1->idVal, mainBlockST[i].varName) == 0)
					{
						//find l-value
						fprintf(fout, "    mov   1   %d\n", i);
						fprintf(fout, "    sub   1   7   1   ; R[1] = l-value\n");
					}
				}
			}
			else
			{
				// If it is a parameter or local of the given method
				if(child1->staticClassNum == 0 && child1->staticMemberNum == 0)
				{
					for(i = 0; i < classesST[classNumber].methodList[methodNumber].numParams; i++)
					{
						if(strcmp(child1->idVal, classesST[classNumber].methodList[methodNumber].paramST[i].varName) == 0)
						{
							int index = 4 + i;
							//find l-value
							fprintf(fout, "    mov   1   %d\n", index);
							fprintf(fout, "    sub   1   7   1   ; R[1] = l-value\n");
						}
					}
					for(i = 0; i < classesST[classNumber].methodList[methodNumber].numLocals; i++)
					{
						if(strcmp(child1->idVal, classesST[classNumber].methodList[methodNumber].localST[i].varName) == 0)
						{
							int index = i + classesST[classNumber].methodList[methodNumber].numParams + 5;
							//find l-value
							fprintf(fout, "    mov   1   %d\n", index);
							fprintf(fout, "    sub   1   7   1   ; R[1] = l-value\n");
						}
					}
				}
				//if it is a field of the class called in the classNumber methodNumber 
				else
				{
					int memNum = 0;
					int classNum = classesST[child1->staticClassNum].superclass;
					while(classNum != 0)
					{
						memNum = memNum + classesST[classNum].numVars;
						classNum = classesST[classNum].superclass;
					}
					//offset
					memNum = memNum + child1->staticMemberNum + 1;

					fprintf(fout, "    lod   1   7   -1   ; R[1] = dynamic caller address\n");
					fprintf(fout, "    mov   2   %d   ; offset\n", memNum);
					fprintf(fout, "    sub   1   1   2   ; R[1] = l-value of field\n");
				}
			}
			fprintf(fout, "    lod   2   6   1   ; R[2] = result of rhs expr\n");
			fprintf(fout, "    str   1   0   2   ; update the r-value at l-value address\n");
			fprintf(fout, "    mov   0   0   ; END OF ASSIGN\n");
			break;
		}
		case PLUS_EXPR:
		{
			codeGenExpr(child1, classNumber, methodNumber);
			codeGenExpr(child2, classNumber, methodNumber);

			fprintf(fout, "    mov   0   0   ; PLUS_EXPR\n");
			fprintf(fout, "    lod   1   6   2   ; M[SP+2] -> R[1]\n");
			fprintf(fout, "    lod   2   6   1   ; M[SP+1] -> R[2]\n");
			fprintf(fout, "    add   1   1   2   ; R[1] = R[1] + R[2]\n");
			fprintf(fout, "    str   6   2   1   ; R[1] -> M[SP+2] END OF PLUS_EXPR\n");

			incSP();
			break;
		}
		case MINUS_EXPR:
		{
			codeGenExpr(child1, classNumber, methodNumber);
			codeGenExpr(child2, classNumber, methodNumber);

			fprintf(fout, "    mov   0   0   ; MINUS_EXPR\n");
			fprintf(fout, "    lod   1   6   2   ; M[SP+2] -> R[1]\n");
			fprintf(fout, "    lod   2   6   1   ; M[SP+1] -> R[2]\n");
			fprintf(fout, "    sub   1   1   2   ; R[1] = R[1] - R[2]\n");
			fprintf(fout, "    str   6   2   1   ; R[1] -> M[SP+2]\n");

			incSP();
			break;
		}
		case TIMES_EXPR:
		{
			codeGenExpr(child1, classNumber, methodNumber);
			codeGenExpr(child2, classNumber, methodNumber);

			fprintf(fout, "    mov   0   0   ; TIMES_EXPR\n");
			fprintf(fout, "    lod   1   6   2   ; M[SP+2] -> R[1]\n");
			fprintf(fout, "    lod   2   6   1   ; M[SP+1] -> R[2]\n");
			fprintf(fout, "    mul   1   1   2   ; R[1] = R[1] * R[2]\n");
			fprintf(fout, "    str   6   2   1   ; R[1] -> M[SP+2] END OF TIMES_EXPR\n");

			incSP();
			break;
		}
		case EQUALITY_EXPR:
		{
			int equal = labelNumber;
			labelNumber++;

			codeGenExpr(child1, classNumber, methodNumber);
			codeGenExpr(child2, classNumber, methodNumber);

			fprintf(fout, "    mov   0   0   ; EQUALITY_EXPR\n");
			fprintf(fout, "    lod   1   6   2   ; M[SP+2] -> R[1]\n");
			fprintf(fout, "    lod   2   6   1   ; M[SP+1] -> R[2]\n");
			fprintf(fout, "    beq   1   2   #%d   ; R[1] < R[2]\n", equal);
			fprintf(fout, "    str   6   2   0   ; false case\n");
			fprintf(fout, "    jmp   0   #%d   ; jump to the end of operation\n", labelNumber);
			fprintf(fout, "#%d: mov   1   1   ; true case\n", equal);
			fprintf(fout, "    str   6   2   1   ; 1 -> M[SP+2]\n");
			fprintf(fout, "#%d: mov   0   0   ; END OF LESS_THAN_EXPR\n", labelNumber);

			labelNumber++;
			incSP();
			break;
		}
		case LESS_THAN_EXPR:
		{
			int less = labelNumber;
			labelNumber++;
			codeGenExpr(child1, classNumber, methodNumber);
			codeGenExpr(child2, classNumber, methodNumber);

			fprintf(fout, "    mov   0   0   ; LESS_THAN_EXPR\n");
			fprintf(fout, "    lod   1   6   2   ; M[SP+2] -> R[1]\n");
			fprintf(fout, "    lod   2   6   1   ; M[SP+1] -> R[2]\n");
			fprintf(fout, "    blt   1   2   #%d   ; R[1] < R[2] is true\n", less);
			fprintf(fout, "    str   6   2   0   ; false case\n");
			fprintf(fout, "    jmp   0   #%d   ; jump to the end of operation\n", labelNumber);
			fprintf(fout, "#%d: mov   1   1   ; true case\n", less);
			fprintf(fout, "    str   6   2   1   ; 1 -> M[SP+2]\n");
			fprintf(fout, "#%d: mov   0   0   ; END OF LESS_THAN_EXPR\n", labelNumber);

			labelNumber++;
			incSP();
			break;
		}
		case NOT_EXPR:
		{
			int fals = labelNumber;
			labelNumber++;
			codeGenExpr(child1, classNumber, methodNumber);

			fprintf(fout, "    mov   0   0   ; NOT_EXPR\n");
			fprintf(fout, "    lod   1   6   1   ; M[SP+1] -> R[1]\n");
			fprintf(fout, "    beq   1   0   #%d   ; R[1] == R[0]\n", fals);
			fprintf(fout, "    str   6   1   0   ; R[0] -> M[SP+1]\n");
			fprintf(fout, "    jmp   0   #%d   ; jump to the end of operation\n", labelNumber);
			fprintf(fout, "#%d: mov   1   1   ; if R[1] is false\n", fals);
			fprintf(fout, "    str   6   1   1   ; R[1] -> M[SP+1]\n");
			fprintf(fout, "#%d: mov   0   0   ; END OF NOT_EXPR\n", labelNumber);

			labelNumber++;
			break;
		}
		case AND_EXPR:
		{
			int fals = labelNumber;
			labelNumber++;
			codeGenExpr(child1, classNumber, methodNumber);
			codeGenExpr(child2, classNumber, methodNumber);

			fprintf(fout, "    mov   0   0   ; AND_EXPR\n");
			fprintf(fout, "    lod   1   6   2   ; M[SP+2] -> R[1]\n");
			fprintf(fout, "    lod   2   6   1   ; M[SP+1] -> R[2]\n");
			fprintf(fout, "    beq   1   0   #%d   ; R[1] == R[0]\n", fals);
			fprintf(fout, "    beq   2   0   #%d   ; R[2] == R[0]\n", fals);
			fprintf(fout, "    mov   1   1   ; true case\n");
			fprintf(fout, "    str   6   2   1   ; 1 -> M[SP+2]\n");
			fprintf(fout, "    jmp   0   #%d   ; jump to the end of operation\n", labelNumber);
			fprintf(fout, "#%d: str   6   2   0   ; R[0] -> M[SP+2]\n", fals);
			fprintf(fout, "#%d: mov   0   0   ; END OF AND_EXPR\n", labelNumber);

			labelNumber++;
			incSP();
			break;
		}
		case IF_THEN_ELSE_EXPR:
		{
			fprintf(fout, "    mov   0   0   ; IF_THEN_ELSE\n");
			int elseBranch = labelNumber;
			int end = labelNumber + 1;
			labelNumber = labelNumber + 2;
			codeGenExpr(child1, classNumber, methodNumber);

			fprintf(fout, "    lod   1   6   1   ; M[SP+1] -> R[1]\n");
			fprintf(fout, "    beq   1   0   #%d   ; Go to else branch\n", elseBranch);
			
			incSP();
			codeGenExprs(child2, classNumber, methodNumber);

			fprintf(fout, "    jmp   0   #%d   ; jump to the end of operation\n", end);
			fprintf(fout, "#%d: mov   0   0   ; else branch\n", elseBranch);

			incSP();
			codeGenExprs(child3, classNumber, methodNumber);

			fprintf(fout, "#%d: mov   0   0   ; END OF IF_THEN_ELSE_EXPR\n", end);
		
			break;
		}
		case WHILE_EXPR:
		{
			int loop = labelNumber;
			int brek = labelNumber + 1;
			labelNumber = labelNumber + 2;

			fprintf(fout, "#%d: mov   0   0   ; WHILE_EXPR\n", loop);

			codeGenExpr(child1, classNumber, methodNumber);

			fprintf(fout, "    lod   1   6   1   ; M[SP+1] -> R[1]\n");
			fprintf(fout, "    beq   1   0   #%d   ; Break the loop\n", brek);

			incSP();
			codeGenExprs(child2, classNumber, methodNumber);
			incSP();

			fprintf(fout, "    jmp   0   #%d   ; go back to the loop\n", loop);
			fprintf(fout, "#%d: mov   0   0   ; END OF WHILE_EXPR\n", brek);
			break;
		}
		case PRINT_EXPR:
		{
			codeGenExpr(child1, classNumber, methodNumber);

			fprintf(fout, "    lod   1   6   1   ; M[SP+1] -> R[1]\n");
			fprintf(fout, "    ptn   1   ; END OF PRINT_EXPR\n");
			break;
		}
		case READ_EXPR:
		{
			fprintf(fout, "    rdn   1   ; Read -> R[1]\n");
			fprintf(fout, "    str   6   0   1   ; R[1] -> M[SP] END OF READ_EXPR\n");
			decSP();
			break;
		}
		case THIS_EXPR:
		{
			fprintf(fout, "    lod   1   7   -1\n");
			fprintf(fout, "    str   6   0   1   ; M[SP] = dynamic caller address\n");
			decSP();
			break;
		}
		case NEW_EXPR:
		{
			int i, n, type;
			for(i = 0; i < numClasses; i++){
				if(strcmp(child1->idVal, classesST[i].className) == 0){
					type = i;
					break;
				}
			}

			n = getNumObjectFields(type) + 1;

			fprintf(fout, "    mov   1   %d   ; n+1 -> R[1]\n", n);
			fprintf(fout, "    mov   2   %d   ; MAX_DISM_ADDR -> R[1]\n", MAX_DISM_ADDR);
			fprintf(fout, "    add   1   1   5   ; R[1] -> n + 1 + HP\n");
			fprintf(fout, "    blt   1   2   #%d   ; check HP + n + 1 < MAX_DISM_ADDR\n", labelNumber);
			fprintf(fout, "    mov   1   66   ; error code 66 => insufficient heap memory\n");
			fprintf(fout, "    hlt   1   ; HP + n + 1 >= MAX_DISM_ADDR\n");
			fprintf(fout, "#%d: mov   0   0\n", labelNumber);

			labelNumber++;
			n--;
			fprintf(fout, "    mov   1   1\n");
			for(i = 0; i < n; i++)
			{
				fprintf(fout, "    str   5   0   0   ; 0 -> M[HP]\n");
				fprintf(fout, "    add   5   5   1   ; HP++\n");
			}

			fprintf(fout, "    mov   2   %d   ; type# -> R[2]\n", type);
			fprintf(fout, "    str   5   0   2   ; type# -> M[HP]\n");
			fprintf(fout, "    str   6   0   5   ; type# addr -> M[SP]\n");
			fprintf(fout, "    mov   1   1\n");
			fprintf(fout, "    add   5   5   1   ; HP++ END OF NEW_EXPR\n");
			decSP();
			break;
		}
		case NULL_EXPR:
		{
			fprintf(fout, "    str   6   0   0   ; R[0] -> M[SP] END OF NULL_EXPR\n");
			decSP();
			break;
		}
		case NAT_LITERAL_EXPR:
		{
			fprintf(fout, "    mov   1   %d   ; NAT_LITERAL_EXPR\n", t->natVal);
			fprintf(fout, "    str   6   0   1   ; R[1] -> M[SP] END OF NAT_LITERAL_EXPR\n");
			decSP();
			break;
		}
		default:
		{
			printf("ERROR\n");
			exit(-1);
		}
	}
}

void codeGenExprs(ASTree *expList, int classNumber, int methodNumber)
{
	ASTList *list = expList->children;
	ASTree *expr;

	while(list != NULL)
	{	
		expr = list->data;
		codeGenExpr(expr, classNumber, methodNumber);
		incSP();
		list = list->next;
	}
	decSP();
}

void genPrologue(int classNumber, int methodNumber)
{
	int i;
	MethodDecl m = classesST[classNumber].methodList[methodNumber];
	int newFP = 6 + m.numParams + m.numLocals;

	fprintf(fout, "#CLASS%dMETHOD%dPro: mov   0   0   ; prologue\n", classNumber, methodNumber);
	
	for(i = 0; i < classesST[classNumber].methodList[methodNumber].numLocals; i++)
	{
		fprintf(fout, "    str   6   0   0\n");
		decSP();
	}

	fprintf(fout, "    str   6   0   7   ; Push oldFP\n");
	decSP();

	fprintf(fout, "    mov   1   %d\n", newFP);
	fprintf(fout, "    add   7   1   6   ; update FP\n");
}

void genEpilogue(int classNumber, int methodNumber)
{
	fprintf(fout, "    mov   0   0   ; #CLASS%dMETHOD%dEp: \n", classNumber, methodNumber);
	
	fprintf(fout, "    lod   3   7   0   ; load RA\n");
	fprintf(fout, "    add   1   7   0   ; get newFP value\n");
	fprintf(fout, "    lod   7   6   2   ; get oldFP\n");
	fprintf(fout, "    lod   2   6   1   ; load the method result\n");
	fprintf(fout, "    add   6   1   0   ; set SP to the original frame\n");
	fprintf(fout, "    str   6   0   2   ; store the result of method\n");
	decSP();
	fprintf(fout, "    jmp   3   0\n"); 
}

void genBody(int classNumber, int methodNumber)
{
	ASTList *list = wholeProgram->children->data->children;  //class decl list
	int i;
	ASTree *class;
	ASTree *meth;
	ASTree *exprs;
	for(i = 1; i < classNumber; i++)
	{
		list = list->next;
	}
	class = list->data; //class decl
	meth = class->childrenTail->data; //meth decl list
	list = meth->children;
	for(i = 0; i < methodNumber; i++)
	{
		list = list->next;
	}
	meth = list->data; //meth decl
	list = meth->childrenTail;
	exprs = list->data;
	
	fprintf(fout, "    mov   0   0   ; #CLASS%dMETHOD%dBody\n", classNumber, methodNumber);
	codeGenExprs(exprs, classNumber, methodNumber);
}

void getDynamicMethodInfo(int staticClass, int staticMethod, int dynamicType, 
	int *dynamicClassToCall, int *dynamicMethodToCall, int index)
{

	fprintf(fout, "#STclass%dMETHOD%dDclass%d: mov   0   0\n", staticClass, staticMethod, dynamicType);
	fprintf(fout, "    jmp   0   #CLASS%dMETHOD%dPro\n", dynamicClassToCall[index], dynamicMethodToCall[index]);
}

void genVTable()
{
	int dt, st;
	int i, j;
	int k = 0;
	char *sm;
	char *dm;
	bool found = false;
	int tblSize = 0;

	for(i = 0; i < numClasses; i++)
	{
		for(j = 0; j < numClasses; j++)
		{
			if(isSubtype(j, i))
			{
				tblSize = tblSize + classesST[i].numMethods;
			}
		}
	}

	int dynamicClassToCall[tblSize];
	int dynamicMethodToCall[tblSize];


	for(st = 0; st < numClasses; st++)
	{
		for(i = 0; i < classesST[st].numMethods; i++)
		{
			for(dt = 0; dt < numClasses; dt++)
			{
				if(isSubtype(dt, st))
				{
					for(j = 0; j < classesST[dt].numMethods; j++)
					{
						sm = classesST[st].methodList[i].methodName;
						dm = classesST[dt].methodList[j].methodName;
						if(strcmp(sm, dm) == 0)
						{
							found = true;
							dynamicClassToCall[k] = dt;
							dynamicMethodToCall[k] = j;
							k++;
							break;
						}
					}
					if(!found)
					{
						dynamicClassToCall[k] = st;
						dynamicMethodToCall[k] = i;
						k++;
					}
					found = false;
				}
			}
		}
	}

	k = 0;

	for(st = 0; st < numClasses; st++)
	{
		for(i = 0; i < classesST[st].numMethods; i++)
		{
			for(dt = 0; dt < numClasses; dt++)
			{
				if(isSubtype(dt, st))
				{
					getDynamicMethodInfo(st, i, dt, dynamicClassToCall, dynamicMethodToCall, k);
					k++;
				}
			}
		}
	}

	fprintf(fout, "#vTable: mov   0   0   ; vTable\n");
	fprintf(fout, "    lod   4   6   1   ; R[4] = num of args\n");
	fprintf(fout, "    mov   2   2\n");
	fprintf(fout, "    add   4   4   2\n");
	fprintf(fout, "    add   4   4   6   ; R[4] = R[4] + R[SP]\n");
	fprintf(fout, "    lod   2   4   0   ; R[3] = static meth #\n");
	fprintf(fout, "    lod   1   4   1   ; R[1] = static class #\n");
	fprintf(fout, "    lod   4   4   2   ; R[4] = M[ R[4] + 2]\n");
	fprintf(fout, "    lod   3   4   0   ; R[2] = dynamic class\n");

	for(st = 0; st < numClasses; st++)
	{
		fprintf(fout, "    mov   4   %d\n", st);
		fprintf(fout, "    beq   1   4   #STclass%d\n", st);
	}
	for(st = 0; st < numClasses; st++)
	{
		fprintf(fout, "#STclass%d: mov   0   0\n", st);

		for(i = 0; i < classesST[st].numMethods; i++)
		{
			fprintf(fout, "    mov   4   %d\n", i);
			fprintf(fout, "    beq   2   4   #STclass%dMETHOD%d\n", st, i);
		}
	}

	for(st = 0; st < numClasses; st++)
	{
		for(i = 0; i < classesST[st].numMethods; i++)
		{
			fprintf(fout, "#STclass%dMETHOD%d: mov   0   0\n", st, i);			
			for(dt = 0; dt < numClasses; dt++)
			{
				if(isSubtype(dt, st))
				{
					fprintf(fout, "    mov   4   %d\n", dt);
					fprintf(fout, "    beq   3   4   #STclass%dMETHOD%dDclass%d\n", st, i, dt);
				}
			}
		}
	}
}
