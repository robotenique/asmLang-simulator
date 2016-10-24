/*
 * @author: Juliano Garcia de Oliveira
 * @author: Enzo Hideki Nakamura
 *
 * MAC0216
 *
 * Parser test
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <mcheck.h>
#include "../include/buffer.h"
#include "../include/error.h"
#include "../include/hash.h"
#include "../include/asmtypes.h"
#include "../include/mactypes.h"
#include "../include/opcodes.h"
#include "../include/optable.h"
#include "../include/parser.h"
int main(int argc, char const *argv[]) {
    if (argc != 2)
        die("Wrong number of arguments, aborting...");
    //FILE* input = fopen("input", "r");
    FILE* input = fopen(argv[1], "r");
    if (input == NULL)
       die("Error opening file, aborting...");
    return 0;
}
