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

bool isEOL (BufferStorage BS);
int procLabel( BufferStorage *BS);
void errLabel(BufferStorage BS);

int parse(const char *s, SymbolTable alias_table, Instruction **instr,
          const char **errptr) {
    int i;
    //char *label;
    BufferStorage BS;
    char *str = estrdup(s);
    BS.B = buffer_create();
    BS.x = BS.y = 0;
    for (i = 0; str[i]!=0; buffer_push_back(BS.B, str[i]), i++);
    for (i = BS.x; BS.B->data[i]!=0 && isspace(BS.B->data[i]); i++, BS.x++);
    if(isEOL(BS)) return 1;
    // If the label is invalid
    if(procLabel(&BS) < 0) {
        errLabel(BS);
        return 0;
    }




    return 0;
}

int procLabel( BufferStorage *BS) {
    int i;
    char first = BS->B->data[BS->x++];
    if(!(isalpha(first)) && !(first == '_'))
        return -1;
    BS->y = BS->x + 1;
    for (i = BS->y; BS->B->data[i]!=0 && isValid(B); i++, BS->y++);
    char *lb = emalloc(BS->y - BS->x + 1);
    int len = BS->y - BS->x + 1;
    for (int k = BS->x; k < len;lb[k] = BS->B->data[k], k++);

    printf("label =  %s\n",lb );
    return 1;

}

void errLabel(BufferStorage BS) {
    printf("NOICE\n");
}
bool isValid(char c)  {
    return isalpha(c) || isdigit(c) || c == '_';
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
