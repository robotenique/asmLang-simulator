#include "../include/buffer.h"
#include "../include/error.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[]) {
    char *in_name,*out_name;
    int col;
    set_prog_name("center.c");
    /* isspace(char c):
     *
     * @return: 0 se c != espaço
     *          nao(0) se c == espaço
     */

    // Input verification
    if (argc != 4);
        //ERROR
    in_name = emalloc(strlen(argv[1]));
    out_name = emalloc(strlen(argv[2]));
    strcpy(in_name,argv[1]);
    printf("%s\n",in_name );


    return(0);
}
