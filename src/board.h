#ifndef BOARD_H
#define BOARD_H 1

#include <stdint.h>

typedef uint8_t piece_t;
#define KING 0
#define QUEEN 1
#define ROOK 2
#define BISHOP 3
#define KNIGHT 4
#define PAWN 5
#define PIECE_EMPTY 6

typedef uint8_t color_t;
#define WHITE 0
#define BLACK 1
#define COLOR_EMPTY 2

typedef uint8_t flags_t;
#define CASTLE_WK 1
#define CASTLE_WQ 2
#define CASTLE_BK 4
#define CASTLE_BQ 8
#define NULLED 16

typedef uint8_t square_t;
#define SQUARE(rank, file) ((file) + ((rank) << 4))
#define NO_SQUARE 0xff
#define IS_SQUARE(sq) (!(sq & 0x88))
#define SQRANK(sq) ((sq) >> 4)
#define SQFILE(sq) ((sq) & 0x7)

#define IS_EMPTY(sq) (b.pieces[sq] == PIECE_EMPTY)
#define IS_PIECE(sq, color, piece) ((b.pieces[sq] == (piece)) && (b.colors[sq] == (color)))

#define A1 0x00
#define B1 0x01
#define C1 0x02
#define D1 0x03
#define E1 0x04
#define F1 0x05
#define G1 0x06
#define H1 0x07
#define A2 0x10
#define B2 0x11
#define C2 0x12
#define D2 0x13
#define E2 0x14
#define F2 0x15
#define G2 0x16
#define H2 0x17
#define A3 0x20
#define B3 0x21
#define C3 0x22
#define D3 0x23
#define E3 0x24
#define F3 0x25
#define G3 0x26
#define H3 0x27
#define A4 0x30
#define B4 0x31
#define C4 0x32
#define D4 0x33
#define E4 0x34
#define F4 0x35
#define G4 0x36
#define H4 0x37
#define A5 0x40
#define B5 0x41
#define C5 0x42
#define D5 0x43
#define E5 0x44
#define F5 0x45
#define G5 0x46
#define H5 0x47
#define A6 0x50
#define B6 0x51
#define C6 0x52
#define D6 0x53
#define E6 0x54
#define F6 0x55
#define G6 0x56
#define H6 0x57
#define A7 0x60
#define B7 0x61
#define C7 0x62
#define D7 0x63
#define E7 0x64
#define F7 0x65
#define G7 0x66
#define H7 0x67
#define A8 0x70
#define B8 0x71
#define C8 0x72
#define D8 0x73
#define E8 0x74
#define F8 0x75
#define G8 0x76
#define H8 0x77

char* initial_fen;

typedef struct {
    uint64_t hash;
    uint64_t pawnhash;
    int material_score;
    piece_t pieces[128];
    color_t colors[128];
    uint8_t pawn_count[2];
    uint8_t piece_count[2];
    uint8_t pawn_counts[2][8];
    uint8_t pawn_ranks[2][8];
    square_t piece_sqs[2][16];
    square_t pawn_sqs[2][9];
    square_t king_sq[2];
    color_t stm;
    flags_t flags;
    square_t ep;
    uint8_t ply;
} board_t;

board_t b;


void clear_board();
void fill_square(color_t c, piece_t p, square_t sq);
void clear_square(square_t sq);
int count_piece(piece_t p);

void load_fen(char* fen);

#endif
