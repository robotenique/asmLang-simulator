/*
  hash.h

  Hash function header.
*/

#ifndef __HASH_H__
#define __HASH_H__

#include <stdio.h>

extern int MAXTABLE;

// Returns the result of hashing given a c-string
int hash(char* c);

#endif
