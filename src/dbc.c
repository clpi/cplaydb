#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <memory.h>
#include <strings.h>
#include "table.h"
#include "ops.h"
#include "table.h"
#include "dbc.h"


int main(int argc, char* argv[]) {
    if (argc < 2) {
        emsg((char*)"Must provide a new", (char*)"database name");
        exit(EXIT_FAILURE);
    }
    char* filen = argv[1];
    Table* t = db_open(filen);
    InputBuffer* input_buffer = new_input_buffer();    
    while (true) {
        prompt_input(input_buffer);
        Statement stmt;
        switch (prepare_statement(input_buffer, &stmt)) {
            case (PREPARE_SUCCESS): 
                switch (execute_statement(&stmt, t)) {
                    case (EXE_SUCCESS): break;
                    case (EXE_TABLE_FULL):
                        emsg("Unrecognized keyword at start of ", input_buffer->buf);
                        break;
                    default: break;
                }
                continue;
            case PREPARE_SYNTAX_ERROR:
                emsg((char*)"Syntax is invalid here, specifically: ", input_buffer->buf );
                continue;
            case PREPARE_NEG_ID:
                emsg((char*)"Id cannot be below 0, was given", input_buffer->buf );
                continue;
            case PREPARE_STRING_TOO_LONG:
                emsg((char*)"Input for prepare str is too long", input_buffer->buf);
                continue;
            case (PREPARE_UNRECOGNIZED_STATEMENT):
                emsg((char*)"Unrecognized keyword at start of", input_buffer->buf );
                continue;
            default: continue;
        }
    }
}
