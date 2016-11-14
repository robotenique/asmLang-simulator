/*
 * @author: Juliano Garcia de Oliveira
 * @author: Enzo Hideki Nakamura
 *
 * MAC0216
 *
 * Parser implementation
 */

#include <ctype.h> //isspace, isalpha, etc
#include <stdlib.h> //malloc, strtoll
#include <string.h> //strcmp, strdup
#include <errno.h> //Check error
#include "../include/buffer.h"
#include "../include/error.h"
#include "../include/asmtypes.h"
#include "../include/mactypes.h"
#include "../include/opcodes.h"
#include "../include/optable.h"
#include "../include/parser.h"
#include "../include/defaultops.h"

/* Limits of the byte types */
#define LIMBYTE1 ((1UL <<  8) - 1)
#define LIMBYTE2 ((1UL << 16) - 1)
#define LIMBYTE3 ((1UL << 24) - 1)
#define LIMTETRA ((1LL << 32) - 1)

// Stores a buffer and two "positions"
typedef struct BufferStorage {
    Buffer *B;
    int x, y;
} BufferStorage;

// Container to store the position of the error and a string
typedef struct errContainer {
    int pos;
    char *or_String;
} errContainer;

typedef union {
  char *label;
  const Operator *opr;
} InstrAlias;

typedef struct InstrAux {
    InstrAlias val;
    bool isLabel;
} InstrAux;

// Stores the information read yet
typedef struct InstrConf {
    bool label;
    bool operator;
    bool *operands;
    char *lb;
    const Operator *opr;
}InstrConf;

/*---------------------------Function Prototypes -----------------------*/

bool isEmptyLine (BufferStorage BS);
bool containsLabel(SymbolTable alias_table, const char *label);
bool isOprInvalid(const Operator *op);
InstrAux* getLabelOrOperator(BufferStorage *BS, errContainer *errC);
/* Error processing functions */
int posErr(const char* source, char* raw, int pos);
int posErrOperands(char* source, int pos, int opNumber);
/* Function to process every operand */
Operand **getOperands_3(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st);
Operand **getOperands_1(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st);
Operand **getOperands_2(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st);
/* Operand parsing functions */
Operand* isString(char* oprd);
Operand* isLabel(char* oprd, SymbolTable st);
Operand* isByte(char* oprd, SymbolTable st, int neg, octa LIMBYTE);
Operand* isRegister(char* oprd, SymbolTable st);


