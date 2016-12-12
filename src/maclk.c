/*
    n. Allan Rocha
    n. Enzo Hideki
    n. Juliano Garcia

    ---

    maclk.c
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../include/buffer.h"
#include "../include/error.h"
#include "../include/stable.h"

int read_word(const char *line, Buffer *B)
{   /*Read a word from the 'line' and put it in the buffer 'B'. Return the size
    of the word.*/
    char c;
    int i = 0;

    buffer_reset(B);

    c = line[i];

    do {
        buffer_push_back(B, c);
        i = i + 1;
        c = line[i];
    } while (!isspace(c) && c != EOF);

    buffer_push_back(B, 0);

    return (B->i-1);
}

FILE **openFiles(int argc, char *argv[])
{   /*Abre todos os 'argc' arquivos de nomes contidos em 'argv'.*/
    FILE *inputs[] = emalloc((argc - 1)*sizeof(FILE *));
    for (int i = 1; i < argc; i++)
        if (!(inputs[i-1] = fopen(argv[i], "r"))) return NULL;

    return inputs;
}

char *what_jmp(char *word)
{   /*Verifica qual o código correspondente ao JMP específico contido em 'word.'*/
    if (!strcmp(word, "JMP")) return "48"
    else if (!strcmp(word, "JZ")) return "4a"
    else if (!strcmp(word, "JNZ")) return "4c"
    else if (!strcmp(word, "JP")) return "4e"
    else if (!strcmp(word, "JN")) return "58"
    else if (!strcmp(word, "JNN")) return "5a"
    else return "5c" /*JNP*/
}

int main(int argc, char *argv[])
{
    int wordlen, num, main, numline, jmpexist, numjmp = 0;
    int numinstr[argc - 1];
    char jmp[6];
    char opcode[2];
    FILE *inputs[];
    FILE *output;
    Buffer *word = buffer_create();
    Buffer *wordtmp = buffer_create();
    Buffer *line = buffer_create();
    InsertionResult result; EntryData data;
    SymbolTable extern_table = stable_create();

    set_prog_name("Linker");

    /*Verifica se os arquivos estão corretos.*/
    if (!(inputs = openFiles(argc, argv))) {
        set_error_msg("Error: invalid argument(s).");
        die(NULL);
    }

    /*Verifica se existem rótulos do EXTERN repetidos*/
    for (int i = 0, main = 0; i < argc - 1; i++) {
        while (read_line(inputs[i], line) && line->data[0] != 'B') {
            if (line->data[0] == 'E') {
                /*Coloca o rótulo na tabela de símbolos com a posição da instrução.*/
                wordlen = read_word(*line->data[2], word);
                if (!strcmp(word->data, "main")) main = 1;
                if (!(result = stable_insert(extern_table, word->data))) {
                    set_error_msg("Error: duplicate extern found.");
                    die(NULL);
                }
                else {
                    read_word(line->data[2+wordlen+1], word);
                    num = atoi(word->data);
                    *(result.data) = num;
                }
            }
            else numinstr[i] = atoi(line->data); /*Número de instruções*/
        }
    }

    /*Verifica se a main foi definida.*/
    if (!main) {
        set_error_msg("Error: main not defined.");
        die(NULL);
    }

    /*Fecha os arquivos e os abre de novo.*/
    for (int i = 0; i < argc - 1; i++) fclose(inputs[i]);
    output = fopen(argv[0], "a+");
    inputs[0] = fopen(argv[1], "r");
    while (read_line(inputs[0], line) && line->data[0] != 'B');

    /*Coloca a primeira instrução - JMP para a main.*/
    data = stable_find(extern_table, "main") + 1;
    if (*data > 16777215) {
        set_error_msg("Error: JMP is very large.");
        die(NULL);
    }
    sprintf(jmp, "%06x\n", *data);
    fprintf(output, strcat("48", jmp));

    /*Junta todos os códigos-objetos em um só e resolve as pendências.*/
    for (int k = 0, numline = 1; k < argc - 1; k++) {
        /*Em cada iteração nova, um código-objeto diferente.*/
        while (read_line(inputs[k], line)) {
            numline++; /*Nova linha.*/
            if (line->data[0] == '*') {
                /*Instrução pendente.*/

                /*Construção da instrução sem o número de instruções a ser pulada.*/
                wordlen = read_word(*line->data[2], word);
                opcode = what_jmp(word->data);
                if (strcmp(opcode, "JMP")) {
                    /*JZ, JNZ, JP, JNN, ... (menos o JMP).*/
                    read_word(*line->data[2+wordlen+1], word);
                    strcat(opcode, word->data);
                    read_word(*line->data[2+wordlen+1+3], word);
                }
                else read_word(*line->data[6], word);

                jmp = 0; /*Sinaliza se há um extern relacionado ao JMP pendente.*/

                /*Procura o rótulo para o qual o fluxo do programa desviará.*/
                for (int i = 0; i < argc - 1 && !jmp; i++) {
                    if (i == k) continue; /*Ignora o JMP pro próprio código-objeto.*/

                    inputs[i] = fopen(argv[i+1], "r");
                    while (read_line(inputs[i], line) && line->data[0] != 'E');

                    /*Verifica se o rótulo está no código-objeto desta iteração.*/
                    do {
                        read_word(line->data[2], wordtmp);
                        if (!strcmp(word->data, wordtmp->data)) {
                            /*O rótulo foi encontrado.*/

                            /*Cálculo do pulo.*/
                            data = stable_find(extern_table, word->data);
                            if (i > k) {
                                numjmp = numinstr[0] - numline;
                                for (int j = k + 1; j < i; numjmp += numinstr[j++]);
                                numjmp += (*data + 1);
                            }
                            else {
                                numjmp = numline - 1;
                                for (int j = k - 1; j > i; numjmp += numinstr[j++]);
                                numjmp += (numinstr[i] - *data);
                            }

                            /*Verifica se o pulo é possível.*/
                            if (strcmp(opcode, "JMP")) {
                                if (numjmp > 65535) {
                                    set_error_msg("Error: JMP is very large.");
                                    die(NULL);
                                }
                                sprintf(numjmp, "%04x\n", numjmp);
                            }
                            else {
                                if (numjmp > 16777215) {
                                    set_error_msg("Error: JMP is very large.");
                                    die(NULL);
                                }
                                sprintf(numjmp, "%06x\n", numjmp);
                            }

                            /*Termina de construir a instrução.*/
                            strcat(opcode, numjmp);
                            opcode[strlen(opcode)-1] = '\n';
                            if (i < k) opcode[1]++;
                            fprintf(output, opcode);
                            jmp = 1;
                            break;
                        }
                    } while (read_line(inputs[i], line) && line->data[0] != 'B');
                    fclose(inputs[i]);
                }
                if (!jmp) {
                    set_error_msg("Error: JMP not defined.");
                    die(NULL);
                }
            }
            else fprintf(output, line->data);
        }
        numline = 0;
    }

    buffer_destroy(word);
    buffer_destroy(wordtmp);
    buffer_destroy(wordline);
    stable_destroy(extern_table);

    return 0;
}
