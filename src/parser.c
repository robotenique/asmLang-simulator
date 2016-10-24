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
    int errLoc;
}errContainer;

typedef union {
  char *label;
  Operator *opr;
} InstrAlias;

typedef struct InstrAux {
    InstrAlias val;
    bool isLabel;
} InstrAux;

bool isEOL (BufferStorage BS);
bool isValidChar(char c);
int procLabel( BufferStorage *BS);
void errLabel(BufferStorage BS);
InstrAux* getLabelOrOperator(BufferStorage *BS, errContainer *errC);
void addLabel(SymbolTable alias_table, const char *label);

int parse(const char *s, SymbolTable alias_table, Instruction **instr,
          const char **errptr) {
    int i;
    BufferStorage BS;
    errContainer errC;
    InstrAux *iAux;
    char *str = estrdup(s);
    BS.B = buffer_create();
    BS.x = BS.y = 0;
    //Remove all spaces from the begining of the string
    for (i = 0; str[i]!=0; buffer_push_back(BS.B, str[i]), i++);
    for (i = BS.x; BS.B->data[i]!=0 && isspace(BS.B->data[i]); i++, BS.x++);
    if(isEOL(BS)) return 1;
    iAux = getLabelOrOperator(&BS, &errC);
    /* IF iAux == NULL, then it's an error, so errC contains
     * the number where the error was found.
     */
    if (!iAux) { return 0; }
    if (iAux->isLabel)
        addLabel(alias_table, (iAux->val).label);


    return 0;
}

void addLabel(SymbolTable alias_table, const char *label) {
    char *aux = estrdup(label);
    printf("Adiciona label = %s\n",aux );
}

InstrAux* getLabelOrOperator(BufferStorage *BS, errContainer *errC){
    int i;
    errC->isErr = false;
    char first = BS->B->data[BS->x];
    if(!(isalpha(first)) && !(first == '_')) {
        errC->isErr = true;
        errC->errLoc = BS->x;
        return NULL;
    }
    for (i = BS->x; isValidChar(BS->B->data[i]); i++);
    BS->y = i;
    if(BS->B->data[BS->y + 1] != ' ') {
        errC->isErr = true;
        errC->errLoc = BS->y;
        return NULL;
    }
    char *str = estrdup((BS->B->data) + BS->x);
    const Operator *op = optable_find(str);
    InstrAux *ret;
    // If it's an operator
    if(op) {
        (ret->val).opr = op; //Try if it works TODO: make a operator_dup to fix this
        ret->isLabel = false;
        return ret;
    }
    // If it's a label
    ret->val.label = estrdup(str);
    ret->isLabel = true;
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
    if(!(BS.B->i))
        return true;
    c = BS.B->data[BS.x];
    if(c == '*')
        return true;
    return false;
}
