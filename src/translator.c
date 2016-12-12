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

ObjCode* createNewObjCode(Instruction *ins, bool im, SymbolTable label_table);
ObjCode* getJumpObj(Instruction *ins, SymbolTable label_table);

/*
 * Function: isJump
 * --------------------------------------------------------
 * Check if a given OPERATOR is a jump operator
 *
 * @args    OPCODE : The code of the operator
 *
 * @return true if the operator is a jump operator, false otherwise
 */
bool isJump(int OPCODE) {
    int jmpLen = 9;
    //List of operators which supports a "Immediate" version
    int jmpList[] = {JMP, JZ, JNZ, JP, JN, JNN, JNP, GO, GETA};
    for(int i = 0; i < jmpLen; i++)
        if(OPCODE == jmpList[i])
            return true;
    return false;
}

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
    int oprList[] = {
        LDB, LDBU, LDW, LDWU, LDT, LDTU, LDO, LDOU, STB, STBU, STW, STWU,
        STT, STTU, STO, STOU, ADD, CMP, SUB, SL, MUL, SR, DIV, NEG, ADDU,
        CMPU, SUBU, SLU, MULU, SRU, DIVU, NEGU, AND, OR, XOR, NXOR};
    for(int i = 0; i < oprLength; i++)
        if(OPCODE == oprList[i])
            return true;
    return false;
}

/*
 * Function: isSpecial
 * --------------------------------------------------------
 * Check if a given operand is a special one =)
 *
 * @args    OPCODE : The code of the operator
 *
 * @return true if the operator is Special, false otherwise
 */
bool isSpecial(int OPCODE) {
    int spcLen = 5;
    int specialOpr[] = { SETW, SAVE, REST, INT, NOP};
    for(int i = 0; i < spcLen; i++)
        if(OPCODE == specialOpr[i])
            return true;
    return false;
}


ObjCode* translateToObject(SymbolTable label_table, Instruction *head) {
    Instruction *p;
    bool immediate;
    //Create a dummy-head linked list
    ObjCode *h;
    h = emalloc(sizeof(ObjCode));
    h->code = estrdup("NULL");
    h->pos = 0;
    h->next = 0;
    for(p = head; p; p = p->next) {
        immediate = canBeImmediate(p->op->opcode);
        // Check if we need to change the opCode
        if(immediate && p->opds[2]->type == NUMBER_TYPE)
            h->next = createNewObjCode(p, true, label_table);
        else
            h->next = createNewObjCode(p, false, label_table);
    }
    return h;
}

