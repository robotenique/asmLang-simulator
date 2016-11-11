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
#include <errno.h>
#include "../include/buffer.h"
#include "../include/error.h"
#include "../include/hash.h"
#include "../include/asmtypes.h"
#include "../include/mactypes.h"
#include "../include/opcodes.h"
#include "../include/optable.h"
#include "../include/parser.h"
#define LIMBYTE1 1 << 7
#define LIMBYTE2 1 << 15
#define LIMBYTE3 1 << 23
#define LIMTETRA 1 << 31

typedef enum {false, true} bool;

typedef struct BufferStorage {
    Buffer *B;
    int x, y;
} BufferStorage;

typedef struct errContainer {
    bool isErr;
    int pos;
    char *errMsg;
} errContainer;

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
bool containsLabel(SymbolTable alias_table, const char *label);
char *trimComment(char *text);
Operand **getOperands_3(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st);
Operand* isRegister(char* oprd, SymbolTable st);
Operand* isByte_1(char* oprd, SymbolTable st);




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
    errno = 0;
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
    //Add the string to the buffer
    for (i = 0; str[i]!=0; buffer_push_back(BS.B, str[i]), i++);
    buffer_push_back(BS.B, 0);
    printf("read = |%s|\n",BS.B->data);
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
    for (nargs = 0; nargs < 3 && iconf.opr->opd_types[nargs] != OP_NONE; ++nargs);

    if (nargs == 3) {
        Operand **vOps = getOperands_3(&BS, &errC, iconf.opr, alias_table);

        if (vOps == NULL) {
            return 0; //null pointer
        }

        if (iconf.label) {
          InsertionResult ir = stable_insert(alias_table, iconf.lb);
          ir.data->opd = operand_create_label(iconf.lb);
        }       
        return 1;
    } else if (nargs == 2) {
        Operand **vOps = getOperands_2(&BS, &errC, iconf.opr, alias_table);

        if (vOps == NULL) {
            return 0; //null pointer
        }
        if (iconf.label) {
          InsertionResult ir = stable_insert(alias_table, iconf.lb);
          ir.data->opd = operand_create_label(iconf.lb);
        }   
      } else if (nargs == 1) {
        Operand **vOps = getOperands_1(&BS, &errC, iconf.opr, alias_table);
        if (vOps == NULL) {
            return 0; //null pointer
        }
        if (iconf.label) {
          InsertionResult ir = stable_insert(alias_table, iconf.lb);

          if (iconf.opr -> opcode == STR) {
            ir.data->opd = operand_create_string(vOps[0] -> value.str);
          } else if (iconf.opr -> opcode == IS) {
            if (vOps[0] -> type == REGISTER) {
              ir.data->opd = operand_create_register(vOps[0] -> value.reg);
            }

            ir.data->opd = operand_create_number(vOps[0] -> value.num);
          } else {
            ir.data->opd = operand_create_label(iconf.lb);
          } 
        }
      }
      else {
        //Operador NOP | rt rt    rt X ty,   tyt   ,    t :rt rt rt X ty,tyt,t
        // |LABEL NOP|
        if (iconf.label) {
          InsertionResult ir = stable_insert(alias_table, iconf.lb);
          ir.data->opd = operand_create_label(iconf.lb);
        }
      }

        

      *instr = instr_create(str = iconf.label ? iconf.lb : NULL, iconf.opr, vOps);
      return 1;

    return 0;
}

