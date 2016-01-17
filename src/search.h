#ifndef SEARCH_H
#define SEARCH_H 1

#include "move.h"

int max_depth;
int reps();
move_t* do_move(int depth);
int alpha_beta(int alpha, int beta, int depth, move_t* best_move);
move_t fallback_move();
#endif