//TODO: REMOVE DEBUGS
ObjCode* createNewObjCode(Instruction *ins, bool im, SymbolTable label_table) {
    char* aux;
    int num = 0;
    EntryData *ret;
    ObjCode* obCode;
    obCode = emalloc(sizeof(ObjCode));
    obCode->code = emalloc(9*sizeof(char));
    obCode->size = 9;
    obCode->pos = ins->pos;
    obCode->next = 0;

    if(im)
        num++;

    // All immediate operators have 3 operands!
    if(canBeImmediate(ins->op->opcode) || im) {
        aux = emalloc(3);
        // Print in 'aux' the operator code
        if(im)
            sprintf(aux, "%02x",(ins->op->opcode + 1));
        else
            sprintf(aux, "%02x", ins->op->opcode);
        strcat(obCode->code, aux);
        for(int i = 0; i < 3; i++) {
            aux[0] = 0;
            switch (ins->opds[i]->type) {
                case REGISTER:
                    sprintf(aux, "%02x", ins->opds[i]->value.reg);
                    break;
                case NUMBER_TYPE:
                    sprintf(aux, "%02llx", ins->opds[i]->value.num);
                    break;
                default:
                    printf("ERROOOOOOOOOOOUUUUUUUUUUUUUUUUUUUUUU\n");
                    exit(-1);
            }
            strcat(obCode->code, aux);
        }
        free(aux);
        return obCode;
    }

    // Special operators who need a different treatment...
    if(isSpecial(ins->op->opcode)) {
        switch (ins->op->opcode) {
            case NOP:
                obCode->code = estrdup("ff000000");
                return obCode;
            case INT:
                aux = emalloc(7);
                obCode->code = estrdup("fe");
                sprintf(aux, "%06llx", ins->opds[0]->value.num);
                strcat(obCode->code, aux);
                free(aux);
                return obCode;
            case SETW:
                aux = emalloc(3);
                obCode->code = estrdup("5a");
                sprintf(aux, "%02x", ins->opds[0]->value.reg);
                strcat(obCode->code, aux);
                free(aux);
                aux = emalloc(5);
                sprintf(aux, "%04llx", ins->opds[1]->value.num);
                strcat(obCode->code, aux);
                free(aux);
                return obCode;
        }
        // SAVE and REST
        aux = emalloc(3);
        sprintf(aux, "%02x", ins->op->opcode);
        strcat(obCode->code, aux);
        for(int i = 0; i < 3; i++) {
            aux[0] = 0;
            sprintf(aux, "%02x", ins->opds[i]->value.reg);
            strcat(obCode->code, aux);
        }
        free(aux);
        return obCode;
    }

    // If it's conditional jump
    if(isJump(ins->op->opcode)) {
        switch (ins->op->opcode) {
            case JMP:
                if(ins->opds[0]->type == LABEL) {
                    ret = stable_find(label_table, ins->opds[0]->value.label);
                    //Label is defined
                    if(ret) {
                        aux = emalloc(7);
                        if(ret->i >= ins->pos) {
                            obCode->code = estrdup("48");
                            sprintf(aux, "%06x", ret->i - ins->pos);
                        }
                        else {
                            obCode->code = estrdup("49");
                            sprintf(aux, "%06x", ins->pos - ret->i);
                        }
                        strcat(obCode->code, aux);
                        free(aux);
                        return obCode;
                    }
                    /* Label is not defined:
                     * Print in the format: "*JMP label"
                     */
                    free(obCode->code);
                    obCode->code = emalloc(6+strlen(ins->opds[0]->value.label));
                    obCode->code = estrdup("*JMP ");
                    strcat(obCode->code, estrdup(ins->opds[0]->value.label));
                    obCode->size = strlen(obCode->code);
                    return obCode;
                }
                // Is a number, so we check whether it's JMP or JMPB
                int num = ins->opds[0]->value.num;
                obCode->code = (num >= 0) ? estrdup("48") : estrdup("49");
                aux = emalloc(7);
                sprintf(aux, "%06x", abs(num));
                strcat(obCode->code, aux);
                free(aux);
                return obCode;
            case GO:
                if(ins->opds[1]->type == LABEL) {
                    EntryData* ret = stable_find(label_table, ins->opds[1]->value.label);
                    if(ret) {
                        aux = emalloc(3);
                        sprintf(aux, "%02x",ins->opds[0]->value.reg);
                        if(ret->i >= ins->pos) {
                            obCode->code = estrdup("56");
                            strcat(obCode->code, aux);
                            aux = emalloc(5);
                            sprintf(aux, "%04x", ret->i - ins->pos);
                        }
                        else {
                            obCode->code = estrdup("57");
                            strcat(obCode->code, aux);
                            aux = emalloc(5);
                            sprintf(aux, "%04x", ins->pos - ret->i);
                        }
                        strcat(obCode->code, aux);
                        free(aux);
                        return obCode;
                    }
                    /* Label is not defined:
                     * Print in the format: "*GO ff label"
                     */
                    free(obCode->code);
                    obCode->code = emalloc(8+strlen(ins->opds[1]->value.label));
                    obCode->code = estrdup("*GO ");
                    aux = emalloc(3);
                    sprintf(aux, "%02x", ins->opds[0]->value.reg);
                    strcat(obCode->code, aux);
                    strcat(obCode->code, " ");
                    strcat(obCode->code, estrdup(ins->opds[1]->value.label));
                    obCode->size = strlen(obCode->code);
                    free(aux);
                    return obCode;
                }
                // Is a number, so we check whether it's GO or GOB
                num = ins->opds[0]->value.num;
                obCode->code = (num >= 0) ? estrdup("56") : estrdup("57");
                aux = emalloc(3);
                sprintf(aux, "%02x",ins->opds[0]->value.reg);
                strcat(obCode->code, aux);
                aux = emalloc(5);
                sprintf(aux, "%04x", abs(num));
                free(aux);
                return obCode;
            case GETA:
                if(ins->opds[1]->type == LABEL) {
                    EntryData* ret = stable_find(label_table, ins->opds[1]->value.label);
                    if(ret) {
                        aux = emalloc(3);
                        sprintf(aux, "%02x",ins->opds[0]->value.reg);
                        if(ret->i >= ins->pos) {
                            obCode->code = estrdup("58");
                            strcat(obCode->code, aux);
                            aux = emalloc(5);
                            sprintf(aux, "%04x", ret->i - ins->pos);
                        }
                        else {
                            obCode->code = estrdup("59");
                            strcat(obCode->code, aux);
                            aux = emalloc(5);
                            sprintf(aux, "%04x", ins->pos - ret->i);
                        }
                        strcat(obCode->code, aux);
                        free(aux);
                        return obCode;
                    }
                    /* Label is not defined:
                     * Print in the format: "*GETA ff label"
                     */
                    free(obCode->code);
                    obCode->code = emalloc(10+strlen(ins->opds[1]->value.label));
                    obCode->code = estrdup("*GETA ");
                    aux = emalloc(3);
                    sprintf(aux, "%02x", ins->opds[0]->value.reg);
                    strcat(obCode->code, aux);
                    strcat(obCode->code, " ");
                    strcat(obCode->code, estrdup(ins->opds[1]->value.label));
                    obCode->size = strlen(obCode->code);
                    free(aux);
                    return obCode;
                }
                // Is a number, so we check whether it's GETA or GETAB
                num = ins->opds[0]->value.num;
                obCode->code = (num >= 0) ? estrdup("58") : estrdup("59");
                aux = emalloc(3);
                sprintf(aux, "%02x",ins->opds[0]->value.reg);
                strcat(obCode->code, aux);
                aux = emalloc(5);
                sprintf(aux, "%04x", abs(num));
                free(aux);
                return obCode;
            default:
                free(obCode->code);
                free(obCode);
                return getJumpObj(ins, label_table);
        }
    }


    printf("QUEM SERA QUE FOI??????????????????????WW\n");
    exit(-1);
}