Operand **getOperands_1(BufferStorage* bs, errContainer *errC, const Operator* op, SymbolTable st){
  int i = 0;
  int commas = 0;
  for (i = bs->x; i < bs->B->i-1; commas += (bs->B->data[i++] == ',') ? 1 : 0);
  errC = emalloc(sizeof(errContainer));
  char* oprds[3] = {NULL, NULL, NULL};
  char *tmp;
  tmp = strtok(estrdup(bs->B->data + bs->x),",");
  for(int i = 0; i < 1 && tmp != NULL; i++) {
      oprds[i] = estrdup(tmp);
      tmp = strtok(NULL, ",");
  }

  if (commas != 0 || oprds[0] == NULL) {
    errC -> errMsg = estrdup("Wrong number of operands.\n");
    errC -> pos = bs -> x;
    return NULL; // wrong number of commas
  }

  oprds[0] = trimSpc(oprds[0]);
  printf("operandLex: |%s|\n",oprds[0] );
  int spaces = 0;
  for (int j = 0; oprds[0][j]; spaces += ((oprds[0][j++] == ' ') ? 1 : 0));
  if (spaces) {
    errC -> errMsg = estrdup("Invalid operand found.\n");
    errC -> pos = bs -> x;
    return NULL;
  }

  Operand** ops = emalloc(3*sizeof(Operand*));

  switch (op -> opd_types[0]) {
    case LABEL:
      if ((ops[i] = isLabel(oprds[i], st)) == NULL) {
        errC -> errMsg = estrdup("Invalid operand found.\n");
        errC -> pos = bs -> x;
        return NULL;
      }
      printf("REGISTER!\n");
      break;
    case BYTE3:
      if ((ops[i] = isByte_3(oprds[i], st, 0) == NULL)){
        errC -> errMsg = estrdup("Invalid operand found.\n");
        errC -> pos = bs -> x;
        return NULL;
      }
      
      break;
    case ADDR3:
      if((ops[i] = isLabel(oprds[i], st)) == NULL){
        if ((ops[i] = isByte_3(oprds[i], st, 1)) == NULL) {
          errC -> errMsg = estrdup("Invalid operand found.\n");
          errC -> pos = bs -> x;
          return NULL; 
        }
      }

      break;
    case REGISTER:
      if ((ops[i] = isRegister(oprds[i], st)) == NULL) {
        errC -> errMsg = estrdup("Invalid operand found.\n");
        errC -> pos = bs -> x;
        return NULL;
      }
      printf("REGISTER!\n");
      break;
    case BYTE1:
      if ((ops[i] = isByte_1(oprds[i], st)) == NULL) {
        errC -> errMsg = estrdup("Invalid operand found.\n");
        errC -> pos = bs -> x;
        return NULL;
      }
      printf("BYTE1!\n");
      break;
    case STRING:
      if ((ops[i] = isString(oprds[i])) == NULL) {
        errC -> errMsg = estrdup("Invalid operand found.\n");
        errC -> pos = bs -> x;
        return NULL;
      }

      printf("STRING!\n");
      break;

    case TETRABYTE | NEG_NUMBER:
      if ((ops[i] = isByte_4(oprds[i], st, 1)) == NULL) {
        errC -> errMsg = estrdup("Invalid operand found.\n");
        errC -> pos = bs -> x;
        return NULL;
      }

      printf("BYTE4!\n");
      break;

    case REGISTER | TETRABYTE | NEG_NUMBER:
      if ((ops[i] = isRegister(oprds[i], st)) == NULL)
        if ((ops[i] = isByte_4(oprds[i], st, 1)) == NULL) {
          errC -> errMsg = estrdup("Invalid operand found.\n");
          errC -> pos = bs -> x;
          return NULL;
        }

      break;

    default:
      break;
  }

  ops[1] = ops[2] = NULL;
  return ops;
}

