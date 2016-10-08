/*
 * @author: Juliano Garcia de Oliveira
 * @author: Enzo Hideki Nakamura
 *
 * MAC0216
 *
 * Symbol Table implementation
 */

/*
  stable.h

  A symbol table associating generic data to strings.
*/
#include "../include/stable.h"
#include "../include/hash.h"
#include "../include/error.h"
#include <stdlib.h>
#include <string.h>

static char hashes[HASH_SIZE][50];
static int keys[HASH_SIZE];


int i = 0;
static int qtd = 0;

// The symbol table.
typedef struct stable_s {
	EntryData* symboltable;
} stable_s;


/*
  Return a new symbol table.
*/
SymbolTable stable_create(){
    SymbolTable t = emalloc(sizeof(stable_s));
    (t -> symboltable) = emalloc(HASH_SIZE * sizeof(EntryData));

	return t;
}

/*
  Destroy a given symbol table.
*/
void stable_destroy(SymbolTable table){
	free(table -> symboltable);
    free(table);
}

/*
  Insert a new entry on the symbol table given its key.

  If there is already an entry with the given key, then a struct
  InsertionResult is returned with new == 0 and data pointing to the
  data associated with the entry. Otherwise, a struct is returned with
  new != 0 and data pointing to the data field of the new entry.

  If there is not enough space on the table, or if there is a memory
  allocation error, then crashes with an error message.
*/
InsertionResult stable_insert(SymbolTable table, const char *key){
	InsertionResult ir;

    ir.new = 0;

    char* cpy = emalloc(strlen(key));
    strcpy(cpy, key);
	  int h = hash(cpy);
    while (strcmp(hashes[h], key)) {
        if (hashes[h][0] == 0) {
        	if (qtd == HASH_SIZE) die("Symbol Table is already full.");

            strcpy(hashes[h], key);
            qtd++;

            ir.new = 1;
            ir.data = &(table -> symboltable[h]);

            keys[i++] = h;

            return ir;
        }

        h = (h + 1) % HASH_SIZE;
    }

    ir.data = &(table -> symboltable[h]);

    return ir;
}

/*
  Find the data associated with a given key.

  Given a key, returns a pointer to the data associated with it, or a
  NULL pointer if the key is not found.
*/
EntryData *stable_find(SymbolTable table, const char *key) {
    char* cpy = emalloc(strlen(key));
    cpy = strcpy(cpy, key);
    int h = hash(cpy);    

    EntryData* ed;

    if (strcmp(hashes[h],key)) {
        if (hashes[h] == 0) {
            return NULL;
        // se a hash estiver associada com alguma coisa
        } else {
            while (strcmp(hashes[h], key)) h = (h + 1) % HASH_SIZE;

            ed = &(table -> symboltable[h]);
        }
    } else {
        ed = &(table -> symboltable[h]);
    }

    return ed;
}

/*
  Visit each entry on the table.

  The visit function is called on each entry, with pointers to its key
  and data. If the visit function returns zero, then the iteration
  stops.

  Returns zero if the iteration was stopped by the visit function,
  nonzero otherwise.
*/
int stable_visit(SymbolTable table, int (*visit)(const char *key, EntryData *data)){

    for (int j=0; j<i; j++)
        if(!visit(hashes[keys[j]], &(table -> symboltable[keys[j]])))
            return 0;
    return 1;
}