ObjCode* getJumpObj(Instruction *ins, SymbolTable label_table) {
    char *aux, *aux2;
    ObjCode* ob;
    ob = emalloc(sizeof(ObjCode));
    ob->code = emalloc(9*sizeof(char));
    ob->pos = ins->pos;
    ob->next = 0;
    if(ins->opds[1]->type == LABEL) {
        EntryData* ret = stable_find(label_table, ins->opds[1]->value.label);
        // If the label is defined
        if(ret) {
            aux = emalloc(3);
            sprintf(aux, "%02x",ins->opds[0]->value.reg);
            if(ret->i >= ins->pos) {
                aux2 = emalloc(3);
                sprintf(aux2, "%02x", ins->op->opcode);
                strcat(ob->code, aux2);
                strcat(ob->code, aux);
                aux = emalloc(5);
                sprintf(aux, "%04x", ret->i - ins->pos);
            }
            else {
                aux2 = emalloc(3);
                sprintf(aux2, "%02x", ins->op->opcode + 1);
                strcat(ob->code, aux2);
                strcat(ob->code, aux);
                aux = emalloc(5);
                sprintf(aux, "%04x", ins->pos - ret->i);
            }
            strcat(ob->code, aux);
            free(aux);
            free(aux2);
            return ob;
        }
        // The label isn't defined, i'll store in the correct format
        free(ob->code);
        ob->code = emalloc(strlen(ins->op->name)+6+strlen(ins->opds[1]->value.label));
        ob->code = estrdup("*");
        strcat(ob->code, estrdup(ins->op->name));
        strcat(ob->code, " ");
        aux = emalloc(3);
        sprintf(aux, "%02x", ins->opds[0]->value.reg);
        strcat(ob->code, " ");
        strcat(ob->code, ins->opds[1]->value.label);
        free(aux);
        ob->size = strlen(ob->code);
        return ob;
    }
    // It's a number!
    int num = ins->opds[1]->value.num;
    aux = emalloc(3);
    if(num >= 0)
        sprintf(aux, "%02x", ins->op->opcode);
    else
        sprintf(aux, "%02x", ins->op->opcode + 1);

    strcat(ob->code, aux);
    sprintf(aux, "%02x", ins->opds[0]->value.reg);
    strcat(ob->code, aux);
    aux = emalloc(5);
    sprintf(aux, "%04llx", ins->opds[1]->value.num);
    free(aux);
    return ob;
}
