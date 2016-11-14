#include "../include/defaultops.h"
#include <stdlib.h>
bool isConditional(const Operator *op) {
    switch (op->opcode) {
        case CALL:
            return true;
        case GETA:
            return true;
        case GO:
            return true;
        case JMP:
            return true;
        case EXTERN:
            return true;
        case JN:
            return true;
        case JNN:
            return true;
        case JNP:
            return true;
        case JNZ:
            return true;
        case JP:
            return true;
        case JZ:
            return true;
        default:
            return false;
    }
}
char *cutSpc(char *text) {
   int length, c, d;
   char *start;
   c = d = 0;
   for(int i = 0; text[i]; text[i] = isspace(text[i]) ? ' ':text[i], i++);
   length = strlen(text);
   start = (char*)malloc(length+1);
   if (start == NULL)
      die("Error in memory allocation!");

    while (*(text+c) != '\0') {
      if (*(text+c) == ' ') {
         int temp = c + 1;
         if (*(text+temp) != '\0') {
            while (*(text+temp) == ' ' && *(text+temp) != '\0') {
               if (*(text+temp) == ' ') {
                  c++;
               }
               temp++;
            }
         }
      }
      *(start+d) = *(text+c);
      c++;
      d++;
   }
   *(start+d)= '\0';

   return trimSpc(trimComment(start));
}
char *trimSpc(char *c) {
    char * e = c + strlen(c) - 1;
    while(*c && isspace(*c)) c++;
    while(e > c && isspace(*e)) *e-- = '\0';
    return c;
}
char *trimComment(char *text) {
    int i;
    char *tmp;
    for(i = 0; text[i] && text[i] != '*'; i++);
    if(text[i] == '*') text[i] = 0;
    tmp = estrdup(text);
    for(i = 0; tmp[i]; i++)
      if(tmp[i] == '\t')
        tmp[i] = ' ';
    return tmp;
}


bool isValidChar(char c)  {
    return c && (isalnum(c) || c == '_');
}
