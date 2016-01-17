#include "print.h"

#include <stdio.h>
#include <stdarg.h>

FILE* logfile = NULL;

void plog(const char* fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    vfprintf(logfile, fmt, argp);
    va_end(argp);
    va_start(argp, fmt);
    vfprintf(stdout, fmt, argp);
    va_end(argp);
    fflush(logfile);
    fflush(stdout);
}

char* piece_chars[3] = {"KQRBNP!", "kqrbnp!", "!!!!!!."};
void print_fen() {
    for (uint8_t rank = 7; rank != 0xff; rank--) {
        int blanks = 0;
        for (uint8_t file = 0; file < 8; file++) {
            square_t sq = SQUARE(rank, file);
            if (b.pieces[sq] == PIECE_EMPTY) {
                blanks++;
                continue;
            } else {
                if (blanks > 0) {
                    plog("%d", blanks);
                    blanks = 0;
                }
                plog("%c", piece_chars[b.colors[sq]][b.pieces[sq]]);
            }
        }
        if (blanks > 0) {
            plog("%d", blanks);
            blanks = 0;
        }
        if (rank > 0) plog("/");
    }
    plog(" %c ", (b.stm == WHITE) ? 'w' : 'b');
    if (!(b.flags & (CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ))) {
        plog("-");
    } else {
        if (b.flags & CASTLE_WK) plog("K");
        if (b.flags & CASTLE_WQ) plog("Q");
        if (b.flags & CASTLE_BK) plog("k");
        if (b.flags & CASTLE_BQ) plog("q");
    }
    plog(" ");
    if (IS_SQUARE(b.ep)) {
        print_square(b.ep);
    } else {
        plog("-");
    }
    plog(" 0 %d", (b.ply / 2) + 1);
}

void print_board() {
    int rank = 7;
    int file = 0;
    plog("  abcdefgh\n");
    for (int rank = 7; rank >= 0; rank--) {
        plog("%c ", '1' + rank);
        for (int file = 0; file < 8; file++) {
            square_t sq = SQUARE(rank, file);
            plog("%c", piece_chars[b.colors[sq]][b.pieces[sq]]);
        }
        plog(" %c\n", '1' + rank);
    }
    plog("  abcdefgh\n");
    plog("Castle: ");
    if (b.flags & CASTLE_WK) plog("K");
    if (b.flags & CASTLE_WQ) plog("Q");
    if (b.flags & CASTLE_BK) plog("k");
    if (b.flags & CASTLE_BQ) plog("q");
    plog("\nPawn counts:\n  ");
    for (int i = 0; i < 8; i++) {
        plog("%d", b.pawn_counts[WHITE][i]);
    }
    plog("\n  ");
    for (int i = 0; i < 8; i++) {
        plog("%d", b.pawn_counts[BLACK][i]);
    }
    plog("\n");
    print_fen();
    plog("\n");
}
       
void print_piece(piece_t p) {
    if ((p == PAWN) || (p == PIECE_EMPTY)) return;
    plog("%c", "KQRBN"[p]);
}

void print_square(square_t sq) {
    plog("%c%c", 'a' + SQFILE(sq), '1' + SQRANK(sq));
}

void print_move(move_t* m) {
    print_piece(b.pieces[m->from_square]);
    print_square(m->from_square);
    print_square(m->to_square);
    if (m->move_type & MOVE_PROMOTION) {
        piece_t promo = m->move_type & 0x7;
        print_piece(promo);
    }
}

