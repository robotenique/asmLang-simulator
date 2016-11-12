#ifndef __DEFAULTOPS_H__
#define __DEFAULTOPS_H__
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

bool isConditional(const Operator *op);

#endif
