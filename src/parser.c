/*
 * @author: Juliano Garcia de Oliveira
 * @author: Enzo Hideki Nakamura
 *
 * MAC0216
 *
 * Parser implementation
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <mcheck.h>
#include "../include/buffer.h"
#include "../include/error.h"
#include "../include/hash.h"
#include "../include/asmtypes.h"
#include "../include/mactypes.h"
#include "../include/opcodes.h"
#include "../include/optable.h"
#include "../include/parser.h"

typedef enum {false, true} bool;

typedef struct BufferStorage {
    Buffer *B;
    int x, y;
} BufferStorage;

typedef struct errContainer {
    bool isErr;
    int pos;
    char *errMsg;
}errContainer;

typedef union {
  char *label;
  const Operator *opr;
} InstrAlias;

typedef struct InstrAux {
    InstrAlias val;
    bool isLabel;
} InstrAux;

typedef struct InstrConf {
    bool label;
    bool operator;
    bool *operands;
    char *lb;
    const Operator *opr;
}InstrConf;

bool isOprInvalid(const Operator *op, errContainer *errC);
char *trimSpc(char *s);
char *cutSpc(char *text);
bool isEOL (BufferStorage BS);
bool isValidChar(char c);
int procLabel( BufferStorage *BS);
void errLabel(BufferStorage BS);
InstrAux* getLabelOrOperator(BufferStorage *BS, errContainer *errC);
bool addLabel(SymbolTable alias_table, const char *label, errContainer *errC);


/******************************************************************************
 * parser.c
 * First, we remove every extra space from the string.
 * Then, we trim the spaces from the begining and ending of the string.
 * Now the string is in the 'raw' form, i.e., when there is a space, it's
 * only one, and there's no space in the end or the begining of the string;
 *
 * We then get a label or an operator.
 *    -> If we found a label, then we try to get another operator;
 *    -> If we found an operator, we evaluate if it's valid :
 *          - If the operator is 'IS', it's not valid, because 'IS' needs
 *            a label;
 *
 *
 *
 *
 *****************************************************************************/


int parse(const char *s, SymbolTable alias_table, Instruction **instr,
          const char **errptr) {
    int i;
    BufferStorage BS;
    errContainer errC;
    InstrConf iconf;
    const Operator *opr;
    InstrAux *iAux = NULL;
    char *str = cutSpc(estrdup(s));
    BS.B = buffer_create();
    BS.x = BS.y = 0;
    iconf.label = false;
    iconf.operator = false;
    //Remove all spaces from the begining of the string
    for (i = 0; str[i]!=0; buffer_push_back(BS.B, str[i]), i++);
    if(isEOL(BS)) return 1;
    iAux = getLabelOrOperator(&BS, &errC);
    /* IF iAux == NULL, then it's an error, so errC contains
     * the position (in the string) where the error was found.
     */
    if (iAux == NULL) { return 0; }

    if (iAux->isLabel) {
        // If there's error in adding the label, (TODO:) print ERROR
        // If the label is already in the symbol table, it's error
        if(containsLabel(alias_table, (iAux->val).label)) {
            errC.pos = BS.x;
            return 0;
        }
        iconf.label = true;
        iconf.lb = estrdup((iAux->val).label);
    }
    else {
        opr = (iAux->val).opr;
        if(isOprInvalid(opr, &errC)) { return 0; }
        iconf.operator = true;
        iconf.opr = opr;
    }
    // We have the label, now we need the operator!
    if(iconf.label) {
        BS.x = BS.y;
        BS.x++;
        BS.y++;
        iAux = getLabelOrOperator(&BS, &errC);
        if (iAux == NULL) {return 0; } //Expected operator
        if(!iAux->isLabel) {
            opr = (iAux->val).opr;
            if(opr->opcode == EXTERN) {
                /* TODO: Add error */
                /* "Invalid label with EXTERN operator" */
                return 0;
            }
            iconf.operator = true;
            iconf.opr = opr;
        }
        else {return 0;} // Duplicate labels error
    }

    
    /* When the program gets to this point, we have:
     * A label and an Operator    or
     * An Operator
     * TODO: Evaluate the operator, then get the (right type of) operands.
     *       After we got the operands, check if the string of the
     *       Instruction is over. If not, then there's an error.
     */



    return 0;
}

bool isOprInvalid(const Operator *op, errContainer *errC) {
    if(op->opcode == IS) {
        errC = emalloc(sizeof(errContainer));
        errC->errMsg = estrdup("The operator \"IS\" needs a label!");
        return true;
    }
    return false;
}

bool containsLabel(SymbolTable alias_table, const char *label) {
    if(stable_find(alias_table, label))
        return false;
    return true;
}
bool addLabel(SymbolTable alias_table, const char *label, errContainer *errC,
            Operand *opr) {
    /* Add a label to the symbol table. */
    errC = emalloc(sizeof(errContainer));
    InsertionResult ir = stable_insert(alias_table, label);
    if(ir.new == 0) {
        errC->errMsg = estrdup("Label already defined!");
        errC->isErr = true;
        return false;
    }
    free(errC);
    return true;
}

InstrAux* getLabelOrOperator(BufferStorage *BS, errContainer *errC){
    int i;
    errC = emalloc(sizeof(errContainer));
    errC->isErr = false;
    char first = BS->B->data[BS->x];
    if(!(isalpha(first)) && !(first == '_')) {
        errC->isErr = true;
        errC->pos = BS->x;
        return NULL;
    }
    for (i = BS->x; isValidChar(BS->B->data[i]); i++);
    BS->y = i;
    // If the next char after the last valid char is
    // not an space, then it's an error
    if(BS->B->data[BS->y] != ' ') {
        errC->isErr = true;
        errC->pos = BS->y;
        return NULL;
    }

    char *str = estrdup((BS->B->data) + BS->x);
    str[BS->y - BS->x] = 0;

    const Operator *op = optable_find(str);
    InstrAux *ret = emalloc(sizeof(InstrAux));
    // If it's an operator
    if(op != NULL){
        ret->val.opr = op;
        ret->isLabel = false;
        return ret;
    }
    // If it's a label
    else {
        ret->val.label = estrdup(str);
        ret->isLabel = true;
    }
    free(str);
    free(errC);
    return ret;
}


void errLabel(BufferStorage BS) {
    printf("NOICE\n");
}
bool isValidChar(char c)  {
    return c && (isalnum(c) || c == '_');
}
bool isEOL(BufferStorage BS) {
    char c;
    int k;
    if(BS.B->i == 0)
    if(!(BS.B->i))
        return true;
    c = BS.B->data[BS.x];
    if(c == '*')
        return true;
    return false;
}
