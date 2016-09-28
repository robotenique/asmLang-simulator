#include "../include/buffer.h"
#include "../include/error.h"
#include "../include/hash.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char word[50], table[10000000][50], freq[10000000], seen[10000000];
int keys[10000000], nwords;

int max(int a, int b){return a > b ? a : b;}

int main(int argc, char **argv){
    Buffer *B = buffer_create();
    
    FILE* input = fopen(argv[1], "r");
    FILE* output = fopen(argv[2], "a");

    int wide = 0;

    memset(freq,0,sizeof(freq));

    if (input == NULL || output == NULL)
       die("Error opening files, aborting...");


    while (read_line(input,B)) {
        int i;
        for (i = 0; isspace(B->data[i]) && B->data[i]!=0; i++);

        while (B->data[i]!=0){
            memset(word,0,sizeof(word));

            int j = 0;
            while (!isspace(B->data[i]) && B->data[i] != 0 && i < (B -> i)) 
                word[j++] = B->data[i++];

            i++;

            int h = hash(word);

            while (strcmp(table[h], word) != 0)
                if (*table[h] == 0) {
                    strcpy(table[h],word);
                    wide = max(wide, strlen(table[h]));
                    keys[nwords++] = h;
                } else h = (h + 1) % MAXTABLE;

            freq[h]++;
        }
    }


    

    for (int i=0; i<nwords; i++) {
        int ind = -1;
        char aux[1] = {127}, *small = aux;
        for (int j=0; j<nwords; j++)
            if(*table[keys[j]] != 0 && !seen[keys[j]] && strcmp(small, table[keys[j]]) > 0) {
                small = table[keys[j]];
                ind = keys[j];
            }

        int width = (int) (wide - strlen(table[ind]) + 1);
        fprintf(output, "%s %*d\n", table[ind], width, freq[ind]);
        seen[ind] = 1;
    }

    return 0;
}