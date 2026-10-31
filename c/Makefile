CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
BIN     := ledger_import

.PHONY: all clean check

all: $(BIN)

$(BIN): ledger_import.c
	$(CC) $(CFLAGS) -o $@ $<

check: $(BIN)
	./$(BIN) ../data/ledger.csv | grep -c '^INSERT'

clean:
	rm -f $(BIN) $(BIN).exe
