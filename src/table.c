#include "board.h"
#include "table.h"
#include "print.h"
#include "eval.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

uint64_t hash_mask;
hash_entry_t* table;
uint64_t pawnhash_mask;
hash_entry_t* pawn_table;
uint64_t tries;
uint64_t hits;

int get_pawn_table() {
    hash_entry_t* h = &pawn_table[b.pawnhash & pawnhash_mask];
    if (h->hash == b.pawnhash) {
        return h->score;
    }
    return HASH_UNKNOWN;
}

void put_pawn_table(int score) {
    assert(score <= CHECKMATE);
    assert(score >= -CHECKMATE);
    hash_entry_t* h = &pawn_table[b.pawnhash & pawnhash_mask];
    h->hash = b.pawnhash;
    h->score = score;
}

int get_table(int alpha, int beta, int depth, move_t* best_move) {
#if CORVAX_NOTABLE
    return HASH_UNKNOWN;
#endif
    tries++;
    hash_entry_t* h = &table[b.hash & hash_mask];
    int score = h->score;
    if (score > CHECKMATE - 500) {
        score -= b.ply;
    } else if (score < -CHECKMATE + 500) {
        score += b.ply;
    }
    if (best_move) *best_move = NO_MOVE;
    if ((h->hash == b.hash) && (h->depth >= depth)) {
        switch (h->status) {
            case HASH_EXACT:
                hits++;
                if (best_move) *best_move = h->best_move;
                return score;
            case HASH_ALPHA:
                if (best_move) *best_move = h->best_move;
                if (score <= alpha) {
                    hits++;
                    return alpha;
                } else {
                    return HASH_UNKNOWN;
                }
            case HASH_BETA:
                if (best_move) *best_move = h->best_move;
                if (score >= beta) {
                    hits++;
                    return beta;
                } else {
                    return HASH_UNKNOWN;
                }
        }
    }
    return HASH_UNKNOWN;
}

void put_table(int depth, int score, int status, move_t best_move) {
    hash_entry_t* h = &table[b.hash & hash_mask];
    if ((score == HASH_UNKNOWN) || (score == HASH_POISON)) {
        plog("Tried to store %d\n", score);
        print_board();
    }
    if (score > CHECKMATE - 500) {
        score += b.ply;
    } else if (score < -CHECKMATE + 500) {
        score -= b.ply;
    }
    h->hash = b.hash;
    h->depth = depth;
    h->score = score;
    h->status = status;
    h->best_move = best_move;
}

#define RANDOM_HASH ((((uint64_t) arc4random()) << 32) | ((uint64_t) arc4random()))
void create_table(int nbits) {
    hash_keys = calloc(2, sizeof(uint64_t**));
    for (int c = 0; c < 2; c++) {
        hash_keys[c] = calloc(6, sizeof(uint64_t*));
        for (int p = 0; p < 6; p++) {
            hash_keys[c][p] = calloc(128, sizeof(uint64_t));
            for (int sq = 0; sq < 128; sq++) {
                hash_keys[c][p][sq] = RANDOM_HASH;
            }
        }
    }
    hash_black = RANDOM_HASH;

    uint64_t entries = 1 << nbits;
    hash_mask = entries - 1;
    table = calloc(entries, sizeof(hash_entry_t));
    for (uint64_t i = 0; i < entries; i++) {
        table[i].score = HASH_POISON;
    }
    tries = 0;
    hits = 0;
}

void create_pawn_table(int nbits) {
    uint64_t entries = 1 << nbits;
    pawnhash_mask = entries - 1;
    pawn_table = calloc(entries, sizeof(hash_entry_t));
    for (uint64_t i = 0; i < entries; i++) {
        pawn_table[i].score = HASH_UNKNOWN;
    }
}


void clear_table() {
    for (uint64_t i = 0; i <= hash_mask; i++) {
        table[i].hash = 0;
    }
    plog("# Table cleared\n");
}
void report_table() {
    uint64_t full = 0;
    for (uint64_t i = 0; i <= hash_mask; i++) {
        if (table[i].status) full++;
    }
    plog("Hash table: %lluM/%lluM full\n", full >> 20, (hash_mask + 1) >> 20);
    plog("Hit rate %lluM/%lluM\n", hits >> 20, tries >> 20);
}
