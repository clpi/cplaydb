#pragma once

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "dbc.h"

#define COLUMN_MAX_NAME_LEN 255
#define TABLE_MAX_PAGES 100
#define TABLE_MAX_PAGES 100

typedef struct Row {
    uint32_t id;
    char username[32];
    char email[32];
} Row;

typedef struct Pager {
    int filedesc;
    uint32_t filelen;
    void* pages[TABLE_MAX_PAGES];
} Pager;

typedef struct Table {
    uint32_t rowlen;
    Pager* pager;
    void* pages[TABLE_MAX_PAGES];
} Table;


void printrow(Row* row) {
    printf("\x1b[37;1m[%d] \x1b[0m\x1b[32m%s  \x1b[34m%s\x1b[0m)\n", 
           row->id, row->username, row->email);
}
#define TABLE_MAX_PAGES 100
#define DBPAGE_SIZE 4096
#define sizeof_attribute(Struct, Attribute) sizeof(((Struct*)0) -> Attribute)

static const uint32_t ID_SIZE = sizeof_attribute(Row, id);
static const uint32_t ID_OFFSET = 0;
static const uint32_t USERNAME_SIZE = sizeof_attribute(Row, username);
static const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
static const uint32_t EMAIL_SIZE = sizeof_attribute(Row, email);
static const uint32_t EMAIL_OFFSET = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

static const uint32_t ROW_SIZE  =ID_SIZE + USERNAME_SIZE +EMAIL_SIZE;
static const uint32_t ROWS_PER_PAGE = DBPAGE_SIZE / ROW_SIZE;
static const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;


void serialize_row(Row* src, void* dest) {
    memcpy((int *)dest + ID_OFFSET, &(src->id), ID_SIZE);
    memcpy((int *)dest + USERNAME_OFFSET, &(src->id), USERNAME_SIZE);
    memcpy((int *)dest + EMAIL_OFFSET, &(src->id), EMAIL_SIZE);
}

void deserialize_row(void* src, Row* dest) {
    memcpy(&(dest->id), (int *)src + ID_OFFSET, ID_SIZE);
    memcpy(&(dest->username), (int *)src + ID_OFFSET, ID_SIZE);
    memcpy(&(dest->email), (int *)src + ID_OFFSET, ID_SIZE);
}

void* get_page(Pager* p, uint32_t pnum) {
    if (pnum > TABLE_MAX_PAGES) {
        emsg((char *)"Attempted non-indexable GET op", (char*)"");
        exit(EXIT_FAILURE);
    }
    if (p->pages[pnum] == NULL) {
        void* page = malloc(DBPAGE_SIZE);
        uint32_t numpages = p->filelen / DBPAGE_SIZE;
        if (p->filelen % DBPAGE_SIZE)
            numpages += 1;

        if (pnum <= numpages) {
            lseek(p->filedesc, pnum * DBPAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(p->filedesc, page, DBPAGE_SIZE);
            if (bytes_read == -1) {
                emsg((char *)"Cannot read file", (char *)"");
                exit(EXIT_FAILURE);
            }
        }
        p->pages[pnum] = page;
    }
    return p->pages[pnum];
}

void* rowslot(Table* table, uint32_t rownum) {
    uint32_t pageno = rownum / ROWS_PER_PAGE;
    void* page = get_page(table->pager, pageno);
    uint32_t row_offset = rownum % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page;
    // return (void *)page + (void *)byte_offset;
}

Table* new_table() {
    Table* table = (Table*)malloc(sizeof(Table));
    table->rowlen = 0;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) 
        table->pages[i] = NULL;
    return table;
}

Pager* pager_open(const char* filen) {
    int fd = open(filen, O_RDWR | O_CREAT | S_IWUSR | S_IRUSR );
    if (fd == -1) {
        emsg((char*)"Unable to open file", (char*)filen);
        exit(-1);
    }
    off_t filelen = lseek(fd, 0, SEEK_END);

    Pager* pgr = (Pager*)malloc(sizeof(Pager));
    pgr->filedesc = fd;
    pgr->filelen = filelen;

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pgr->pages[i] = NULL;
    }
    return pgr;
}

Table* db_open(const char* filename) {
    Pager* pager = pager_open(filename);
    uint32_t numrows = pager->filelen / ROW_SIZE;
    Table* t = (Table *)malloc(sizeof(Table));
    t->pager = pager;
    t->rowlen = numrows;
    return t;

}

void pager_flush(Pager* p, uint32_t pnum, uint32_t size) {
    if (p->pages[pnum] == NULL) {
        emsg((char*)"Tried to flush null page", (char*)"");
        exit(EXIT_FAILURE);
    }
    off_t offset = lseek(p->filedesc, pnum * DBPAGE_SIZE, SEEK_SET);
    if (offset == -1) {
        emsg((char*)"Error seeking ", (char*)"");
        exit(EXIT_FAILURE);
    }
    ssize_t bytesw = write(p->filedesc, p->pages[pnum], size);
    if (bytesw == -1) {
        emsg((char*)"Error writing to file", (char*)"");
        exit(EXIT_FAILURE);
    }
}

void dbclose(Table* t) {
    Pager* p = t->pager;
    uint32_t num_full_pages = t->rowlen / ROWS_PER_PAGE;
    for (uint32_t i = 0; i < num_full_pages; i++) {
        if (p->pages[i] == NULL) continue;
        pager_flush(p, i, DBPAGE_SIZE);
        free(p->pages[i]);
        p->pages[i] = NULL;
    }

    uint32_t num_additional_rows = t->rowlen % ROWS_PER_PAGE;
    if (num_additional_rows > 0) {
        uint32_t pageno = num_full_pages;
        if (p->pages[pageno] != NULL) {
            pager_flush(p, pageno, num_additional_rows * ROW_SIZE);
            free(p->pages[pageno]);
            p->pages[pageno] = NULL;
        }
    }
    int res = close(p->filedesc);
    if (res == -1) {
        emsg((char*)"Error closing DB file", (char*)"");
        exit(EXIT_FAILURE);
    }
    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        void* page = p->pages[i];
        if (page) {
            free(page);
            p->pages[i] = NULL;
        }
    }
    free(p);
    free(t);
}


void free_table(Table* table) {
    for (int i = 0; table->pages[i]; i++) 
        free(table->pages[i]);
    free(table);
}
