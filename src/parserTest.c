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
#include "../include/defaultops.h"

void printOperands(Operand **opds);


int main(int argc, char const *argv[]) {
    Buffer *B = buffer_create();
    SymbolTable st = stable_create();
    Instruction * instList;
    int parseResult;
    char *tmp;
    const char *errStr;
    //if (argc != 2)
    //    die("Wrong number of arguments, aborting...");
    FILE* input = fopen("input", "r");
    //FILE* input = fopen(argv[1], "r");
    if (input == NULL)
       die("Error opening file, aborting...");

    while (read_line(input, B)) {
        buffer_push_back(B,0);

        end = emalloc(sizeof(string));
        end -> s = NULL;
        end -> next = NULL;
        first = end;

        char *token = strtok (B -> data, ";");
        while (token != NULL) {
            insert(token);
            token = strtok (NULL, ";");
        }

        while (first -> next != NULL) {
            printf("Send = |%s|\n",(first -> next) -> s);
            parseResult = parse((first->next)->s, st, &instList, &errStr);
            if(parseResult) {
                printf("=============PARSED============\n");
                printf("LINE = %s\n",(first->next)->s);
                printf("LABEL = %s\n",(tmp = (instList->label ? instList->label : "n/a")));
                printf("OPERATOR = %s\n",instList->op->name);
                printOperands(instList->opds);
            }
            first = first->next;
            exit(1);
        }
        buffer_reset(B);
    }

    return 0;
}

void printOperands(Operand **opds) {
    printf("OPERANDS = ");
    for(int i = 0; i < 3; i++)
        switch (opds[i]->type) {
            case REGISTER:
                printf("Register(%d)\n",opds[i]->value.reg);
                break;
            case NUMBER_TYPE:
                printf("Number(%lli)\n",opds[i]->value.num);
                break;
            case LABEL:
                printf("Label(%s)\n",opds[i]->value.label);
                break;
            case STRING:
                printf("String(%s)\n",opds[i]->value.str);
                break;
            default:
                continue;
        }

}
