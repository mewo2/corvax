#include "eval.h"
#include "print.h"
#include "board.h"
#include "table.h"
#include <assert.h>
#include <stdio.h>
int king_endgame_scores[64] = {
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 10, 10, 10, 10, 10, 10, 0,
     0, 10, 20, 20, 20, 20, 10, 0,
     0, 10, 20, 30, 30, 20, 10, 0,
     0, 10, 20, 30, 30, 20, 10, 0,
     0, 10, 20, 20, 20, 20, 10, 0,
     0, 10, 10, 10, 10, 10, 10, 0,
     0, 0, 0, 0, 0, 0, 0, 0};

int piece_square_scores[6][64] = {
    {-30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -10,-20,-20,-20,-20,-20,-20,-10,
         20, 20,  0,  0,  0,  0, 20, 20,
          20, 30, 10,  0,  0, 10, 30, 20},
    {-20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5,  5,  5,  5,  0,-10,
         -5,  0,  5,  5,  5,  5,  0, -5,
           0,  0,  5,  5,  5,  5,  0, -5,
           -10,  5,  5,  5,  5,  5,  0,-10,
           -10,  0,  5,  0,  0,  0,  0,-10,
           -20,-10,-10, -5, -5,-10,-10,-20},
    {0,  0,  0,  0,  0,  0,  0,  0,
          5, 10, 10, 10, 10, 10, 10,  5,
           -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
             -5,  0,  0,  0,  0,  0,  0, -5,
              -5,  0,  0,  0,  0,  0,  0, -5,
               -5,  0,  0,  0,  0,  0,  0, -5,
                 0,  0,  0,  5,  5,  0,  0,  0},
    {-20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-10,-10,-10,-10,-10,-20},
    {-50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 10, 15, 15, 12,  5,-30,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50},
    { 0,  0,  0,  0,  0,  0,  0,  0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
         5,  5, 10, 25, 25, 10,  5,  5,
          0,  0,  0, 20, 21,  0,  0,  0,
           5, -5,-10,  0,  0,-10, -5,  5,
            5, 10, 10,-20,-20, 10, 10,  5,
             0,  0,  0,  0,  0,  0,  0,  0}};

int piece_scores[6] = {20000, 975, 500, 325, 325, 100};

int mobility_scores[6] = {1, 1, 2, 2, 4, 4};
int mobility(color_t side) {
    color_t stm = b.stm;
    b.stm = side;
    int old_idx = move_idx;
    move_gen(0);
    int moves = 0;
    for (int idx = old_idx; idx < move_idx; idx++) {
        moves += mobility_scores[b.pieces[move_stack[idx].from_square]];
    }
    move_idx = old_idx;
    b.stm = stm;
    return moves;
}

int evaluate_piece(color_t c, piece_t p, square_t sq) {
    int idx = SQFILE(sq) + 8 * ((c == WHITE) ? (7 - SQRANK(sq)) : SQRANK(sq));
    if (c == WHITE) {
        return piece_scores[p] + piece_square_scores[p][idx];
    } else {
        return -piece_scores[p] - piece_square_scores[p][idx];
    }
}

int count_pieces(color_t c, piece_t p) {
    int count = 0;
    for (int i = 0; i < b.piece_count[c]; i++) {
        if (b.pieces[b.piece_sqs[c][i]] == p) count++;
    }
    return count;
}

int evaluate_position(color_t c) {
    int score = 0;
    int bishops = 0;
    if (b.piece_count[WHITE] + b.piece_count[BLACK] < 5) {
        square_t sq = b.king_sq[c];
        int king_idx = SQFILE(sq) + 8 * ((c == WHITE) ? (7 - SQRANK(sq)) : SQRANK(sq));
        score += king_endgame_scores[king_idx] - piece_square_scores[KING][king_idx];
    }
    for (int i = 0; i < b.piece_count[c]; i++) {
        square_t sq = b.piece_sqs[c][i];
        int file = SQFILE(sq);
        piece_t p = b.pieces[sq];
        switch (p) {
            case BISHOP:
                bishops++;
                break;
            case ROOK:
                if (b.pawn_counts[c][file] == 0) {
                    if (b.pawn_counts[!c][file] == 0) {
                        score += 20;
                    } else {
                        score += 10;
                    }
                }
                break;
        }
    }
    if (bishops > 1) score += 50;
    return score;
}

