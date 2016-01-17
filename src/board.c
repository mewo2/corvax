#include "board.h"
#include "eval.h"
#include "table.h"

#include <stdio.h>
#include <assert.h>

char* initial_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void clear_board() {
    for (int i = 0; i < 128; i++) {
        b.pieces[i] = PIECE_EMPTY;
        b.colors[i] = COLOR_EMPTY;
    }
    for (int i = 0; i < 8; i++) {
        b.pawn_counts[WHITE][i] = 0;
        b.pawn_counts[BLACK][i] = 0;
        b.pawn_ranks[WHITE][i] = 0;
        b.pawn_ranks[BLACK][i] = 7;
    }
    b.king_sq[WHITE] = NO_SQUARE;
    b.king_sq[BLACK] = NO_SQUARE;
    b.pawn_count[WHITE] = 0;
    b.piece_count[WHITE] = 0;
    b.pawn_count[BLACK] = 0;
    b.piece_count[BLACK] = 0;
    b.stm = WHITE;
    b.flags = CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ;
    b.ep = NO_SQUARE;
    b.ply = 0;
    b.material_score = 0;
    b.hash = 0;
    b.pawnhash = 0;
    positions[0] = 0;
}

void fill_square(color_t c, piece_t p, square_t sq) {
    assert(p < 6);
    assert(c < 2);
    b.pieces[sq] = p;
    b.colors[sq] = c;
    if (p == KING) {
        b.king_sq[c] = sq;
    } else if (p == PAWN) {
        b.pawn_sqs[c][b.pawn_count[c]] = sq;
        b.pawn_count[c]++;
        b.pawn_counts[c][SQFILE(sq)]++;
        b.pawnhash ^= hash_keys[c][PAWN][sq];
    } else {
        b.piece_sqs[c][b.piece_count[c]] = sq;
        b.piece_count[c]++;
    }
    b.hash ^= hash_keys[c][p][sq];
    /*b.material_score += evaluate_piece(c, p, sq);*/
}

void clear_square(square_t sq) {
    /*if (!IS_EMPTY(sq)) {*/
        /*b.material_score -= evaluate_piece(b.colors[sq], b.pieces[sq], sq);*/
    /*}*/
    color_t c = b.colors[sq];
    piece_t p = b.pieces[sq];
    if (IS_EMPTY(sq)) {
        return;
    }
    b.hash ^= hash_keys[c][p][sq];
    if (p == PAWN) {
        for (int i = 0; i < b.pawn_count[c]; i++) {
            if (b.pawn_sqs[c][i] == sq) {
                b.pawn_sqs[c][i] = b.pawn_sqs[c][b.pawn_count[c] - 1];
                break;
            }
        }
        b.pawn_counts[c][SQFILE(sq)]--;
        b.pawnhash ^= hash_keys[c][PAWN][sq];
        b.pawn_count[c]--;
    } else if (p != KING) {
        for (int i = 0; i < b.piece_count[c]; i++) {
            if (b.piece_sqs[c][i] == sq) {
                b.piece_sqs[c][i] = b.piece_sqs[c][b.piece_count[c] - 1];
                break;
            }
        }
        b.piece_count[c]--;
    }
    b.pieces[sq] = PIECE_EMPTY;
    b.colors[sq] = COLOR_EMPTY;
}

int count_piece(piece_t p) {
    int count = 0;
    for (int i = 0; i < 128; i++) {
        if (b.pieces[i] == p) {
            count++;
        }
    }
    return count;
}

void load_fen(char* fen) {
    clear_board();
    char* ptr = fen;
    uint8_t rank = 7;
    uint8_t file = 0;
    do {
        switch (*ptr) {
            case 'K':
                fill_square(WHITE, KING, SQUARE(rank, file));
                file++;
                break;
            case 'Q':
                fill_square(WHITE, QUEEN, SQUARE(rank, file));
                file++;
                break;
            case 'R':
                fill_square(WHITE, ROOK, SQUARE(rank, file));
                file++;
                break;
            case 'B':
                fill_square(WHITE, BISHOP, SQUARE(rank, file));
                file++;
                break;
            case 'N':
                fill_square(WHITE, KNIGHT, SQUARE(rank, file));
                file++;
                break;
            case 'P':
                fill_square(WHITE, PAWN, SQUARE(rank, file));
                file++;
                break;
            case 'k':
                fill_square(BLACK, KING, SQUARE(rank, file));
                file++;
                break;
            case 'q':
                fill_square(BLACK, QUEEN, SQUARE(rank, file));
                file++;
                break;
            case 'r':
                fill_square(BLACK, ROOK, SQUARE(rank, file));
                file++;
                break;
            case 'b':
                fill_square(BLACK, BISHOP, SQUARE(rank, file));
                file++;
                break;
            case 'n':
                fill_square(BLACK, KNIGHT, SQUARE(rank, file));
                file++;
                break;
            case 'p':
                fill_square(BLACK, PAWN, SQUARE(rank, file));
                file++;
                break;
            case '/':
                rank--;
                file = 0;
                break;
            case '1':
                file += 1;
                break;
            case '2':
                file += 2;
                break;
            case '3':
                file += 3;
                break;
            case '4':
                file += 4;
                break;
            case '5':
                file += 5;
                break;
            case '6':
                file += 6;
                break;
            case '7':
                file += 7;
                break;
            case '8':
                file += 8;
                break;
        };
        ptr++;
    } while (*ptr != ' ');
    
    ptr++;

    if (*ptr == 'w') {
        b.stm = WHITE;
    } else {
        b.stm = BLACK;
    }

    ptr += 2;
    
    b.flags = 0;
    do {
        switch (*ptr) {
            case 'K':
                b.flags |= CASTLE_WK;
                break;
            case 'Q':
                b.flags |= CASTLE_WQ;
                break;
            case 'k':
                b.flags |= CASTLE_BK;
                break;
            case 'q':
                b.flags |= CASTLE_BQ;
                break;
        }
        ptr++;
    } while (*ptr != ' ');
    
    ptr++;

    if (*ptr != '-') {
        uint8_t file = ptr[0] - 'a';
        uint8_t rank = ptr[1] - '1';
        b.ep = SQUARE(rank, file);
    }

    do {
        ptr++;
    } while (*ptr != ' ');

    ptr++;
    int ply = 0;
    sscanf(ptr, "%d", &ply);
    b.ply = ply;
    positions[b.ply] = b.hash;
    for (square_t sq = 0; sq < 128; sq++) {
        if (!IS_SQUARE(sq)) continue;
        if (b.pieces[sq] != PIECE_EMPTY) {
            b.material_score += evaluate_piece(b.colors[sq], b.pieces[sq], sq);
        }
    }
}

