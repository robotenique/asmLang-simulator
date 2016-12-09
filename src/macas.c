#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/buffer.h"
#include "../include/error.h"
#include "../include/asmtypes.h"
#include "../include/mactypes.h"
#include "../include/opcodes.h"
#include "../include/optable.h"
#include "../include/parser.h"
#include "../include/stable.h"
#include "../include/defaultops.h"
#include "../include/asm.h"

int main(int argc, char const *argv[]) {
    //set_prog_name("macas");
    if (argc != 3)
        die("Wrong number of arguments, aborting...");
    FILE* input = fopen(argv[1], "r");
    FILE* output = fopen(argv[2], "w");
    if (input == NULL)
       die("Error opening file, aborting...");
    if(output == NULL)
        die("Error opening file, aborting...");

    // TODO: Check errors
    assemble(estrdup(argv[1]), input, output);


    return 0;
}
