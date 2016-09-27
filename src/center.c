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
    if (argc != 4);
        //ERROR
    if ((col = atoi(argv[3])));
        //ERROR

    input = fopen(argv[1],"r");
    output = fopen(argv[2],"w");
    if (input == NULL);
        //ERROR

    while(read_line(input,B)) {
        int i, j;
        buffer_push_back(B,0);
        if(nLine) fprintf(output,"\n");
        nLine++;
        for (i = 0;isspace(B->data[i]) && B->data[i]!=0; i++);
        for (j =(B->i) - 2;isspace(B->data[j]) && j >= i; j--);
        if(i == (B-> i)- 1)
            printf("%s",B->data);
        else if((j - i + 1) > col)
            for (int p = i; p <= j; fprintf(output, "%c",B->data[p]), p++);
            //ERROR (print nLine + "line too long") (IN THE STDERR)
        else centralizeLine(B, col, i, j, output);
    }
    return(0);
}


void centralizeLine (Buffer *B, int col, int i, int j, FILE *output) {
    for (int x = 0; x < (col - (j-i+1))/2 ; fprintf(output, " "), x++);
    for (int w = i; w <= j; fprintf(output, "%c",B->data[w]) , w++);
}
