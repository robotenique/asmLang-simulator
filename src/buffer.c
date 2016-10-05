/*
 * @author: Juliano Garcia de Oliveira
 * @author: Enzo Hideki Nakamura
 *
 * MAC0216
 *
 * Buffer Implementation
 */
#include "../include/buffer.h"
#include "../include/error.h"
#include <stdlib.h>


/* REMAINDER: The 0 represents the end of a string. Remember to use the
 * buffer_push_back() function to push the 0 when finished reading the
 * desired string.
 */
 Buffer *buffer_create() {
     Buffer *B = emalloc(sizeof(Buffer));
     B -> n = 1024;
     B -> i = 0;
     B -> data = emalloc(B -> n) ;
     return B;
 }

 void buffer_destroy(Buffer *B) {
     free(B -> data);
     free(B);
 }

 void buffer_reset(Buffer *B){
    free(B -> data);
    B -> data = emalloc(B -> n);
    B -> i = 0;
 }

 void buffer_push_back(Buffer *B, char c){
     B -> data[B->i++] = c;
 }

 int read_line(FILE *input, Buffer *B){
    char c;
    int count = 0;
    buffer_reset(B);

    while((c = fgetc(input)) != EOF && c != '\n') {
        B -> data[B -> i++] = c;
        count ++;
    }
    if (c == '\n') {
        B -> data[B -> i++] = c;
        count ++;
    }
    return count;
 }
