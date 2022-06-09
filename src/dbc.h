#pragma once

#include <stdlib.h>
#include <stdio.h>

void emsg(char* msg1, char* msg2) {
    printf("\x1b[31;1mError:\x1b[0m\x1b[31m %s \x1b[33m'%s'\x1b[0m\n", msg1, msg2);
}
