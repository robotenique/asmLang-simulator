/*
 * @author: Juliano Garcia de Oliveira
 * @author: Enzo Hideki Nakamura
 *
 * MAC0216
 *
 * Parser test implementation
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

// Functions prototype
void printOperands(Operand **opds);
char *removeNL(char *str);
void addLine(Line **head, char *line, int number);
void parseEntry(Line *head);
void printAllList(Instruction *head, Line *headL, int times);
bool isEmpty(char *str);
int checkLabels(SymbolTable st, Instruction *head);
void insert(char *ss);


/*
 * Function: main
 * --------------------------------------------------------
 * Open the file read from the CLI, then creates a linked list
 * with all the lines of the file, also separating lines that have ';' into
 * different lines. Then, it sends the list to the function parseEntry.
 *
 * @args    argc: Number of args
 *          argv: Array with the args
 *
 * @return default return
 */
int main(int argc, char const *argv[]) {
    Buffer *B = buffer_create();
    Line *head = NULL;
    set_prog_name("parser");
    if (argc != 2)
        die("Wrong number of arguments, aborting...");
    FILE* input = fopen(argv[1], "r");
    if (input == NULL)
       die("Error opening file, aborting...");
    int lineCount = 0;
    while (read_line(input, B)) {
        buffer_push_back(B,0);
        end = emalloc(sizeof(string));
        end->s = NULL;
        end->next = NULL;
        first = end;
        char *token = strtok (B->data, ";");
        while (token != NULL) {
            insert(token);
            token = strtok (NULL, ";");
        }
        while (first->next != NULL) {
            lineCount++;
            if(!isEmpty(trimComment(first->next->s)))
                addLine(&head, trimComment(first->next->s), lineCount);
            first = first->next;
        }
        buffer_reset(B);
    }
    parseEntry(head);
    buffer_destroy(B);
    return 0;
}

/*
 * Function: parseEntry
 * --------------------------------------------------------
 * Parses the lines in the linked list, then print the Instr. as defined.
 * If there's an error, prints only up to the error.
 *
 * @args    head: Pointer to the head of the line linked list
 *
 * @return
 */
void parseEntry(Line *head) {
    SymbolTable st = stable_create();
    InsertionResult ir;
    Instruction *instHEAD, *end, *ptr;
    instHEAD = NULL;
    end = NULL;
    int parseResult;
    const char *errStr;
    Line *p, *ant;
    // Insert the default labels into the symbol table
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
        instHEAD->lineno = p->number;
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
        else if(parseResult == 1) {
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
 * Function: printAllList
 * --------------------------------------------------------
 * Print all Instructions in the linked list or until the line number is
 * less than or equals "times".
 *
 * @args    head: Head of the linked list with the instructions
 *          headL: Head of linked list with the lines (Strings)
 *          times: Max number of line to print the list
 *
 * @return
 */
void printAllList(Instruction *head, Line *headL, int times) {
    Instruction *p;
    Line *q;
    p = head;
    q = headL;
    for(;p && q && p->lineno <= times; p = p->next, q = q->next) {
        printf("\n");
        printf("LINE = %s\n",removeNL(q->line));
        printf("LABEL = %s\n",(p->label ? p->label : "n/a"));
        printf("OPERATOR = %s\n",p->op->name);
        printOperands(p->opds);
    }
}

/*
 * Function: printOperands
 * --------------------------------------------------------
 * Print the operands with the right string and formatting.
 *
 * @args    opds: Array with the operands of a instruction
 *
 * @return
 */
void printOperands(Operand **opds) {
    printf("OPERANDS = ");
    int lim;
    // Get the number of operands
    for(lim = 0;lim < 3 && opds[lim]; lim++);
    for(int i = 0; i < 3; i++) {
        char *tmp;
        // Set the ending string for a pretty print
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

/*
 * Function: removeNL
 * --------------------------------------------------------
 * Remove the first '\n' from the ending of the string, and return it.
 *
 * @args    str: A string
 *
 * @return A string without a newline at the ending.
 */
char *removeNL(char *str) {
    char *tmp = estrdup(str);
    for(int i = strlen(tmp); i > 0; i--)
        if(tmp[i] == '\n'){
            tmp[i] = 0;
            break;
        }
    return tmp;
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

/*
 * Function: isEmpty
 * --------------------------------------------------------
 * Check if a string is made of spaces only
 *
 * @args    str: A string
 *
 * @return true if the string has only spaces, false if not
 */
bool isEmpty(char *str) {
    int len = strlen(str);
    int i;
    for(i = 0; str[i] && isspace(str[i]); i++);
    if(i == len)
        return true;
    return false;
}

/*
 * Function: checkLabels
 * --------------------------------------------------------
 * Check if all the labels used in the conditional operators (JP, JMP, ...)
 * and the EXTERN were defined in the scope of the program. This is necessary
 * because the function "parse" only receives one line per call, so without
 * this function it wouldn't be possible to check, for instance, if a label
 * referenced by the EXTERN operator was defined.
 *
 * @args    st: The SymbolTable with all the labels
 *          head: The head of the linked list with the instructions
 *
 * @return  -1 if there's no error. If found an error, return the number of the
 *          line in which the error was found.
 */
int checkLabels(SymbolTable st, Instruction *head) {
    Instruction *p;
    EntryData *dt;
    for(p = head; p; p = p->next) {
        if(isConditional(p->op) && p->op->opcode == EXTERN) {
            int nargs;
            for (nargs = 0; nargs < 3 && p->op->opd_types[nargs] != OP_NONE; ++nargs);
            if(nargs == 1 && p->opds[0]->type == LABEL){
                dt = stable_find(st,p->opds[0]->value.label);
                if(dt == NULL || dt->opd->type != LABEL) {
                    set_error_msg("Error with %s operator: label \"%s\" never defined!",
                    p->op->name, p->opds[0]->value.label);
                    return p->lineno;
                }
            }
        }
    }
    return -1;
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
