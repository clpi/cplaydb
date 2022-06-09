#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <memory.h>
#include <strings.h>
#include "table.h"
#include "dbc.h"

typedef enum {
    MCMD_SUCCESS = 0,
    MCMD_UNRECOGNIZED_COMMAND = -1,
} MetaCommandResult;

typedef enum {
    PREPARE_SUCCESS = 0,
    PREPARE_UNRECOGNIZED_STATEMENT = -1,
    PREPARE_SYNTAX_ERROR = -2,
    PREPARE_STRING_TOO_LONG = -3,
    PREPARE_NEG_ID = -4,
} PrepareResult;

typedef enum OpResult {
    OP_SUCCESS = 0,
    OP_FAILED_TABLE_FULL = -1,
} OpResult;

typedef enum {
    STATEMENT_INS = 1,
    STATEMENT_SEL = 2,
    STATEMENT_DEL = 3,
    STATEMENT_SET = 4,
    STATEMENT_HELP = 5,
    STATEMENT_LIST = 6,
    STATEMENT_FIND = 7,
} StatementType;

typedef struct {
    StatementType type;
    Row inserting;
} Statement;

typedef struct {
    char* buf;
    size_t buflen;
    ssize_t inplen;
} InputBuffer;

typedef enum {
    EXE_SUCCESS = 0,
    EXE_TABLE_FULL = -1,
} ExecutionResult;

ExecutionResult execute_select(Statement* s, Table* t) { 
    Row row;
    for (uint32_t i = 0; i < t->rowlen; i++) {
        deserialize_row(rowslot(t, i), &row);
        printrow(&row);
    }
    return EXE_SUCCESS; 
}
ExecutionResult execute_insert(Statement* s, Table* t) { 
    if (t->rowlen >= TABLE_MAX_ROWS) 
        return EXE_TABLE_FULL;
    Row* to_ins = &(s->inserting);
    serialize_row(to_ins, rowslot(t, t->rowlen));
    t->rowlen += 1;
    return EXE_SUCCESS; 
}
ExecutionResult execute_help(Statement* s, Table* t) { return EXE_SUCCESS; }
ExecutionResult execute_find(Statement* s, Table* t) { return EXE_SUCCESS;}
ExecutionResult execute_del(Statement* s, Table* t) { return EXE_SUCCESS;}
ExecutionResult execute_set(Statement* s, Table* t) { return EXE_SUCCESS;}
ExecutionResult execute_list(Statement* s, Table* t) { return EXE_SUCCESS;}

ExecutionResult exe_stmt(Statement* stmt, Table *tbl) {
    switch (stmt->type) {
        case STATEMENT_INS:
            return execute_insert(stmt, tbl);
        case STATEMENT_SEL:
            return execute_select(stmt, tbl);
        case STATEMENT_HELP:
            return execute_help(stmt, tbl);
        case STATEMENT_LIST:
            return execute_list(stmt, tbl);
        case STATEMENT_DEL:
            return execute_del(stmt, tbl);
        case STATEMENT_SET:
            return execute_set(stmt, tbl);
        case STATEMENT_FIND:
            return execute_find(stmt, tbl);
        default: return EXE_TABLE_FULL;
    }
}

InputBuffer* new_input_buffer() {
    InputBuffer* inbuf = (InputBuffer*)malloc(sizeof(InputBuffer));
    inbuf->buf = NULL;
    inbuf->buflen = inbuf->inplen = 0;
    return inbuf;
}

void close_input_buffer(InputBuffer* in) {
    free(in->buf);
    free(in);
}

void prompt_input(InputBuffer* in) {
    printf("\x1b[32mdb \x1b[37;1m>_ \x1b[0m");
    ssize_t bytes_read = getline(&(in->buf), &(in->buflen), stdin);
    if (bytes_read <= 0) {
        emsg((char*)"Error reading input", (char*)"Or non given");
        exit(-1);
    }
    in->inplen = bytes_read - 1;
    in->buf[bytes_read - 1] = 0;
}

/* NOTE: */
InputBuffer* new_input_buffer(); 

/* NOTE: */
void close_input_buffer(InputBuffer* in); 

/* NOTE: */
void prompt_input(InputBuffer* in);

// ExecutionResult await_input;

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* st) {
    if (strncmp(input_buffer->buf, "insert", 6) == 0) {
        st->type = STATEMENT_INS;
        int args = sscanf(
            input_buffer->buf, "insert %d %s %s", 
                &(st->inserting.id),
                st->inserting.username, st-> inserting.email);
        if (args < 3) return PREPARE_SYNTAX_ERROR;
        return PREPARE_SUCCESS;
    } else if (strncmp(input_buffer->buf, "select", 6) == 0) {
        st->type = STATEMENT_SEL;
        return PREPARE_SUCCESS;
    } else if (strncmp(input_buffer->buf, "delete", 6) == 0) {
        st->type = STATEMENT_DEL;
        return PREPARE_SUCCESS;
    } else if (strncmp(input_buffer->buf, "set", 3) == 0) {
        st->type = STATEMENT_SET;
        return PREPARE_SUCCESS;
    } else if ((strncmp(input_buffer->buf, "ls", 2) == 0)
            || (strcmp(input_buffer->buf, "list") == 0)) {
        st->type = STATEMENT_LIST;
        return PREPARE_SUCCESS;
    } else if ((strncmp(input_buffer->buf, "f", 1) == 0)
            || (strcmp(input_buffer->buf, "find") == 0)) {
        st->type = STATEMENT_FIND;
        return PREPARE_SUCCESS;
    } else if ((strncmp(input_buffer->buf, "h", 1) == 0)
            || (strcmp(input_buffer->buf, "help") == 0)) {
        st->type = STATEMENT_HELP;
        return PREPARE_SUCCESS;
    } else if ((strcmp(input_buffer->buf, "exit") == 0)
            || (strcmp(input_buffer->buf, "quit") == 0)) {
        close_input_buffer(input_buffer);
        exit(0);
    } 
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

MetaCommandResult do_meta_cmd(InputBuffer* in) {
    if (strcmp(in->buf, ".exit") == 0) 
        exit(0);
    else
        return MCMD_UNRECOGNIZED_COMMAND;
}

ExecutionResult execute_statement(Statement* st, Table* t) {
    switch (st->type) {
        case (STATEMENT_INS):
            printf("\x1b[37;1mInsert statemen\x1b[0m\n");
            return EXE_SUCCESS;
        case (STATEMENT_SET):
            printf("\x1b[37;1mSet statemen\x1b[0m\n");
            return EXE_SUCCESS;
        case (STATEMENT_DEL):
            printf("\x1b[37;1mDelete statemen\x1b[0m\n");
            return EXE_SUCCESS;
        case (STATEMENT_SEL):
            printf("\x1b[37;1mSelect statemen\x1b[0m\n");
            return EXE_SUCCESS;
        case (STATEMENT_HELP):
            printf("\x1b[37;1mHelp statement\x1b[0m\n");
            return EXE_SUCCESS;
        case (STATEMENT_LIST):
            printf("\x1b[37;1mList statement\x1b[0m\n");
            return EXE_SUCCESS;
        case (STATEMENT_FIND):
            printf("\x1b[37;1mFind statement\x1b[0m\n");
            return EXE_SUCCESS;
        default: return EXE_TABLE_FULL;
    }
}