/*
 * Function: parse
 * --------------------------------------------------------
 * Parses a line of instruction, and creates a Instruction instance with
 * the information of the line parsed. The function creates a default string
 * from the given line, get the label (if it has one), get the operator
 * and then get all the operators, while checking error in each step.
 * If any error is found, the parse function stops from parsing the rest
 * of the line.
 *
 * @args    s: The string with the original instruction.
 *          alias_table: Symbol table with the registered operands
 *          instr: Linked list with the instructions
 *          errptr: A pointer to a char in the string S
 *
 * @return 0 on error, nonzero on sucess
 */
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
    if(isEmptyLine(BS)) return -1;
    iAux = getLabelOrOperator(&BS, &errC);
    /* IF iAux == NULL, then it's an error, so errC contains
     * the position (in the string) where the error was found.
     */
    if (iAux == NULL) {
        errC.pos = posErr(s, BS.B->data, errC.pos);
        *errptr = &(s[errC.pos]);
        return 0;
    }

    if (iAux->isLabel) {
        // If there's error in adding the label, (TODO:) print ERROR
        // If the label is already in the symbol table, it's error
        if(containsLabel(alias_table, (iAux->val).label)) {
            set_error_msg("Label already defined!");
            errC.pos = posErr(s, BS.B->data, BS.x);
            *errptr = &(s[errC.pos]);
            return 0;
        }
        iconf.label = true;
        iconf.lb = estrdup((iAux->val).label);
    }
    else {
        opr = (iAux->val).opr;
        if(isOprInvalid(opr)) {
            errC.pos = posErr(s, BS.B->data, BS.x);
            *errptr = &(s[errC.pos]);
            return 0;
        }
        iconf.operator = true;
        iconf.opr = opr;
    }
    // We have the label, now we need the operator!
    if(iconf.label) {
        BS.x = ++BS.y;
        iAux = getLabelOrOperator(&BS, &errC);
        if (iAux == NULL) {
          errC.pos = posErr(s, BS.B->data, errC.pos);
            *errptr = &(s[errC.pos]);
            return 0;
        }

        if(!iAux->isLabel) {
            opr = (iAux->val).opr;
            if(opr->opcode == EXTERN) {
                errC.pos = BS.x;
                set_error_msg("EXTERN operator can't be Labeled!");
                errC.pos = posErr(s, BS.B->data, BS.x);
                *errptr = &(s[errC.pos]);
                return 0;
            }
            iconf.operator = true;
            iconf.opr = opr;
        }
        else {
            errC.pos = BS.x;
            set_error_msg("Duplicate label assignment!");
            errC.pos = posErr(s, BS.B->data, BS.x);
            *errptr = &(s[errC.pos]);
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

    errC.or_String = malloc(strlen(s) + 10);
    strcpy(errC.or_String, s);
    // Check the operands
    switch (nargs) {
        case 3:
            vOps = getOperands_3(&BS, &errC, iconf.opr, alias_table);
            if (vOps == NULL) {
                *errptr = &(s[errC.pos]);
                return 0;
            }
            if (iconf.label) {
                InsertionResult ir = stable_insert(alias_table, iconf.lb);
                ir.data->opd = operand_create_label(iconf.lb);
            }
        break;
        case 2:
            vOps = getOperands_2(&BS, &errC, iconf.opr, alias_table);
            if (vOps == NULL) {
                *errptr = &(s[errC.pos]);
                return 0;
            }
            if (iconf.label) {
                InsertionResult ir = stable_insert(alias_table, iconf.lb);
                ir.data->opd = operand_create_label(iconf.lb);
            }
        break;
        case 1:
            vOps = getOperands_1(&BS, &errC, iconf.opr, alias_table);
            if (vOps == NULL) {
                *errptr = &(s[errC.pos]);
                return 0;
            }
            if (iconf.label) {
                InsertionResult ir = stable_insert(alias_table, iconf.lb);
                if (iconf.opr -> opcode == STR) {
                    ir.data->opd = operand_create_string(vOps[0] -> value.str);
                }
                else if (iconf.opr -> opcode == IS) {
                    if (vOps[0] -> type == REGISTER)
                        ir.data->opd = operand_create_register(vOps[0] -> value.reg);
                    else
                        ir.data->opd = operand_create_number(vOps[0] -> value.num);
                }
                else
                    ir.data->opd = operand_create_label(iconf.lb);
            }
        break;
        default:
            //NOP OPERATOR
            if (iconf.label) {
                InsertionResult ir = stable_insert(alias_table, iconf.lb);
                ir.data->opd = operand_create_label(iconf.lb);
            }
        break;
    }
    // Creates the instruction
    Instruction * newInst;
    newInst = instr_create(str = iconf.label ? iconf.lb : NULL, iconf.opr, vOps);
    if(*instr != NULL)
        (*instr)->next = newInst;
    *instr = newInst;
    return 1;
}

/*
 * Function: getOperands_1
 * --------------------------------------------------------
 * Get the operand for the instructions which requires 1 operand only.
 * The operand is parsed individually in the correct function.
 *
 * @args    bs: A buffer storage with the line in the buffer
 *          errC: Error container to set the error position
 *          op: The operator
 *          st: the SymbolTable with the operands already stored
 *
 * @return An array with the three operands, on success. In this case, the
 *          two last position of this array are "NULL". If there's an error
 &          returns NULL.
 */
Operand **getOperands_1(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st){
  int i = 0;
  int commas = 0;
  for (i = bs->x; i < bs->B->i-1; commas += (bs->B->data[i++] == ',') ? 1 : 0);
  char* oprds[3] = {NULL, NULL, NULL};
  char *tmp;
  tmp = strtok(estrdup(bs->B->data + bs->x),",");
  for(int i = 0; i < 1 && tmp != NULL; i++) {
      oprds[i] = estrdup(tmp);
      tmp = strtok(NULL, ",");
  }

  if (commas != 0 || oprds[0] == NULL) {
    set_error_msg("Wrong number of operands!\n");
    errC -> pos = posErrOperands(errC -> or_String, bs -> x, 1);
    return NULL; // wrong number of commas
  }

  oprds[0] = trimSpc(oprds[0]);
  int spaces = 0;
  for (int j = 0; oprds[0][j]; spaces += ((oprds[0][j++] == ' ') ? 1 : 0));
  if (spaces && op->opcode != STR) {
    set_error_msg("Invalid operand found for operator %s!\n",op->name);
    errC -> pos = posErrOperands(errC -> or_String, bs -> x, 1);
    return NULL;
  }

  Operand** ops = emalloc(3*sizeof(Operand*));

  // Process the operand
  switch (op -> opd_types[0]) {
    case LABEL:
      if ((ops[0] = isLabel(oprds[0], st)) == NULL) {
        if(isConditional(op))
            ops[0] = operand_create_label(oprds[0]);
        else {
            errC -> pos = posErrOperands(errC -> or_String, bs -> x, 1);
            return NULL;
        }
      }
      break;
    case BYTE3:
      if ((ops[0] = isByte(oprds[0], st, 0, LIMBYTE3))== NULL){
        errC -> pos = posErrOperands(errC -> or_String, bs -> x, 1);
        return NULL;
      }
      break;
    case ADDR3:
      if((ops[0] = isLabel(oprds[0], st)) == NULL){
        if ((ops[0] = isByte(oprds[0], st, 1, LIMBYTE3)) == NULL) {
            if(isConditional(op))
                ops[0] = operand_create_label(oprds[0]);
            else {
                errC -> pos = posErrOperands(errC -> or_String, bs -> x, 1);
                return NULL;
            }
        }
      }
      break;
    case REGISTER:
      if ((ops[0] = isRegister(oprds[0], st)) == NULL) {
        errC -> pos = posErrOperands(errC -> or_String, bs -> x, 1);
        return NULL;
      }
      break;
    case BYTE1:
      if ((ops[0] = isByte(oprds[0], st,  0, LIMBYTE1)) == NULL) {
        errC -> pos = posErrOperands(errC -> or_String, bs -> x, 1);
        return NULL;
      }
      break;
    case STRING:
      if ((ops[0] = isString(oprds[0])) == NULL) {
        errC -> pos = bs -> x;
        errC -> pos = posErrOperands(errC -> or_String, bs -> x, 1);
        return NULL;
      }
      break;

    case TETRABYTE | NEG_NUMBER:
      if ((ops[0] = isByte(oprds[0], st, 1, LIMTETRA)) == NULL) {
        errC -> pos = posErrOperands(errC -> or_String, bs -> x, 1);
        return NULL;
      }
      break;

    case REGISTER | TETRABYTE | NEG_NUMBER:
        if ((ops[0] = isRegister(oprds[0], st)) == NULL){
            if ((ops[0] = isByte(oprds[0], st, 1, LIMTETRA)) == NULL) {
                errC -> pos = posErrOperands(errC -> or_String, bs -> x, 1);
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

/*
 * Function: getOperands_2
 * --------------------------------------------------------
 * Get the operands for the instruction which requires 2 operands. Parse each
 * operand individually, and return them on sucess.
 *
 * @args    bs: A buffer storage with the line in the buffer
 *          errC: Error container to set the error position
 *          op: The operator
 *          st: the SymbolTable with the operands already stored
 *
 * @return An array with the three operands, on success. In this case, the
 *          last position of this array are "NULL". If there's an error
 *          returns NULL.
 */
Operand **getOperands_2(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st){
  int i = 0;
  int commas = 0;
  // Count the commans in the operands
  for (i = bs->x; i < bs->B->i-1; commas += (bs->B->data[i++] == ',') ? 1 : 0);
  char* oprds[3] = {NULL, NULL, NULL};
  char *tmp;
  tmp = strtok(estrdup(bs->B->data + bs->x),",");
  // Separate the operands by the ","
  for(int i = 0; i < 2 && tmp != NULL; i++) {
      oprds[i] = estrdup(tmp);
      tmp = strtok(NULL, ",");
  }

  if (commas != 1 || oprds[0] == NULL || oprds[1] == NULL) {
    set_error_msg("Wrong number of operands.\n");
    errC -> pos = bs -> x;
    if (commas != 1)
        errC -> pos = posErrOperands(errC -> or_String, strlen(errC -> or_String) - 1, 1);
    else if (oprds[0] == NULL)
        errC -> pos = posErrOperands(errC -> or_String, bs -> x, 1);
    else if (oprds[1] == NULL)
        errC -> pos = posErrOperands(errC -> or_String, bs -> x, 2);
    return NULL;
  }

  for (int i = 0; i < 2; i++) {
    oprds[i] = trimSpc(oprds[i]);
    int spaces = 0;
    for (int j = 0; oprds[i][j]; spaces += ((oprds[i][j++] == ' ') ? 1 : 0));
    if (spaces) {
        set_error_msg("Invalid operand found for operator %s!\n",op->name);
      errC -> pos = posErrOperands(errC -> or_String, bs -> x, i+1);
      return NULL;
    }
  }

  Operand** ops = emalloc(3*sizeof(Operand*));
  ops[2] = NULL;
  for (int i = 0; i < 2; i++)
    switch (op -> opd_types[i]) {
      case REGISTER:
        ops[i] = isRegister(oprds[i], st);
        if (ops[i] == NULL) {
          errC -> pos = bs -> x;
          errC -> pos = posErrOperands(errC -> or_String, bs -> x, i+1);
          return NULL;
        }
        break;
        case ADDR2:
            if ((ops[i] = isLabel(oprds[i], st)) == NULL) {
                if ((ops[i] = isByte(oprds[i], st, 1, LIMBYTE2)) == NULL) {
                    if(isConditional(op))
                        ops[i] = operand_create_label(oprds[i]);
                    else {
                        errC -> pos = posErrOperands(errC -> or_String, bs -> x, i+1);
                        return NULL;
                    }
                }
            }
            break;
      case BYTE2:
            if ((ops[i] = isByte(oprds[i], st, 0, LIMBYTE2)) == NULL) {
                errC -> pos = posErrOperands(errC -> or_String, bs -> x, i+1);
                return NULL;
            }
            break;
      default:
            break;
    }
    return ops;
}

/*
 * Function: getOperands_3
 * --------------------------------------------------------
 * Get the operands for the instruction which requres all 3 operands. Parse
 * each operand individually, and return them on sucess.
 *
 * @args    bs: A buffer storage with the line in the buffer
 *          errC: Error container to set the error position
 *          op: The operator
 *          st: the SymbolTable with the operands already stored
 *
 * @return An array with the three operands, on success. If there's an error
 *          returns NULL.
 */
Operand **getOperands_3(BufferStorage* bs, errContainer *errC,
    const Operator* op, SymbolTable st){
  int i = 0;
  int commas = 0;
  // Count the commas of the operands string
  for (i = bs->x; i < bs->B->i-1; commas += (bs->B->data[i++] == ',') ? 1 : 0);
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
    if (commas != 2)
        errC -> pos = posErrOperands(errC -> or_String, strlen(errC -> or_String) - 1, 1);
    else if (oprds[0] == NULL)
        errC -> pos = posErrOperands(errC -> or_String, bs -> x, 1);
    else if (oprds[1] == NULL)
        errC -> pos = posErrOperands(errC -> or_String, bs -> x, 2);
    else if (oprds[2] == NULL)
        errC -> pos = posErrOperands(errC -> or_String, bs -> x, 3);
    free(tmp);
    return NULL;
  }
  for (int i = 0; i < 3; i++) {
    oprds[i] = trimSpc(oprds[i]);
    int spaces = 0;
    for (int j = 0; oprds[i][j]; spaces += ((oprds[i][j++] == ' ') ? 1 : 0));
    if (spaces) {
      set_error_msg("Invalid operand found for operator %s!\n",op->name);
      errC -> pos = posErrOperands(errC -> or_String, bs -> x, i+1);
      free(tmp);
      return NULL;
    }
  }

  Operand** ops = emalloc(3*sizeof(Operand*));

  // Parse every operand
  for (int i = 0; i < 3; i++)
    switch (op -> opd_types[i]) {
      case REGISTER:
        ops[i] = isRegister(oprds[i], st);
        if (ops[i] == NULL) {
          errC -> pos = posErrOperands(errC -> or_String, bs -> x, i+1);
          return NULL;
        }
        break;
      case BYTE1:
        if ((ops[i] = isByte(oprds[i], st, 0, LIMBYTE1)) == NULL) {
          errC -> pos = bs -> x;
          posErrOperands(errC -> or_String, bs -> x, i+1);
          return NULL;
        }
        break;
      case IMMEDIATE:
        if ((ops[i] = isRegister(oprds[i], st)) == NULL) {
          if ((ops[i] = isByte(oprds[i], st, 0, LIMBYTE1)) == NULL) {
            errC -> pos = posErrOperands(errC -> or_String, bs -> x, i+1);
            return NULL;
          }
        }
        break;
      default:
        break;
    }
    return ops;
}

/*
 * Function: isString
 * --------------------------------------------------------
 * Check if a given operand string is a valid operand for the STR operator.
 * It checks the " in the expression to see if it's valid.
 *
 * @args    oprd : A string of the operand
 *
 * @return A operand instance on sucess, NULL on error.
 */
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

/*
 * Function: isLabel
 * --------------------------------------------------------
 * Check if a given operand string is a label, just checking if the
 * string is in the SymbolTable, and if it is, checking if the type is LABEL.
 *
 * @args    oprd: The string
 *          st: The SymbolTable with the operands
 *
 * @return  An instance of the operand on sucess, NULL on error
 */
Operand* isLabel(char* oprd, SymbolTable st){
  EntryData* ed = stable_find(st, oprd);

  if (ed != NULL && ed -> opd -> type == LABEL) {
    return operand_create_label(oprd);
  }
  set_error_msg("Label not declared!\n");
  return NULL;
}

/*
 * Function: isByte
 * --------------------------------------------------------
 * Check if a number is a valid byte type. The size of the byte is specified
 * by the parameter, then the function checks if it's a plain number checking
 * the limits, if not, checks if it's an hexadecimal number checking the
 * limits, if not, check if the string is in the symbol table, then checks if
 * the type is NUMBER_TYPE, and finally checks the limits.
 *
 * @args    oprd: A string with the operand
 *          st: A SymbolTable with the operands already stored
 *          neg: A flag to allow negative numbers. Set to 1 to allow, 0 to deny.
 *          LIMBYTE: the limits allowed for a number.
 *
 * @return An instance of the operand on sucess, NULL on error.
 */
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
  // If it's just a 0 we need to make an exception due to strtoll behavior
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
            //Calculate the hexadecimal number
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

/*
 * Function: isRegister
 * --------------------------------------------------------
 * Check if a string is a valid Register. If the string starts with '$',
 * it checks to see if the number / string after it is a number in the correct
 * range for a register. If it's a label, search it in the SymbolTable to check
 * if it was previously defined.
 *
 * @args    oprd: A string of the operand
 *          st: A SymbolTable with the operands already stored
 *
 * @return An instance of the operand on sucess, NULL on error.
 */
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

/*
 * Function: isOprInvalid
 * --------------------------------------------------------
 * Check if the operator is the "IS", on sucess sets the error message.
 * This is necessary because the operator IS the only operator that expects
 * a label.
 *
 * @args    op: The operator

 *
 * @return true if it's invalid, false if it's not
 */
bool isOprInvalid(const Operator *op) {
    if(op->opcode == IS) {
        set_error_msg("Operator \"IS\" expects a label!");
        return true;
    }
    return false;
}

/*
 * Function: containsLabel
 * --------------------------------------------------------
 * Sematically pretty way of checking if a label is in the SymbolTable.
 *
 * @args    alias_table: The SymbolTable with the operands
 *          label: The label to search in the SymbolTable
 *
 * @return true if it's on the symbol table, false if it's not
 */
bool containsLabel(SymbolTable alias_table, const char *label) {
    if(stable_find(alias_table, label))
        return true;
    return false;
}

/*
 * Function: getLabelOrOperator
 * --------------------------------------------------------
 * Get a label or an operator. It checks to see if the label is valid, then
 * checks if the label is an operator. If it's an operator, returns the
 * operator, if it's a label, returns the label.
 *
 * @args    BS : A buffer storage with the string of the instruction
 *          errC: The error Container
 *
 * @return  A InstrAux struct with the operator or the label, and a bool
            set to the type returned. If there's an error, returns NULL.
 */
InstrAux* getLabelOrOperator(BufferStorage *BS, errContainer *errC){
    int i;
    char first = BS->B->data[BS->x];
    if(!(isalpha(first)) && !(first == '_')) {
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
        errC->pos = (BS->y) - 1;
        if(BS->B->data[BS->y] == 0)
            set_error_msg("Unexpected end of line!");
        else
            set_error_msg("Unexpected char!");
        free(tmp);
        return NULL;
    }
    else{
        if(strcmp(tmp, "NOP") == 0 && BS->B->data[BS->y] != 0){
           errC->pos = BS->y;
            set_error_msg("Unexpected char after NOP!");
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
    return ret;
}

/*
 * Function: isEmptyLine
 * --------------------------------------------------------
 * Check if a line is empty
 *
 * @args    BS: The buffer storage with the string
 *
* @return true if it's empty, false if it's not
 */
bool isEmptyLine(BufferStorage BS) {
    int i;
    if(BS.B->data[0] == 0)
        return true;
    for(i = 0; i < BS.B->i &&    isspace(BS.B->data[i]); i++);
    return i == BS.B->i;
}

/*
 * Function: posErr
 * --------------------------------------------------------
 * Given a string "source" and another "raw", the function returns the position
 * of the char in raw[pos] but in the string source. It's used to set the
 * error pointer (*errPtr) in the parse function.
 *
 * @args    source: A string
 *          raw: the reduced form of the source string
 *
 * @return  the position of the char raw[pos] in the string 'source'
 */
int posErr(const char* source, char* raw, int pos) {
  int i = -1;
  int count = 0;
  do {
    if (!isspace(raw[++i])) count++;
  } while (i != pos);

  i = -1;
  do {
    if (!isspace(source[++i])) count--;
  } while (count);

  return i;
}

/*
 * Function: posErrOperands
 * --------------------------------------------------------
 * Similar to the posErr, but returns the first char of operand at position opnumber.
 *
 * @args    source: A string
 *          raw: the reduced form of the source string
 *          opnumber: the position of the operand (i.e., 1, 2, or 3, indicating
 *          first, second, or third, respectively)
 *
 * @return
 */
int posErrOperands(char* source, int pos, int opNumber) {
  int commas = 0;
  int i = pos;

  int commaPosition = 0;

  while (commas != opNumber-1) {
    if (source[i] == ',') commas++, commaPosition = i; 
    i++;
  }

  while (isspace(source[i])) i++;

  int sz = strlen(source);

  return i > sz-1 ? commaPosition : i;
}
