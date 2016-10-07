/*
 * @author: Juliano Garcia de Oliveira
 * @author: Enzo Hideki Nakamura
 *
 * MAC0216
 *
 * SymbolTable test: Frequency of words in a text
 */
#include "../include/buffer.h"
#include "../include/error.h"
#include "../include/hash.h"
#include "../include/stable.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* p;
    int freq;
} words;
char word[50], table[HASH_SIZE][50], seen[HASH_SIZE];
int keys[HASH_SIZE], nwords = 0;
words copiaSt[HASH_SIZE];
int wide = 0, ind = 0;

int max(int a, int b);
int visit(const char *key, EntryData *data);

int compare (const void * a, const void * b){
  words *orderA = (words *)a;
  words *orderB = (words *)b;

  return strcmp(orderA->p, orderB->p);
}

int main(int argc, char **argv) {
    Buffer *B = buffer_create();
    SymbolTable st = stable_create();
    FILE* input = fopen(argv[1], "r");
    FILE* output = fopen(argv[2], "a");

    // Input verification
    if (argc != 3)
        die("Wrong number of arguments, aborting...");
    if (input == NULL || output == NULL)
       die("Error opening files, aborting...");

    // Read lines and fill the SymbolTable
    while (read_line(input,B)) {
        buffer_push_back(B,0);
        int i;
        for (i = 0; isspace(B->data[i]) && B->data[i]!=0; i++);

        while (B->data[i]!=0) {
            memset(word,0,sizeof(word));
            int j = 0;
            while (!isspace(B->data[i]) && B->data[i] != 0 && i < (B -> i))
                word[j++] = B->data[i++];
            wide = max(wide, strlen(word));
            i++;
            //Generate hash
            int h = hash(word);
            //Insert the word in the SymbolTable
            InsertionResult ir = stable_insert(st, word);
            //Increase the word freq. in the SymbolTable if needed
            if (h!=0) (*ir.data).i = 1 + (!ir.new * (*ir.data).i);
        }
    }
    //Get all the words and their frequency from the SymbolTable
    stable_visit(st, &visit);

    //Print the words in the specified format and order
    /*for (int i=0; i<nwords; i++) {
        int tmp = -1;
        char aux[1] = {127}, *small = aux;
        for (int j=0; j<nwords; j++)
            if(copiaSt[j].p != 0 && !seen[j] && strcmp(small, copiaSt[j].p) > 0) {
                small = copiaSt[j].p;
                tmp = j;
            }

        int width = (int) (wide - strlen(copiaSt[tmp].p) + 1);
        fprintf(output, "%s %*d\n", copiaSt[tmp].p, width, copiaSt[tmp].freq);
        seen[tmp] = 1;
    }*/

    qsort(copiaSt, nwords, sizeof(words), compare);
    for (int i=0; i<nwords; i++) fprintf(output, "%s %*d\n", copiaSt[i].p, (int) (wide - strlen(copiaSt[i].p) + 1), copiaSt[i].freq);

    //Destroy the data structures used
    stable_destroy(st);
    buffer_destroy(B);
    return 0;
//
}


int max(int a, int b){return a > b ? a : b;}

/*
 * Function: visit
 * --------------------------------------------------------
 *   Receives an element from the table and store it in a matrix
 * @args key: String of the word
 *       data: The frequency of the current word
 * @return always 1, as we want to copy every element of the SymbolTable
 */
int visit(const char *key, EntryData *data){
    nwords++;
    copiaSt[ind].p = emalloc(strlen(key));
    strcpy(copiaSt[ind].p, key);
    copiaSt[ind].freq = data -> i;
    ind++;

    return 1;
}
