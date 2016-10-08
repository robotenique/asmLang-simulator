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
#include <mcheck.h>


typedef struct {
    char* p;
    int freq;
} words;
char word[80];
words copiaSt[HASH_SIZE];
int wide = 0, ind = 0, nwords = 0;

int max(int a, int b);
int visit(const char *key, EntryData *data);
int compare (const void * a, const void * b);

int main(int argc, char **argv) {
    //Better alternative to verify heap memory consistency
    mcheck(0);
    Buffer *B = buffer_create();
    SymbolTable st = stable_create();
    // Input verification
    if (argc != 2)
        die("Wrong number of arguments, aborting...");
    //FILE* input = fopen("input", "r");
    FILE* input = fopen(argv[1], "r");
    if (input == NULL)
       die("Error opening file, aborting...");

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
    qsort(copiaSt, nwords, sizeof(words), compare);
    for (int i=0; i<nwords; i++) printf("%s %*d\n", copiaSt[i].p, (int) (wide - strlen(copiaSt[i].p) + 1), copiaSt[i].freq);

    //Destroy the data structures used
    stable_destroy(st);
    buffer_destroy(B);
    return 0;
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

/*
 * Function: compare
 * --------------------------------------------------------
 *   Receives two pointers and compare them lexicographically, according to
 * their words.
 * @args a: First pointer
 *       b: Second pointer
 * @return  < 0, if 'a' has a word less than the word of 'b'.
 *          > 0, if 'b' has a word less than the word of 'a'.
 *          = 0, if 'a' has a word equal to the word of 'b'.
 */
int compare (const void * a, const void * b){
  return strcmp(((words *)a)->p, ((words *)b)->p);
}
