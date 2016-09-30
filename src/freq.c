#include "../include/buffer.h"
#include "../include/error.h"
#include "../include/hash.h"
#include "../include/stable.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char word[50], table[10000000][50], seen[10000000];
int keys[10000000], nwords = 0;

int wide = 0, ind = 0;

typedef struct {
    char* p;
    int freq;
} words;

words copiaSt[10000000];

int max(int a, int b){return a > b ? a : b;}

int visit(const char *key, EntryData *data){
    nwords++;
    copiaSt[ind].p = malloc(strlen(key));
    strcpy(copiaSt[ind].p, key);
    copiaSt[ind].freq = data -> i;
    ind++;

    return 1;
}

int main(int argc, char **argv){
    Buffer *B = buffer_create();

    SymbolTable st = stable_create();

    FILE* input = fopen(argv[1], "r");
    FILE* output = fopen(argv[2], "a");
    


    if (input == NULL || output == NULL)
       die("Error opening files, aborting...");

    while (read_line(input,B)) {
        int i;
        for (i = 0; isspace(B->data[i]) && B->data[i]!=0; i++);

        while (B->data[i]!=0) {
            memset(word,0,sizeof(word));

            int j = 0;
            while (!isspace(B->data[i]) && B->data[i] != 0 && i < (B -> i))
                word[j++] = B->data[i++];
//ab     c
            wide = max(wide, strlen(word));

            i++;

            int h = hash(word);

            InsertionResult ir = stable_insert(st, word);

            if (h!=0) (*ir.data).i = 1 + (!ir.new * (*ir.data).i);
        }
    }

    stable_visit(st, &visit);

    for (int i=0; i<nwords; i++) {
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
    }

    return 0;
}
