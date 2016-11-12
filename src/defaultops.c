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
