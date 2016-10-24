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

typedef enum { false, true } bool;

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
    InstrAlias value;
    bool isLabel;
} InstrAux;

bool isEOL (BufferStorage BS);
bool isValidChar(char c);
int procLabel( BufferStorage *BS);
void errLabel(BufferStorage BS);
InstrAux getLabelOrOperator(BufferStorage *BS);


int parse(const char *s, SymbolTable alias_table, Instruction **instr,
          const char **errptr) {
    int i;
    BufferStorage BS;
    Operand *op;
    char *lb = NULL;
    char *str = estrdup(s);
    BS.B = buffer_create();
    BS.x = BS.y = 0;
    //Remove all spaces from the begining of the string
    for (i = 0; str[i]!=0; buffer_push_back(BS.B, str[i]), i++);
    for (i = BS.x; BS.B->data[i]!=0 && isspace(BS.B->data[i]); i++, BS.x++);
    if(isEOL(BS)) return 1;
    // If the label / Operator is invalid
    if(!procLabel(&BS)) {
        errLabel(BS);
        return 0;
    }
    else {
        // lb receives the string read
        lb = emalloc(BS.y - BS.x + 1);
        lb[BS.y - BS.x] = 0;
        int len = BS.y - BS.x + 1;
        for (int k = BS.x; k < len - 1; lb[k] = BS.B->data[k], k++);
        //If the next char isn't a space, then it's an error
        if(BS.B->data[BS.y + 1] != ' ') {
            errLabel(BS);
            return 0;
        }

    }




    return 0;
}


InstrAux getLabelOrOperator(BufferStorage *BS, errContainer *errC){
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
    char *str = estrdup((BS->B->data) + BS->x)
    Operator *op = optable_find(str);
    InstrAux ret;
    ret.d
    // If it's an operator
    if(op) {

    }


}


int procLabel(BufferStorage *BS) {
    int i;
    char first = BS->B->data[BS->x];
    if(!(isalpha(first)) && !(first == '_'))
        return 0;
    BS->y = BS->x;
    for (i = BS->y; isValidLabel(BS->B->data[i]); i++, BS->y++);
    return 1;

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
    if(c == '*' || c == ';')
        return true;
    return false;
}