Operand **getOperands_2(BufferStorage* bs, errContainer *errC, const Operator* op, SymbolTable st){
  int i = 0;
  int commas = 0;
  for (i = bs->x; i < bs->B->i-1; commas += (bs->B->data[i++] == ',') ? 1 : 0);
  errC = emalloc(sizeof(errContainer));
  char* oprds[3] = {NULL, NULL, NULL};
  char *tmp;
  tmp = strtok(estrdup(bs->B->data + bs->x),",");
  for(int i = 0; i < 2 && tmp != NULL; i++) {
      oprds[i] = estrdup(tmp);
      tmp = strtok(NULL, ",");
  }

  if (commas != 1 || oprds[0] == NULL || oprds[1] == NULL) {
    errC -> errMsg = estrdup("Wrong number of operands.\n");
    errC -> pos = bs -> x;
    return NULL; // wrong number of commas
  }

  for (int i = 0; i < 2; i++) {
    oprds[i] = trimSpc(oprds[i]);
    printf("operandLex: |%s|\n",oprds[i] );
    int spaces = 0;
    for (int j = 0; oprds[i][j]; spaces += ((oprds[i][j++] == ' ') ? 1 : 0));
    if (spaces) {
      errC -> errMsg = estrdup("Invalid operand found.\n");
      errC -> pos = bs -> x;
      return NULL;
    }
  }

  Operand** ops = emalloc(3*sizeof(Operand*));

  for (int i = 0; i < 2; i++)
    switch (op -> opd_types[i]) {
      case REGISTER:
        ops[i] = isRegister(oprds[i], st);
        if (ops[i] == NULL) {
          errC -> errMsg = estrdup("Invalid operand found.\n");
          errC -> pos = bs -> x;
          return NULL;
        }
        printf("REGISTER!\n");
        break;
      case ADDR2:
        if ((ops[i] = isLabel(oprds[i], st)) == NULL) {
          if ((ops[i] = isByte_2(oprds[i], st, 1) == NULL)){
            errC -> errMsg = estrdup("Invalid operand found.\n");
            errC -> pos = bs -> x;
            return NULL;
          }
        }

        break;
      case BYTE2:
        if ((ops[i] = isByte_2(oprds[i], st, 0)) == NULL) {
          errC -> errMsg = estrdup("Invalid operand found.\n");
          errC -> pos = bs -> x;
          return NULL; 
        }

        break;
      default:
        break;
    }
    ops[2] = NULL;

    return ops;
}

Operand **getOperands_3(BufferStorage* bs, errContainer *errC, const Operator* op, SymbolTable st){
  int i = 0;
  int commas = 0;
  for (i = bs->x; i < bs->B->i-1; commas += (bs->B->data[i++] == ',') ? 1 : 0);
  errC = emalloc(sizeof(errContainer));
  char* oprds[3] = {NULL, NULL, NULL};
  char *tmp;
  tmp = strtok(estrdup(bs->B->data + bs->x),",");
  for(int i = 0; i < 3 && tmp != NULL; i++) {
      oprds[i] = estrdup(tmp);
      tmp = strtok(NULL, ",");
  }

  if (commas != 2 || oprds[0] == NULL || oprds[1] == NULL || oprds[2] == NULL) {
    errC -> errMsg = estrdup("Wrong number of operands.\n");
    errC -> pos = bs -> x;
    return NULL; // wrong number of commas
  }
  for (int i = 0; i < 3; i++) {
    oprds[i] = trimSpc(oprds[i]);
    printf("operandLex: |%s|\n",oprds[i] );
    int spaces = 0;
    for (int j = 0; oprds[i][j]; spaces += ((oprds[i][j++] == ' ') ? 1 : 0));
    if (spaces) {
      errC -> errMsg = estrdup("Invalid operand found.\n");
      errC -> pos = bs -> x;
      return NULL;
    }
  }

  Operand** ops = emalloc(3*sizeof(Operand*));

  for (int i = 0; i < 3; i++)
    switch (op -> opd_types[i]) {
      case REGISTER:
        ops[i] = isRegister(oprds[i], st);
        if (ops[i] == NULL) {
          errC -> errMsg = estrdup("Invalid operand found.\n");
          errC -> pos = bs -> x;
          return NULL;
        }
        printf("REGISTER!\n");
        break;
      case BYTE1:
        ops[i] = isByte_1(oprds[i], st);
        if (ops[i] == NULL) {
          errC -> errMsg = estrdup("Invalid operand found.\n");
          errC -> pos = bs -> x;
          return NULL;
        }
        printf("BYTE1!\n");
        break;
      case IMMEDIATE:
        if ((ops[i] = isRegister(oprds[i], st)) == NULL) {
          if ((ops[i] = isByte_1(oprds[i], st)) == NULL) {
            errC -> errMsg = estrdup("Invalid operand found.\n");
            errC -> pos = bs -> x;
            return NULL; 
          }
        }

        break;
      default:
        break;
    }

    return ops;
}

