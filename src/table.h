#ifndef TABLE_H
#define TABLE_H 1

#include "move.h"

#define HASH_EXACT 1
#define HASH_ALPHA 2
#define HASH_BETA 3

#define HASH_UNKNOWN 32765
#define HASH_POISON 32764
typedef struct {
    uint64_t hash;
    move_t best_move;
    int16_t score;
    uint8_t depth;
    uint8_t status;
} hash_entry_t; // 22 bytes

uint64_t hash_black;
uint64_t*** hash_keys;

int get_table(int alpha, int beta, int depth, move_t* best_move);
void put_table(int depth, int score, int status, move_t best_move);
int get_pawn_table();
void put_pawn_table(int score);

void create_table(int nbits);
void create_pawn_table(int nbits);
void clear_table();
void report_table();

#endif
