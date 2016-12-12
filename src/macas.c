#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/error.h"
#include "../include/asm.h"

int main(int argc, char const *argv[]) {
    set_prog_name("macas");
    if (argc != 2)
        die("Wrong number of arguments, aborting...");
    FILE* input = fopen(argv[1], "r");
    char *aux = emalloc(sizeof(argv[1])+6);
    aux = estrdup(argv[1]);
    strcat(aux, ".maco");
    FILE* output = fopen(aux, "w");
    if (input == NULL)
        die("Error opening file, aborting...");
    if(output == NULL)
        die("Error creating file, aborting...");

    
    assemble(estrdup(argv[1]), input, output);


    return 0;
}
