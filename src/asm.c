/*
 * @author: Juliano Garcia de Oliveira
 * @author: Enzo Hideki Nakamura
 *
 * MAC0216
 *
 * Assembler implementation
 */
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

void addLine(Line **head, char *line, int number);
void evaluateText(Line *head);


int assemble(const char *filename, FILE *input, FILE *output) {
    Buffer *B = buffer_create();
    Line *head = NULL;
    int lineCount = 0;

    // Create a linked list with all the lines in the input file
    while (read_line(input, B)) {
        buffer_push_back(B,0);
        // End is a global variable defined in defaultops.h
        end = emalloc(sizeof(string));
        end->s = NULL;
        end->next = NULL;
        first = end;
        char *token = strtok(B->data, ";");
        while (token != NULL) {
            insert(token);
            token = strtok(NULL, ";");
        }
        while (first->next != NULL) {
            lineCount++;
            if(!isEmpty(trimComment(first->next->s))) {
                addLine(&head,trimComment(first->next->s) , lineCount);
            }
            first = first->next;
        }
        buffer_reset(B);
    }
    evaluateText(head);
    buffer_destroy(B);
    return 0;


}


void evaluateText(Line *head) {
    //Create the three symbol tables
    SymbolTable alias_table = stable_create();
    SymbolTable extern_table = stable_create();
    SymbolTable label_table = stable_create();
    InsertionResult ir;
    Instruction *instHEAD, *end, *ptr;
    instHEAD = NULL;
    end = NULL;
    int parseResult;
    const char *errStr;
    Line *p, *ant;
    int posC = 1;
    // Insert the default labels into the symbol table
    ir = stable_insert(alias_table, "rA");
    ir.data->opd = operand_create_register(255);
    ir = stable_insert(alias_table, "rR");
    ir.data->opd = operand_create_register(254);
    ir = stable_insert(alias_table, "rY");
    ir.data->opd = operand_create_register(251);
    ir = stable_insert(alias_table, "rX");
    ir.data->opd = operand_create_register(252);
    ir = stable_insert(alias_table, "rZ");
    ir.data->opd = operand_create_register(250);
    ir = stable_insert(alias_table, "rSP");
    ir.data->opd = operand_create_register(253);
    p = head;
    // Get the head of the linked list
    if(p != NULL) {
        parseResult =  parse(p->line, alias_table, &instHEAD, &errStr);
        if(parseResult == 0) { //Print error
            stable_destroy(st);
            int iniLen = strlen(estrdup("\nline : "));
            for(int n = p->number; n; n/=10, ++iniLen);
            octa dist =abs((octa)(p->line - errStr));
            printf("\nline %d: %s\n",p->number,removeNL(p->line));
            for(int k = 0; k < dist + iniLen - 1; printf(" "), k++);
            printf("^\n");
            print_error_msg(NULL);
            return;
        }
        // TODO: ADD label into the label_table, add extern into the extern table, etc
        if(!isPseudo) {
            instHEAD->lineno = p->number;
            int opC = instHEAD->opr->opcode;
            instHEAD->pos = posC;
            if(instHEAD->label) {
                ir = stable_insert(label_table, instHEAD->label);
                if(ir == 0) {
                    //TODO: ERROR, THE LABEL IS ALREADY DEFINED
                    return;
                }
                ir.data->i = posC;
            }
            else if(opc == EXTERN) {
                ir = stable_insert(extern_table, instHEAD->label);
            }

            if(opC == CALL)
                posC += 4;

            else if(opC == PUSH)
                posC += 2;
            else
                posC++;
        }
        p = p->next;
    }
    // The end has the same pointer as the head
    ptr = instHEAD;
    ant = p;
    /* Get the parse result of each list. If there's an immediate error,
     * It stops the parsing and print the error, along with all the elements
     * before the error.
     */
    for(; p; ant = p, p = p->next) {
        end = NULL;
        parseResult =  parse(p->line, st, &end, &errStr);
        if(parseResult == 0) {
            printAllList(instHEAD, head, ant->number);
            stable_destroy(st);
            int iniLen = strlen(estrdup("\nline : "));
            for(int n = p->number; n; n/=10, ++iniLen);
            octa dist =abs((octa)(p->line - errStr));
            printf("\nline %d: %s",p->number,removeNL(p->line));
            for(int k = 0; k < dist + iniLen - 1; printf(" "), k++);
            printf("^\n");
            print_error_msg(NULL);
            return;
        }
        else if(parseResult == 1 && !isPseudo) {
            ptr->next = end;
            ptr->next->lineno = p->number;
            ptr = end;
        }
    }

    // If there's no error, check if all the labels are in the SymbolTable!
    int lineError = checkLabels(st, instHEAD);
    /* If there's a label which is not defined, print all the parsing
     * before the error, then print the line of the error and
     * the error message.
     */
    if(lineError > 0) {
        printAllList(instHEAD, head, lineError - 1);
        for(p = head; p && p->number != lineError; p = p->next);
        if(p)
            printf("\nline %d: %s\n",p->number, removeNL(p->line));
        print_error_msg(NULL);
    }
    else {
        // If there's more than 1 line, print everything up until the line
        if(ant)
            printAllList(instHEAD, head, ant->number);
        else //print the only line.
            printAllList(instHEAD, head, 10);
    }
    stable_destroy(st);

}




/*
 * Function: insert
 * --------------------------------------------------------
 * Simple function to insert a new string into the tokens linked list.
 *
 * @args    ss: A string
 *
 * @return
 */
void insert(char *ss) {
    string *new = malloc(sizeof(string));
    new -> s = estrdup(ss);
    new -> next = NULL;
    end -> next = new;
    end = end -> next;
}

/*
 * Function: addLine
 * --------------------------------------------------------
 * Add a new line to the linked list, with the correct line number
 *
 * @args    head: The head of the linked list
 *          line: The string of the line
 *          number: The number of the line
 *
 * @return
 */
void  addLine(Line **head, char *line, int number) {
    Line *aux, *p, *new;
    // Create a new Line
    new = emalloc(sizeof(Line));
    new->line = estrdup(line);
    new->number = number;
    // Insert it into the ending of the linked list
    for(aux = NULL, p = *head; p; aux = p, p = p->next);
    if(aux == NULL)
        *head = new;
    else
        aux->next = new;
    new->next = NULL;
}
