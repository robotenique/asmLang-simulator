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
char *removeNL(char *str);

typedef Instruction **pointer;

int main(int argc, char const *argv[]) {
    Buffer *B = buffer_create();
    SymbolTable st = stable_create();
    Instruction ** instList;
    instList = emalloc(sizeof(Instruction*));
    int parseResult;
    char *tmp = estrdup("");
    const char *errStr;
    set_prog_name("parser");
    //if (argc != 2)
    //    die("Wrong number of arguments, aborting...");
    FILE* input = fopen("input", "r");
    //FILE* input = fopen(argv[1], "r");
    if (input == NULL)
       die("Error opening file, aborting...");
    int lineCount = 0;
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
            printf("\n++++++++++++++++++++++++++++++++++++++++++++++++\n");
            printf("Send = |%s|\n",(first -> next) -> s);
            parseResult = parse((first->next)->s, st, instList, &errStr);
            lineCount++;
            if(parseResult) {
                if(*instList) {
                    tmp = removeNL((first->next)->s);
                    printf("=============PARSED============\n");
                    printf("LINE = %s\n",tmp);
                    printf("LABEL = %s\n",(tmp = ((*instList)->label ? (*instList)->label : "n/a")));
                    printf("OPERATOR = %s\n",(*instList)->op->name);
                    printOperands((*instList)->opds);
                }
            }
            else {
                tmp = removeNL((first->next)->s);
                printf("\n=============FOUND ERROR=============\n");
                printf("line %d: \"%s\"\n",lineCount, tmp);
                print_error_msg(NULL);
                set_error_msg("NULL");
            }
            first = first->next;

        }
        buffer_reset(B);
    }
    return 0;
}

void printOperands(Operand **opds) {
    printf("OPERANDS = ");
    int lim;
    for(lim = 0; opds[lim]; lim++);
    for(int i = 0; i < 3; i++) {
        char *tmp;
        if(i == lim - 1) tmp = ";";
        else tmp = estrdup(", ");
        if(opds[i] != NULL) {
            switch (opds[i]->type) {
                case REGISTER:
                    printf("Register(%d)%s",opds[i]->value.reg, tmp);
                    break;
                case NUMBER_TYPE:
                    printf("Number(%lli)%s",opds[i]->value.num, tmp);
                    break;
                case LABEL:
                    printf("Label(%s)%s",opds[i]->value.label, tmp);
                    break;
                case STRING:
                    printf("String(%s)%s",opds[i]->value.str, tmp);
                    break;
                default:
                    continue;
            }
        }
    }
    printf("\n");

}

char *removeNL(char *str) {
    char *tmp = estrdup(str);
    for(int i = strlen(tmp); i > 0; i--)
        if(tmp[i] == '\n'){
            tmp[i] = 0;
            break;
        }
    return tmp;
}