int evaluate(int verbose) {
    int score = 0;
    if (b.stm == WHITE) {
        score += 30;
    } else {
        score -= 30;
    }
    score += b.material_score;
#if CORVAX_MOBILITY
    score += mobility(WHITE) - mobility(BLACK);
#endif
    score += pawn_evaluate();
    if ((b.piece_count[WHITE] == 0) && (b.piece_count[BLACK] == 0)) {
        score += 100 * (b.pawn_count[WHITE] - b.pawn_count[BLACK]);
    }
    score += evaluate_position(WHITE) - evaluate_position(BLACK);
    return score;
}
int pawn_evaluate() {
#if CORVAX_DOUBLEDPAWN
    int score = get_pawn_table();
    if (score != HASH_UNKNOWN) {
        assert(score <= CHECKMATE);
        assert(score >= -CHECKMATE);
        return score;
    }
    score = 0;
    int pawn_front[2][8] = {{0, 0, 0, 0, 0, 0, 0, 0}, {7, 7, 7, 7, 7, 7, 7, 7}};
    int pawn_back[2][8] = {{7, 7, 7, 7, 7, 7, 7, 7}, {0, 0, 0, 0, 0, 0, 0, 0}};
    for (int i = 0; i < 8; i++) {
        int wc = b.pawn_counts[WHITE][i];
        int bc = b.pawn_counts[BLACK][i];
        int wcp = (i == 7) ? 0 : b.pawn_counts[WHITE][i+1];
        int bcp = (i == 7) ? 0 : b.pawn_counts[BLACK][i+1];
        int wcm = (i == 0) ? 0 : b.pawn_counts[WHITE][i-1];
        int bcm = (i == 0) ? 0 : b.pawn_counts[BLACK][i-1];

        if (wc > 1) score -= 20;
        if (bc > 1) score += 20;
        if ((wc > 0) && (wcp == 0) && (wcm == 0)) {
            score -= 20;
        }
        if ((bc > 0) && (bcp == 0) && (bcm == 0)) {
            score += 20;
        }
        if (wc) {
            int pf, pb;
            for (pf = 6; (pf > 0) && !IS_PIECE(SQUARE(pf, i), WHITE, PAWN); pf--);
            for (pb = 1; (pb < 7) && !IS_PIECE(SQUARE(pb, i), WHITE, PAWN); pb++);
            pawn_front[WHITE][i] = pf;
            pawn_back[WHITE][i] = pb;
        }
        if (bc) {
            int pf, pb;
            for (pb = 6; (pb > 0) && !IS_PIECE(SQUARE(pb, i), BLACK, PAWN); pb--);
            for (pf = 1; (pf < 7) && !IS_PIECE(SQUARE(pf, i), BLACK, PAWN); pf++);
            pawn_front[BLACK][i] = pf;
            pawn_back[BLACK][i] = pb;
        }
    }
    for (int i = 0; i < 8; i++) {
        int pfw = pawn_front[WHITE][i];
        if ((pfw >= pawn_back[BLACK][i]) &&
                ((i == 0) || (pfw >= pawn_back[BLACK][i-1])) &&
                ((i == 7) || (pfw >= pawn_back[BLACK][i+1]))) {
            score += 20 * pfw;
        }
        int pfb = pawn_front[BLACK][i];
        if ((pfb <= pawn_back[WHITE][i]) &&
                ((i == 0) || (pfb <= pawn_back[WHITE][i-1])) &&
                ((i == 7) || (pfb <= pawn_back[WHITE][i+1]))) {
            score -= 20 * (7 - pfb);
        }
    }
    put_pawn_table(score);
    return score;
#else
    return 0;
#endif
}


