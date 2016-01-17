#ifndef MOVE_H
#define MOVE_H 1

#include "board.h"

#define MOVE_NORMAL 0
#define MOVE_CAPTURE 16
#define MOVE_PROMOTION 8
#define MOVE_CASTLE 32
#define MOVE_EP 17
#define MOVE_PAWN_PUSH 64

typedef uint8_t move_type_t;

typedef struct {
    square_t to_square;
    square_t from_square;
    piece_t cap_piece;
    flags_t oldflags;
    square_t oldep;
    move_type_t move_type;
    int16_t delta_material;
    int16_t estimated_score;
} move_t; // 10 bytes
#define NO_MOVE ((move_t) {NO_SQUARE, NO_SQUARE, 0, 0, 0, 0, 0, 0})

#define SAME_MOVE(_mp1, _mp2) (((_mp1)->to_square == (_mp2)->to_square) && ((_mp1)->from_square == (_mp2)->from_square) && ((_mp1)->move_type == (_mp2)->move_type))

move_t* made_moves[1024];
move_t move_stack[8192];
int move_idx;
uint64_t positions[1024];

#define NORTH 16
#define NORTH2 32
#define SOUTH -16
#define SOUTH2 -32
#define EAST 1
#define WEST -1
#define NE 17
#define NW 15
#define SE -15
#define SW -17

typedef struct {
    uint64_t moves;
    uint64_t captures;
    uint64_t eps;
    uint64_t castles;
    uint64_t promotions;
    uint64_t checks;
} perft_t;

void make_null_move();
void unmake_null_move();

void make_move(move_t* m);
void unmake_move(move_t* m);
void move_push(square_t from, square_t to, move_type_t move_type);

void move_gen(int captures);
void move_gen_pawn(square_t sq);
void move_gen_pawn_cap(square_t sq);

int would_cause_check(move_t* m);
int in_check(color_t side);
int is_attacked(color_t side, square_t sq);
int is_legal(move_t* m);
void filter_legal_moves(int base_idx);

void parse_move(char* mov);
void perft(int depth, perft_t* p);
#endif
