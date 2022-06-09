CC:=clang
OUT:=dist
OUTB:=$(OUT)/bin
OUTO:=$(OUT)/obj
SRCF:=src/dbc.c
BIN :=dbc
OUTF:=$(OUTB)/$(BIN)

CFLAGS=
LD_LIBRARY_PATH=

.PHONY=all

all: clean setup build run

clean:
	rm -rf $(OUT)/*

setup:
	mkdir -p $(OUTB)
	mkdir -p $(OUTO)

build:
	$(CC) $(CFLAGS) $(SRCF) -o $(OUTB)/$(BIN)
	
run:
	./$(OUTF) testdb
