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
#define LIMBYTE1 ((1UL << 8) - 1)
#define LIMBYTE2 ((1UL << 16) - 1)
#define LIMBYTE3 ((1UL << 24) - 1)
#define LIMTETRA ((1UL << 32) - 1)

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

char *trimSpc(char *s);
char *cutSpc(char *text);
char *trimComment(char *text);
bool isEmptyLine (BufferStorage BS);
bool isValidChar(char c);
bool containsLabel(SymbolTable alias_table, const char *label);
bool isOprInvalid(const Operator *op, errContainer *errC);
InstrAux* getLabelOrOperator(BufferStorage *BS, errContainer *errC);
Operand **getOperands_3(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st);
Operand **getOperands_1(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st);
Operand **getOperands_2(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st);
Operand* isString(char* oprd);
Operand* isLabel(char* oprd, SymbolTable st);
Operand* isByte(char* oprd, SymbolTable st, int neg, octa LIMBYTE);
Operand* isRegister(char* oprd, SymbolTable st);

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
    *instr = NULL;
    //Add the string to the buffer
    for (i = 0; str[i]!=0; buffer_push_back(BS.B, str[i]), i++);
    buffer_push_back(BS.B, 0);
    printf("read = |%s|\n",BS.B->data);
    if(isEmptyLine(BS)) return 1;
    iAux = getLabelOrOperator(&BS, &errC);
    /* IF iAux == NULL, then it's an error, so errC contains
     * the position (in the string) where the error was found.
     */
    if (iAux == NULL) return 0;


    if (iAux->isLabel) {
        // If there's error in adding the label, (TODO:) print ERROR
        // If the label is already in the symbol table, it's error
        if(containsLabel(alias_table, (iAux->val).label)) {
            errC.pos = BS.x;
            set_error_msg("Label already defined!");
            return 0;
        }
        iconf.label = true;
        iconf.lb = estrdup((iAux->val).label);
    }
    else {
        opr = (iAux->val).opr;
        if(isOprInvalid(opr, &errC)) {
            //TODO: SETAR o errptr em todos os "return 0"
            errC.pos = BS.x;
            return 0;
        }
        iconf.operator = true;
        iconf.opr = opr;
    }
    // We have the label, now we need the operator!
    if(iconf.label) {
        BS.x = ++BS.y;
        iAux = getLabelOrOperator(&BS, &errC);
        if (iAux == NULL) return 0;

        if(!iAux->isLabel) {
            opr = (iAux->val).opr;
            if(opr->opcode == EXTERN) {
                errC.pos = BS.x;
                set_error_msg("EXTERN operator doesn't support Label!");
                return 0;
            }
            iconf.operator = true;
            iconf.opr = opr;
        }
        else {
            errC.pos = BS.x;
            set_error_msg("Duplicate label assignment!");
            return 0;
        }
    }

    BS.x = ++BS.y;

    int nargs;
    for (nargs = 0; nargs < 3 && iconf.opr->opd_types[nargs] != OP_NONE; ++nargs);
    Operand **vOps;
    vOps = emalloc(sizeof(Operand *));
    for(int i = 0; i < 3; i++)
        vOps[i] = NULL;

    if (nargs == 3) {
        vOps = getOperands_3(&BS, &errC, iconf.opr, alias_table);
        if (vOps == NULL) return 0;


        if (iconf.label) {
          InsertionResult ir = stable_insert(alias_table, iconf.lb);
          ir.data->opd = operand_create_label(iconf.lb);
        }
    } else if (nargs == 2) {
        Operand **vOps = getOperands_2(&BS, &errC, iconf.opr, alias_table);

        if (vOps == NULL) return 0;

        if (iconf.label) {
          InsertionResult ir = stable_insert(alias_table, iconf.lb);
          ir.data->opd = operand_create_label(iconf.lb);
        }
    } else if (nargs == 1) {
        vOps = getOperands_1(&BS, &errC, iconf.opr, alias_table);
        if (vOps == NULL) return 0;
        if (iconf.label) {
          InsertionResult ir = stable_insert(alias_table, iconf.lb);
          if (iconf.opr -> opcode == STR) {
            ir.data->opd = operand_create_string(vOps[0] -> value.str);
          } else if (iconf.opr -> opcode == IS) {
            if (vOps[0] -> type == REGISTER) {
              ir.data->opd = operand_create_register(vOps[0] -> value.reg);
            } else {
            ir.data->opd = operand_create_number(vOps[0] -> value.num);
            }
          } else {
            ir.data->opd = operand_create_label(iconf.lb);
          }
        }
      }
      else { //NOP OPERATOR
        if (iconf.label) {
          InsertionResult ir = stable_insert(alias_table, iconf.lb);
          ir.data->opd = operand_create_label(iconf.lb);
        }
      }
    *instr = instr_create(str = iconf.label ? iconf.lb : NULL, iconf.opr, vOps);
    return 1;
}

