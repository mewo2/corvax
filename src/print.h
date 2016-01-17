#ifndef PRINT_H
#define PRINT_H 1

#include <stdio.h>
#include "board.h"
#include "move.h"

FILE* logfile;

void plog(const char* fmt, ...);

void print_fen();
void print_board();
void print_piece(piece_t p);
void print_square(square_t sq);
void print_move(move_t* m);

#endif
