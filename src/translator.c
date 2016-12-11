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
ObjCode* createNewObjCode(const char* str, int pos);

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
        isImmediate = canBeImmediate(p->op->opcode);
        if(isImmediate && p->opds[2]->type == NUMBER_TYPE)
        else
    }


}
