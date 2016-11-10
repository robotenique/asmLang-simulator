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
 *  TODO: remove comments before cutSpc
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
    buffer_push_back(BS.B, 0);
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
        BS.x = ++BS.y;
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

    BS.x = ++BS.y;

    int nargs;
    for (nargs=0; iconf.opr -> opd_types[nargs] != 0; nargs++);

    if (nargs == 3) {
      Operand **vOps = getOperands(&BS, &errC, iconf.opr);

      if (vOps == NULL) {
        return 0; //null pointer
      }

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

Operand **getOperands(BufferStorage* bs, errContainer *errC, const Operator* op, SymbolTable st){
  int i = 0;
  int commas = 0;
  for (i = bs->x; i < bs->B->i-1; commas += (bs->B->data[i++] == ',')?1:0);

  errC = emalloc(sizeof(errContainer));

  char* oprds[3] = {strtok(bs->B->data, ","), strtok(NULL, ","), strtok(NULL, ",")};

  if (commas != 2 || oprds[0] == NULL || oprds[1] == NULL || oprds[2] == NULL) {
    errC -> errMsg = estrdup("Wrong number of operands.\n");
    errC -> pos = bs -> x;
    return NULL; // wrong number of commas
  }

  for (int i = 0; i < 3; i++) {
    oprds[i] = trimSpc(oprds[i]);

    int spaces = 0;
    for (int j = 0; oprds[j]; spaces += oprds[j] == ' ' ? 1 : 0);

    if (spaces) {
      errC -> errMsg = estrdup("Invalid operand found.\n");
      errC -> pos = bs -> x;
      return NULL;
    }
  }
/*
#define BYTE1        0x01  // One-byte number.
#define REGISTER     0x20  // Register.
#define IMMEDIATE    (REGISTER | BYTE1)  // Immediate constant.

*/Operand** ops = emalloc(3*sizeof(Operand*));

  for (int i = 0; i < 3; i++)
    switch (op -> opd_types[i]) {
      case REGISTER:
        ops[i] = isRegister(oprds[i]);
        if (ops[i] == NULL) {
          errC -> errMsg = estrdup("Invalid operand found.\n");
          errC -> pos = bs -> x;
          return NULL;
        }

        break;
      case BYTE1:

      case IMMEDIATE:
    }
}

Operand* isRegister(char* oprd, SymbolTable st){
  if (strcmp(oprd, "$") == 0) {
    return NULL;
  }

  if (strlen(oprd) > 1 && oprd[0] == '$') {
    char* check;
    int n = strtol(oprd+1, &check, 10);

    if (*check != '\0') {
      return NULL;
    }
    
    if (oprd[0] = '$' && n <= 255 && n >= 0) {
      return operand_create_register(oprd+1);
    }
  } else {
    EntryData *ed = stable_find(st, oprd);

    if (ed == NULL) return NULL;

    if (ed -> opd -> type == REGISTER) {
      return operand_dup(ed -> opd);
    }
  }
}


Operand* isByte(char* oprd, SymbolTable st){
  if (strcmp(oprd, "0") == 0) {
    return operand_create_register(oprd+1);    
  }

  char* check;
  int n = strtol(oprd, &check, 10);

  if (oprd[0] == '#'){ 
    if (strlen(oprd) < 2) {
      return NULL;
    }

    n = strtol(oprd+1, &check, 16);

    if (*check != '\0') {
      return NULL;
    }

    if (n >= 0 && n <= 255) {
      return operand_create_number((octa) n);
    }

    return NULL;
  } 

  if (*check == '\0') {
    if (n >= 0 && n <= 255) {
      return operand_create_number((octa) n);
    } 
  } else {
    EntryData *ed = stable_find(st, oprd);

    if (ed == NULL) return NULL;

    if (ed -> opr -> type == NUMBER_TYPE) {
      if (ed -> opr -> value.num >= 0 && ed -> opr -> value.num <= 255) {
        return operand_create_number((octa) ed -> opr -> value.num);
      }
    }
  }

  return NULL;
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

char *cutSpc(char *text) {
   int length, c, d;
   char *start;
   c = d = 0;
   length = strlen(text);
   start = (char*)malloc(length+1);
   if (start == NULL)
      exit(EXIT_FAILURE);

    while (*(text+c) != '\0') {
      if (*(text+c) == ' ') {
         int temp = c + 1;
         if (*(text+temp) != '\0') {
            while (*(text+temp) == ' ' && *(text+temp) != '\0') {
               if (*(text+temp) == ' ') {
                  c++;
               }
               temp++;
            }
         }
      }
      *(start+d) = *(text+c);
      c++;
      d++;
   }
   *(start+d)= '\0';
   return trimSpc(start);
}

char *trimSpc(char *c) {
    char * e = c + strlen(c) - 1;
    while(*c && isspace(*c)) c++;
    while(e > c && isspace(*e)) *e-- = '\0';
    return c;
}
