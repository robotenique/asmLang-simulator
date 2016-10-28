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
#include "../include/stable.h"

int main(int argc, char const *argv[]) {
    Buffer *B = buffer_create();
    SymbolTable st = stable_create();
    Instruction * instList;
    const char *errStr;
    //if (argc != 2)
    //    die("Wrong number of arguments, aborting...");
    FILE* input = fopen("input", "r");
    //FILE* input = fopen(argv[1], "r");
    if (input == NULL)
       die("Error opening file, aborting...");
    while (read_line(input, B)) {
        // TODO: Separar as strings pelo ';', e mandar
        // cada uma independentemente para o parser (Sem o ';')
        // Se ainda houver linha, adiciona-la
        buffer_push_back(B,0);
        parse(B->data, st, &instList, &errStr);
        buffer_reset(B);
        exit(-1);
    }
    return 0;
}