Operand **getOperands_1(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st){
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
    set_error_msg("Wrong number of operands!\n");
    errC -> pos = bs -> x;
    return NULL; // wrong number of commas
  }

  oprds[0] = trimSpc(oprds[0]);
  printf("operandLex: |%s|\n",oprds[0] );
  int spaces = 0;
  for (int j = 0; oprds[0][j]; spaces += ((oprds[0][j++] == ' ') ? 1 : 0));
  if (spaces) {
    set_error_msg("Invalid operand found!\n");
    errC -> pos = bs -> x;
    return NULL;
  }

  Operand** ops = emalloc(3*sizeof(Operand*));

  switch (op -> opd_types[0]) {
    case LABEL:
      if ((ops[0] = isLabel(oprds[0], st)) == NULL) {
        errC -> pos = bs -> x;
        return NULL;
      }
      break;
    case BYTE3:
      if ((ops[0] = isByte(oprds[0], st, 0, LIMBYTE3))== NULL){
        errC -> pos = bs -> x;
        return NULL;
      }
      break;
    case ADDR3:
      if((ops[0] = isLabel(oprds[0], st)) == NULL){
        if ((ops[0] = isByte(oprds[0], st, 1, LIMBYTE3)) == NULL) {
          errC -> pos = bs -> x;
          return NULL;
        }
      }
      break;
    case REGISTER:
      if ((ops[0] = isRegister(oprds[0], st)) == NULL) {
        errC -> pos = bs -> x;
        return NULL;
      }
      break;
    case BYTE1:
      if ((ops[0] = isByte(oprds[0], st,  0, LIMBYTE1)) == NULL) {
        errC -> pos = bs -> x;
        return NULL;
      }
      break;
    case STRING:
      if ((ops[0] = isString(oprds[0])) == NULL) {
        errC -> pos = bs -> x;
        return NULL;
      }
      break;

    case TETRABYTE | NEG_NUMBER:
      if ((ops[0] = isByte(oprds[0], st, 1, LIMTETRA)) == NULL) {
        errC -> pos = bs -> x;
        return NULL;
      }
      break;

    case REGISTER | TETRABYTE | NEG_NUMBER:
        if ((ops[0] = isRegister(oprds[0], st)) == NULL){
            if ((ops[0] = isByte(oprds[0], st, 1, LIMTETRA)) == NULL) {
                errC -> pos = bs -> x;
                return NULL;
            }
        }
      break;

    default:
      break;
  }

  ops[1] = ops[2] = NULL;
  return ops;
}

