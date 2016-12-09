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



/* Function Prototypes */
void addLine(Line **head, char *line, int number);
void evaluateText(Line *head);
void insert(char *ss);
bool isEmpty(char *str);
bool isExternOk(SymbolTable extern_table, SymbolTable label_table);
void destroyStables(SymbolTable a, SymbolTable b, SymbolTable c);
char *removeNL(char *str);
void unbranchInstructions(SymbolTable label_table, Instruction *head);

//TODO: Think what to do with the STR operation

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
    EntryData *ret;
    instHEAD = NULL;
    end = NULL;
    int parseResult;
    const char *errStr;
    Line *p, *ant;
    bool gotHead = false;
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
    while(!gotHead) {
        if(p != NULL) {
            parseResult =  parse(p->line, alias_table, &instHEAD, &errStr);
            if(parseResult == 0) { //Print error
                destroyStables(alias_table, extern_table, label_table);
                int iniLen = strlen(estrdup("\nline : "));
                for(int n = p->number; n; n/=10, ++iniLen);
                octa dist =abs((octa)(p->line - errStr));
                fprintf(stderr, "\nline %d: %s\n",p->number,removeNL(p->line));
                for(int k = 0; k < dist + iniLen - 1; fprintf(stderr, " "), k++);
                fprintf(stderr, "^\n");
                print_error_msg(NULL);
                return;
            }
            int opC = instHEAD->op->opcode;
            if(!isPseudo) {
                gotHead = true;
                instHEAD->lineno = p->number;
                instHEAD->pos = posC;
                if(instHEAD->label) {
                    ret = stable_find(alias_table, instHEAD->label);
                    if(ret)
                        die("Error on line %d: Label \"%s\" is already an alias!",
                        instHEAD->lineno, instHEAD->label);
                    ir = stable_insert(label_table, instHEAD->label);
                    if(ir.new == 0) {
                        die("Error on line %d: Label \"%s\" is already defined!",
                        instHEAD->lineno, instHEAD->label);
                    }
                    ir.data->i = posC;
                }
                if(opC == CALL)
                    posC += 4;
                else if(opC == PUSH)
                    posC += 2;
                else
                    posC++;
            }
            else if(opC == EXTERN) {
                ir = stable_insert(extern_table, instHEAD->opds[0]->value.label);
                ir.data->i = 0; // No value
            }
            if(isPseudo)
            instr_destroy(instHEAD);

            p = p->next;
        }
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
        parseResult =  parse(p->line, alias_table, &end, &errStr);
        if(parseResult == 0) {
            destroyStables(alias_table, extern_table, label_table);
            int iniLen = strlen(estrdup("\nline : "));
            for(int n = p->number; n; n/=10, ++iniLen);
            octa dist =abs((octa)(p->line - errStr));
            fprintf(stderr, "\nline %d: %s",p->number,removeNL(p->line));
            for(int k = 0; k < dist + iniLen - 1; fprintf(stderr, " "), k++);
            fprintf(stderr, "^\n");
            print_error_msg(NULL);
            return;
        }
        int opCode = end->op->opcode;
        if(parseResult == 1 && !isPseudo) {
            ptr->next = end;
            ptr->next->lineno = p->number;
            ptr = end;
            ptr->pos = posC;
            if(ptr->label) {
                ret = stable_find(alias_table, instHEAD->label);
                if(ret)
                    die("Error on line %d: Label \"%s\" is already an alias!",
                    instHEAD->lineno, instHEAD->label);
                ir = stable_insert(label_table, ptr->label);
                if(ir.new == 0) {
                    die("Error on line %d: Label \"%s\" is already defined!",
                    instHEAD->lineno, instHEAD->label);
                }
                ir.data->i = posC;
            }
            if(opCode == CALL)
                posC += 4;
            else if(opCode == PUSH)
                posC += 2;
            else
                posC++;
        }
        else if(opCode == EXTERN) {
            ir = stable_insert(extern_table, end->opds[0]->value.label);
            ir.data->i = 0; // No value
        }
        if(isPseudo)
            instr_destroy(instHEAD);
    }

    // Check if all extern labels were defined
    if(!isExternOk(extern_table, label_table)) {
        print_error_msg(NULL);
        destroyStables(alias_table, extern_table, label_table);
        return;
    }
    //TODO: Unbranch the Instruction linked list
    unbranchInstructions(label_table, instHEAD);
    stable_destroy(st);
}

/*
 * Function: unbranchInstructions
 * --------------------------------------------------------
 * Remove the pseudo-operators (PUSH and CALL) from the list,
 * and replace them with the full operations.
 *
 * @args    label_table : The symbol table with the labels
 *          head: The linked list of Instructions
 *
 * @return
 */
void unbranchInstructions(SymbolTable label_table, Instruction *head) {
    Instruction *p, *ant;
    int opCode;
    for(ant = NULL, p = head; ant = p, p = p->next) {
        opCode = p->op->opcode;
        /* Create an operand */
        Operand **vOps;
        vOps = emalloc(sizeof(Operand *));
        for(int i = 0; i < 3; i++)
            vOps[i] = NULL;
        if(opCode == CALL) {
            vOps[0] = operand_create_label("rZ");
            vOps[1] = operand_create_number((octa)4);
            instr_create(, optable_find("GETA"),vOps);
        }
        else if(opCode == PUSH) {

        }
    }

}

/*
 * Function: isExternOk
 * --------------------------------------------------------
 * Check if the labels exported via the EXTERN pseudo-operator were defined.
 *
 * @args    extern_table: A symbol table with the extern labels
 *          label_table:  A symbol table with all the labels of the instructions
 *
 * @return  true if no error, false otherwise
 */
bool isExternOk(SymbolTable extern_table, SymbolTable label_table) {
    StrStorage strS = stable_Keys(extern_table);
    EntryData *ret, *ret2;
    for(int i = 0; i < strS.i; i++) {
        ret = stable_find(label_table, strS.str[i]);
        if(ret) {
            ret2 = stable_find(extern_table, strS.str[i]);
            ret2->i = ret->i;
        }
        else {
            set_error_msg("Extern label \"%s\" is never defined!", strS.str[i]);
            return false;
        }
    }
    return true;
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

void destroyStables(SymbolTable a, SymbolTable b, SymbolTable c) {
    stable_destroy(a);
    stable_destroy(b);
    stable_destroy(c);
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