Operand* isString(char* oprd){
  int i;
  int count = 0;
  for (i = 0; oprd[i]; count += (oprd[i++] == '\"') ? 1 : 0);

  Buffer* B = buffer_create();
  for (i = 0; oprd[i]; i++) {
    if (oprd[i] != '\"') {
      buffer_push_back(B, oprd[i]);
    }
  }

  buffer_push_back(B,0);

  if (count == 2 && oprd[0] == '\"' && oprd[strlen(oprd) - 1] == '\"') {
    return operand_create_string(B -> data);
  }

  return NULL;
}

Operand* isLabel(char* oprd, SymbolTable st){
  EntryData* ed = stable_find(st, oprd);

  if (ed != NULL && ed -> opd -> type == LABEL) {
    return operand_create_label(oprd);
  }

  return NULL;
}

Operand* isByte_1(char* oprd, SymbolTable st){
  if (strcmp(oprd, "0") == 0) {
    return operand_create_number(0);
  }
  char* check;
  long long int n = strtoll(oprd, &check, 10);
  if (errno == ERANGE) return NULL;

    if (oprd[0] == '#') {
        if (strlen(oprd) >= 2) {
            n = strtoll(oprd+1, &check, 16);
            if (errno == ERANGE) return NULL;
            if (*check == '\0')
                if (n >= 0 && n <= LIMBYTE1)
                    return operand_create_number((octa) n);
        }
    return NULL;
    }
    if (*check == '\0') {
        if (n >= 0 && n <= LIMBYTE1)
            return operand_create_number((octa) n);
    }
    else {
    EntryData *ed = stable_find(st, oprd);
    if (ed != NULL)
        if (ed -> opd -> type == NUMBER_TYPE)
            if (ed -> opd -> value.num >= 0 && ed -> opd -> value.num <= LIMBYTE1)
                return operand_create_number((octa) ed -> opd -> value.num);
  }
  return NULL;
}

Operand* isByte_2(char* oprd, SymbolTable st, int neg){
  if (strcmp(oprd, "0") == 0) {
    return operand_create_number(0);
  }
  char* check;
  long long int n = strtoll(oprd, &check, 10);

  if (errno == ERANGE) return NULL;

    if (oprd[0] == '#') {
        if (strlen(oprd) >= 2) {
            n = strtoll(oprd+1, &check, 16);

            if (errno == ERANGE) return NULL;

            if (*check == '\0')
                if (n >= -LIMBYTE2*neg && n <= LIMBYTE2-1)
                    return operand_create_number((octa) n);
        }
        
        return NULL;
    }

    if (*check == '\0') {
        if (n >= -LIMBYTE2*neg && n <= LIMBYTE2-1)
            return operand_create_number((octa) n);
    }
    else {
    EntryData *ed = stable_find(st, oprd);
    if (ed != NULL)
        if (ed -> opd -> type == NUMBER_TYPE)
            if (ed -> opd -> value.num >= -LIMBYTE2*neg && ed -> opd -> value.num <= LIMBYTE2-1)
                return operand_create_number((octa) ed -> opd -> value.num);
  }
  return NULL;
}

