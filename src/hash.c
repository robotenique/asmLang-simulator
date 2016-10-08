/*
 * @author: Juliano Garcia de Oliveira
 * @author: Enzo Hideki Nakamura
 *
 * MAC0216
 *
 * Hash Implementation
 */
#include <stdio.h>
#include <string.h>
#include "../include/hash.h"

int p = 1e7 - 9;

long long po(int x, int e){
	long long res = 1;
	for(int i=0; i<e; i++) res = (res * x) % p;
	return res;
}

int hash(char* c){
	int t = 0, res = 0;

	for(; c[t]; t++);

	for(int i=0; i<t; i++)
		if(c[i] >= '!' && c[i] <= '~') 
			res = (res + (c[i]-'!'+1) * po(95, i)) % p;

	return res;
}
