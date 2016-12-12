#ifndef __TRANSLATOR_H__
#define __TRANSLATOR_H__

#include "stable.h"
#include "asmtypes.h"

/*
  Returns a linked list with the object code
  
  INPUT:

  - label_table -- A symbol table with the labels

  - head -- The head of a linked list with the instruction

  Returns the head of the linked list which contains the object code,
  which was converted in this function;
*/
ObjCode* translateToObject(SymbolTable label_table, Instruction *head);

#endif