Operand* isByte_3(char* oprd, SymbolTable st, int neg){
  if (strcmp(oprd, "0") == 0) {
    return operand_create_number(0);
  }
  char* check;
  long long int n = strtoll(oprd, &check, 10);

  if (errno == ERANGE) return NULL;

    if (oprd[0] == '#') {
        if (strlen(oprd) >= 2) {
            n = strtoll(oprd+1, &check, 16);
            if (errno == ERANGE) return NULL;
            if (*check == '\0')
                if (n >= -LIMBYTE3*neg && n <= LIMBYTE3-1)
                    return operand_create_number((octa) n);
        }
        
        return NULL;
    }

    if (*check == '\0') {
        if (n >= -LIMBYTE3*neg && n <= LIMBYTE3-1)
            return operand_create_number((octa) n);
    }
    else {
    EntryData *ed = stable_find(st, oprd);
    if (ed != NULL)
        if (ed -> opd -> type == NUMBER_TYPE)
            if (ed -> opd -> value.num >= -LIMBYTE3*neg && ed -> opd -> value.num <= LIMBYTE3-1)
                return operand_create_number((octa) ed -> opd -> value.num);
  }
  return NULL;
}

Operand* isByte_4(char* oprd, SymbolTable st, int neg){
  if (strcmp(oprd, "0") == 0) {
    return operand_create_number(0);
  }
  char* check;
  long long int n = strtoll(oprd, &check, 10);
  if (errno == ERANGE) return NULL;

    if (oprd[0] == '#') {
        if (strlen(oprd) >= 2) {
            n = strtoll(oprd+1, &check, 16);
            if (errno == ERANGE) return NULL;
            if (*check == '\0')
                if (n >= -LIMTETRA*neg && n <= LIMTETRA-1)
                    return operand_create_number((octa) n);
        }
        
        return NULL;
    }

    if (*check == '\0') {
        if (n >= -LIMTETRA*neg && n <= LIMTETRA-1)
            return operand_create_number((octa) n);
    }
    else {
    EntryData *ed = stable_find(st, oprd);
    if (ed != NULL)
        if (ed -> opd -> type == NUMBER_TYPE)
            if (ed -> opd -> value.num >= -LIMTETRA*neg && ed -> opd -> value.num <= LIMTETRA-1)
                return operand_create_number((octa) ed -> opd -> value.num);
  }
  return NULL;
}

Operand* isRegister(char* oprd, SymbolTable st){
  if (strcmp(oprd, "$") == 0) {
    return NULL;
  }

  if (strlen(oprd) > 1 && oprd[0] == '$') {
    char* check;
    long long int n = strtoll(oprd, &check, 10);
    if (errno == ERANGE) return NULL;

    if (*check != '\0') {
      return NULL;
    }

    if (oprd[0] == '$' && n <= 255 && n >= 0) {
      return operand_create_register((unsigned char)n);
    }
  } else {
    EntryData *ed = stable_find(st, oprd);

    if (ed == NULL) return NULL;

    if (ed -> opd -> type == REGISTER) {
      return operand_dup(ed -> opd);
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
        return true;
    return false;
}
/*
bool addLabel(SymbolTable alias_table, const char *label, errContainer *errC,
            Operand *opr) {
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
*/

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
    if(strcmp(BS->B->data + BS->x, "NOP") != 0 && BS->B->data[BS->y] != ' ') {
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
   for(int i = 0; text[i]; text[i] = isspace(text[i]) ? ' ':text[i], i++);
   length = strlen(text);
   start = (char*)malloc(length+1);
   if (start == NULL)
      die("Error in memory allocation!");

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

   return trimSpc(trimComment(start));
}

char *trimSpc(char *c) {
    char * e = c + strlen(c) - 1;
    while(*c && isspace(*c)) c++;
    while(e > c && isspace(*e)) *e-- = '\0';
    return c;
}

char *trimComment(char *text) {
    int i;
    for(i = 0; text[i] && text[i] != '*'; i++);
    if(text[i] == '*') text[i] = 0;
    return estrdup(text);
}
