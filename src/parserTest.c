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
void printAllList(Instruction *head, Line *headL, int times);
bool isEmpty(char *str);
int checkLabels(SymbolTable st, Instruction *head);
void insert(char *ss);


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
            if(!isEmpty(first->next->s))
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
    Instruction *instHEAD, *end, *ptr;
    instHEAD = NULL;
    end = NULL;
    int parseResult;
    const char *errStr;
    Line *p, *ant;
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
    p = head;
    // Get the head of the linked list
    if(p != NULL) {
        parseResult =  parse(p->line, st, &instHEAD, &errStr);
        if(parseResult == 0) {
            char *tmp = removeNL(p->line);
            printf("\n=============FOUND ERROR=============\n");
            printf("line %d: \"%s\"\n",p->number, tmp);
            print_error_msg(NULL);
            return;
        }
        instHEAD->lineno = p->number;
        p = p->next;
    }
    ptr = instHEAD;
    ant = p;
    // The end has the same pointer as the head
    for(; p; ant = p, p = p->next) {
        end = NULL;
        parseResult =  parse(p->line, st, &end, &errStr);
        if(parseResult == 0) {
            printAllList(instHEAD, head, ant->number);
            char *tmp = removeNL(p->line);
            printf("\nline %d: \"%s\"\n",p->number, tmp);
            print_error_msg(NULL);
            return;
        }
        else if(parseResult == 1) {
            ptr->next = end;
            ptr->lineno = p->number;
            ptr = end;
        }
    }
    // If there's no error, check if all the labels are in the SymbolTable!
    int lineError = checkLabels(st, instHEAD);
    if(lineError > 0) {
        for(p = head; p && p->number != lineError; p = p->next);
        if(p == NULL)
        if(p)
            printf("\nline %d: %s\n",p->number, removeNL(p->line));
        print_error_msg(NULL);
    }
    else {
        if(ant)
            printAllList(instHEAD, head, ant->number);
        else
            printAllList(instHEAD, head, 10);
    }
}

void printAllList(Instruction *head, Line *headL, int times) {
    Instruction *p;
    Line *q;
    p = head;
    q = headL;
    int i = 0;
    for(; p && q && i < times; p = p->next, q = q->next ,i++) {
        printf("\n");
        printf("LINE = %s\n",removeNL(q->line));
        printf("LABEL = %s\n",(p->label ? p->label : "n/a"));
        printf("OPERATOR = %s\n",p->op->name);
        printOperands(p->opds);
    }
}

void printOperands(Operand **opds) {
    printf("OPERANDS = ");
    int lim;
    for(lim = 0;lim < 3 && opds[lim]; lim++);
    for(int i = 0; i < 3; i++) {
        char *tmp;
        if(i == lim - 1) tmp = estrdup(";");
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

bool isEmpty(char *str) {
    int len = strlen(str);
    int i;
    for(i = 0; str[i] && isspace(str[i]); i++);
    if(i == len)
        return true;
    return false;
}

int checkLabels(SymbolTable st, Instruction *head) {
    Instruction *p;
    for(p = head; p; p = p->next) {
        if(isConditional(p->op)) {
            int nargs;
            for (nargs = 0; nargs < 3 && p->op->opd_types[nargs] != OP_NONE; ++nargs);
            if(nargs == 1 && stable_find(st,p->opds[0]->value.label) == NULL){
                set_error_msg("Error with %s operator: label \"%s\" never defined!",
                p->op->name, p->opds[0]->value.label);
                return p->lineno - 1;
            }
            else if(nargs == 2 && stable_find(st,p->opds[1]->value.label) == NULL) {
                set_error_msg("Error with %s operator: label \"%s\" never defined!",
                p->op->name, p->opds[1]->value.label);
                return p->lineno - 1;
            }
        }
    }
    return -1;
}

void insert(char *ss) {
    string *new = malloc(sizeof(string));
    new -> s = estrdup(ss);
    new -> next = NULL;
    end -> next = new;
    end = end -> next;
}