Operand **getOperands_2(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st){
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
    set_error_msg("Wrong number of operands.\n");
    errC -> pos = bs -> x;
    return NULL;
  }

  for (int i = 0; i < 2; i++) {
    oprds[i] = trimSpc(oprds[i]);
    printf("operandLex: |%s|\n",oprds[i] );
    int spaces = 0;
    for (int j = 0; oprds[i][j]; spaces += ((oprds[i][j++] == ' ') ? 1 : 0));
    if (spaces) {
      set_error_msg("Invalid operand found!\n");
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
          errC -> pos = bs -> x;
          return NULL;
        }
        break;
        case ADDR2:
            if ((ops[i] = isLabel(oprds[i], st)) == NULL) {
                if ((ops[i] = isByte(oprds[i], st, 1, LIMBYTE2)) == NULL){
                    errC -> pos = bs -> x;
                    return NULL;
                    }
            }
            break;
      case BYTE2:
            if ((ops[i] = isByte(oprds[i], st, 0, LIMBYTE2)) == NULL) {
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

Operand **getOperands_3(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st){
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
    set_error_msg("Wrong number of operands!\n");
    errC -> pos = bs -> x;
    free(tmp);
    return NULL;
  }
  for (int i = 0; i < 3; i++) {
    oprds[i] = trimSpc(oprds[i]);
    printf("operandLex: |%s|\n",oprds[i] );
    int spaces = 0;
    for (int j = 0; oprds[i][j]; spaces += ((oprds[i][j++] == ' ') ? 1 : 0));
    if (spaces) {
      set_error_msg("Invalid operand found!\n");
      errC -> pos = bs -> x;
      free(tmp);
      return NULL;
    }
  }

  Operand** ops = emalloc(3*sizeof(Operand*));

  for (int i = 0; i < 3; i++)
    switch (op -> opd_types[i]) {
      case REGISTER:
        ops[i] = isRegister(oprds[i], st);
        if (ops[i] == NULL) {
          errC -> pos = bs -> x;
          return NULL;
        }
        printf("REGISTER!\n");
        break;
      case BYTE1:
        if ((ops[i] = isByte(oprds[i], st, 0, LIMBYTE1)) == NULL) {
          errC -> pos = bs -> x;
          return NULL;
        }
        printf("BYTE1!\n");
        break;
      case IMMEDIATE:
        if ((ops[i] = isRegister(oprds[i], st)) == NULL) {
          if ((ops[i] = isByte(oprds[i], st, 0, LIMBYTE1)) == NULL) {
            errC -> pos = bs -> x;
            return NULL;
          }
        }
        break;
      default:
        break;
    }
    free(errC);
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
  set_error_msg("Invalid String!\n");
  return NULL;
}

Operand* isLabel(char* oprd, SymbolTable st){
  EntryData* ed = stable_find(st, oprd);

  if (ed != NULL && ed -> opd -> type == LABEL) {
    return operand_create_label(oprd);
  }
  set_error_msg("Label not declared!\n");
  return NULL;
}

Operand* isByte(char* oprd, SymbolTable st, int neg, octa LIMBYTE){
  int byteNum;
  switch (LIMBYTE) {
    case LIMBYTE1:
        byteNum = 1;
    break;
    case LIMBYTE2:
        byteNum = 2;
    break;
    case LIMBYTE3:
        byteNum = 3;
    break;
    case LIMTETRA:
        byteNum = 4;
    break;
    default:
        byteNum = -1;
    break;
  }
  if (strcmp(oprd, "0") == 0) {
    return operand_create_number(0);
  }
  char* check;
  octa n = strtoll(oprd, &check, 10);
  if (errno == ERANGE) {
      set_error_msg("Number not in range (overflow or underflow)!\n");
      return NULL;
  }

    if (oprd[0] == '#') {
        if (strlen(oprd) >= 2) {
            n = strtoll(oprd + 1, &check, 16);
            if (errno == ERANGE) {
                set_error_msg("Number not in range (overflow or underflow)!\n");
                return NULL;
            }
            if (*check == '\0')
                if (n >= -LIMBYTE*neg && n <= LIMBYTE)
                    return operand_create_number(n);

        }
    set_error_msg("Invalid operand, expected Byte%d number!\n",byteNum);
    return NULL;
    }
    if (*check == '\0') {
        if (n >= -LIMBYTE*neg && n <= LIMBYTE)
            return operand_create_number(n);
        set_error_msg("Invalid operand, expected Byte%d number!\n",byteNum);
    }
    else {
    EntryData *ed = stable_find(st, oprd);
        if (ed != NULL)
            if (ed->opd->type == NUMBER_TYPE)
                if (ed->opd->value.num >= -LIMBYTE*neg &&
                    ed->opd->value.num <= LIMBYTE)
                    return operand_create_number(ed->opd->value.num);
        set_error_msg("Invalid operand, Label is undefined or out of range!\n");

  }
  return NULL;
}

Operand* isRegister(char* oprd, SymbolTable st){
  if (strcmp(oprd, "$") == 0) {
      set_error_msg("Invalid operand, expected Register!\n");
    return NULL;
    }

  if (strlen(oprd) > 1 && oprd[0] == '$') {
    char* check;
    long long int n = strtoll(oprd + 1, &check, 10);
    if (errno == ERANGE) {
        set_error_msg("Invalid Register, number is out of range!\n");
        return NULL;
    }

    if (*check != '\0') {
        set_error_msg("Invalid Register!\n");
      return NULL;
    }

    if (oprd[0] == '$' && n <= 255 && n >= 0) {
      return operand_create_register((unsigned char)n);
    }
    set_error_msg("Invalid Register, number is out of range!\n");
  } else {
    EntryData *ed = stable_find(st, oprd);

    if (ed == NULL) {
        set_error_msg("Invalid Register, label not declared!\n");
        return NULL;
    }

    if (ed -> opd -> type == REGISTER) {
      return operand_dup(ed -> opd);
    }
    set_error_msg("Invalid Register, label is not a Register!\n");
  }
  return NULL;
}

bool isOprInvalid(const Operator *op, errContainer *errC) {
    if(op->opcode == IS) {
        set_error_msg("Operator \"IS\" expects a label!");
        return true;
    }
    return false;
}
bool containsLabel(SymbolTable alias_table, const char *label) {
    if(stable_find(alias_table, label))
        return true;
    return false;
}
InstrAux* getLabelOrOperator(BufferStorage *BS, errContainer *errC){
    int i;
    errC = emalloc(sizeof(errContainer));
    char first = BS->B->data[BS->x];
    if(!(isalpha(first)) && !(first == '_')) {
        errC->isErr = true;
        errC->pos = BS->x;
        set_error_msg("Invalid char for label / operator!");
        return NULL;
    }
    for (i = BS->x; isValidChar(BS->B->data[i]); i++);
    BS->y = i;
    // If the next char after the last valid char is
    // not an space, then it's an error
    char *tmp = estrdup((BS->B->data) + BS->x);
    tmp[BS->y - BS->x] = 0;
    if(strcmp(tmp, "NOP") != 0 && BS->B->data[BS->y] != ' ') {
        errC->isErr = true;
        errC->pos = BS->y;
        if(BS->B->data[BS->y] == 0)
            set_error_msg("Unexpected end of line!");
        else
            set_error_msg("Unexpected char!");
        free(tmp);
        return NULL;
    }
    else{
        if(strcmp(tmp, "NOP") == 0 && BS->B->data[BS->y] != 0){
            set_error_msg("Unexpected Operand after NOP!");
            free(tmp);
            return NULL;
        }
        free(tmp);
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
bool isValidChar(char c)  {
    return c && (isalnum(c) || c == '_');
}
bool isEmptyLine(BufferStorage BS) {
    int i;
    if(BS.B->data[0] == 0)
        return true;
    for(i = 0; i < BS.B->i &&    isspace(BS.B->data[i]); i++);
    return i == BS.B->i;
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
