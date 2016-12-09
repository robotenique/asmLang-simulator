#ifndef __DEFAULTOPS_H__
#define __DEFAULTOPS_H__
#include <string.h>
#include <ctype.h>
#include "asmtypes.h"
#include "opcodes.h"
#include "optable.h"
#include "error.h"


typedef enum {false, true} bool;

typedef struct line {
    char *line;
    int number;
    struct line* next;
} Line;

typedef struct node{
    char* s;
    struct node* next;
} string;

string *end;
string *first;

typedef struct {
    char **str;
    int i;
} StrStorage;

bool isPseudo;

bool isConditional(const Operator *op);
char *trimSpc(char *s);
char *cutSpc(char *text);
char *trimComment(char *text);
bool isValidChar(char c);

#endif
