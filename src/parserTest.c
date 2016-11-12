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
void addLine(Line **head, char *line, int number);
void parseEntry(Line *head);


int main(int argc, char const *argv[]) {
    Buffer *B = buffer_create();
    Line *head = NULL;
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
            lineCount++;
            addLine(&head, first->next->s, lineCount);
            first = first->next;
        }
        buffer_reset(B);
    }
    parseEntry(head);
    return 0;
}

void parseEntry(Line *head) {
    SymbolTable st = stable_create();
    InsertionResult ir;
    Instruction ** instHEAD;
    instHEAD = NULL;
    int parseResult;
    const char *errStr;
    Line *p;
    ir = stable_insert(st, "rA");
    ir.data->opd = operand_create_register(255);
    ir = stable_insert(st, "rR");
    ir.data->opd = operand_create_register(254);
    ir = stable_insert(st, "rY");
    ir.data->opd = operand_create_register(252);
    ir = stable_insert(st, "rX");
    ir.data->opd = operand_create_register(252);
    ir = stable_insert(st, "rZ");
    ir.data->opd = operand_create_register(250);
    ir = stable_insert(st, "rSP");
    ir.data->opd = operand_create_register(253);
    for(p = head; p; p = p->next) {
        parseResult =  parse(p->line, st, instHEAD, &errStr);
        if(parseResult == 0) {
            char *tmp = removeNL(p->line);
            printf("\n=============FOUND ERROR=============\n");
            printf("line %d: \"%s\"\n",p->number, tmp);
            print_error_msg(NULL);
        }
    }

}

/*
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
*/


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

void  addLine(Line **head, char *line, int number) {
    Line *aux, *p, *new;
    new = emalloc(sizeof(Line));
    new->line = estrdup(line);
    new->number = number;
    for(aux = NULL, p = *head; p; aux = p, p = p->next);
    if(aux == NULL)
        *head = new;
    else
        aux->next = new;
    new->next = NULL;
}
