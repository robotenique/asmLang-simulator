#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/buffer.h"
#include "../include/error.h"
#include "../include/asmtypes.h"
#include "../include/mactypes.h"
#include "../include/opcodes.h"
#include "../include/optable.h"
#include "../include/parser.h"
#include "../include/stable.h"
#include "../include/defaultops.h"
#include "../include/asm.h"

ObjCode* createNewObjCode(Instruction *ins, bool im);

/*
 * Function: canBeImmediate
 * --------------------------------------------------------
 * Check if a given OPERATOR can have an immediate version!
 *
 * @args    OPCODE : The code of the operator
 *
 * @return true if the operator can be immediate, false otherwise
 */
bool canBeImmediate(int OPCODE) {
    int oprLength = 36;
    //List of operators which supports a "Immediate" version
    oprList = {
        LDB, LDBU, LDW, LDWU, LDT, LDTU, LDO, LDOU, STB, STBU, STW, STWU,
        STT, STTU, STO, STOU, ADD, CMP, SUB, SL, MUL, SR, DIV, NEG, ADDU,
        CMPU, SUBU, SLU, MULU, SRU, DIVU, NEGU, AND, OR, XOR, NXOR};
    for(int i = 0; i < oprLength; i++)
        if(OPCODE == oprList[i])
            return true;
    return false;
}



ObjCode* translateToObject(SymbolTable label_table, Instruction *head) {
    Instruction *p;
    bool isImmediate;
    //Create a dummy-head linked list
    ObjCode *h;
    objHEAD = emalloc(sizeof(ObjCode));
    objHEAD->code = estrdup("NULL");
    objHEAD->pos = 0;
    objHEAD->next = 0;
    for(p = head; p; p = p->next) {
        immediate = canBeImmediate(p->op->opcode);
        // Check if we need to change the opCode
        if(immediate && p->opds[2]->type == NUMBER_TYPE)
            objHEAD->next = createNewObjCode(p, true);
        else
    }


}

//TODO: REMOVE DEBUGS
ObjCode* createNewObjCode(Instruction *ins, bool im) {
    ObjCode* obCode;
    obCode = emalloc(sizeof(ObjCode));
    obCode->code = emalloc(9*sizeof(char));
    obCode->pos = ins->pos;
    obCode->next = 0;
    // All immediate operators have 3 operands
    if(canBeImmediate(ins->op->opcode) || im) {
        char* aux = emalloc(3);
        // Print in 'aux' the operator code
        if(im)
            sprintf(aux, "%02x",(ins->op->opcode + 1));
        else
            sprintf(aux, "%02x", ins->op->opcode);
        strcat(obCode->code, aux);
        for(int i = 0; i < 3; i++) {
            switch (ins->opds[i]->type) {
                case REGISTER:
                    break;
                case NUMBER_TYPE:
                    break;
                default:
                    printf("ERROOOOOOOOOOOUUUUUUUUUUUUUUUUUUUUUU\n");
            }
        }

    }
}
