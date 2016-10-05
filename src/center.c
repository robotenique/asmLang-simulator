/*
 * @author: Juliano Garcia de Oliveira
 * @author: Enzo Hideki Nakamura
 *
 * MAC0216
 *
 * Buffer Test : Centralize text
 */
#include "../include/buffer.h"
#include "../include/error.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void centralizeLine (Buffer *B, int col, int i, int j, FILE *output);

int main(int argc, char const *argv[]) {
    /* isspace(char c):
     *
     * @return: 0 se c != espaço
     *          nao(0) se c == espaço
     */
    FILE *input, *output;
    int col, nLine = 0;
    Buffer *B = buffer_create();
    set_prog_name("center.c");


    // Input verification
    if (argc != 4)
        die("Wrong number of arguments, aborting...");

    if (!(col = atoi(argv[3])))
       die("Can't process the column input, aborting...");

    input = fopen(argv[1],"r");
    output = fopen(argv[2],"a");
    if (input == NULL || output == NULL)
       die("Error opening files, aborting...");

    while(read_line(input,B)) {
        int i, j;
        buffer_push_back(B,0);
        if(nLine) fprintf(output,"\n");
        nLine++;
        for (i = 0;isspace(B->data[i]) && B->data[i]!=0; i++);
        for (j =(B->i) - 2;isspace(B->data[j]) && j >= i; j--);
        if(i == (B-> i)- 2)
            printf("%s",B->data);
        else if((j - i + 1) > col) {
            for (int p = i; p <= j; fprintf(output, "%c",B->data[p]), p++);
            print_error_msg("line %d: Line too long\n", nLine);
        }

        else centralizeLine(B, col, i, j, output);
    }

    buffer_destroy(B);
    return(0);
}

/*
 * Function: centralizeLine
 * --------------------------------------------------------
 *   Receives a string and column number, then print the string
 *   centralized in the output file given.
 * @args B: Buffer of the read line
 *       col: Number of columns to center the line
 *       i: Where the line begins
 *       j: Where the line ends
 *       output: FILE to print the centralized line

 * @return
 */
void centralizeLine (Buffer *B, int col, int i, int j, FILE *output) {
    for (int x = 0; x < (col - (j-i+1))/2 ; fprintf(output, " "), x++);
    for (int w = i; w <= j; fprintf(output, "%c",B->data[w]) , w++);
}
