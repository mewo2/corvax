CFILES = src/chess.c \
		 src/board.c \
		 src/move.c \
		 src/print.c \
		 src/search.c \
		 src/eval.c \
		 src/table.c

OPTIONS = -DCORVAX_KILLERS=1 \
	  -DCORVAX_MOVEORDER=1 \
	  -DCORVAX_QUIESCENCE=1 \
	  -DCORVAX_LMR=1 \
	  -DCORVAX_ASPIRATION=1 \
	  -DCORVAX_MOBILITY=0 \
	  -DCORVAX_NOTABLE=0 \
	  -DCORVAX_NULLMOVE=1 \
	  -DCORVAX_PVS=1 \
	  -DCORVAX_DOUBLEDPAWN=1 \
	  -DCORVAX_FUTILITY=1

CFLAGS = -O3

all: bin/corvax

bin/corvax: $(CFILES)
	mkdir -p bin
	clang $(CFLAGS) $(CFILES) $(OPTIONS) -o $@

clean:
	rm -rf bin/*

test: bin/corvax
	python tests/test.py
