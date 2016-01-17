#ifndef EVAL_H
#define EVAL_H 1

#include "board.h"

#define CHECKMATE 30000

int evaluate_piece(color_t c, piece_t p, square_t sq);
int evaluate(int verbose);
int pawn_evaluate();
int piece_scores[6];
#endif
