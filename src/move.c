#include "move.h"
#include "board.h"
#include "print.h"
#include "eval.h"
#include "table.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int move_idx = 0;
int knight_dirs[8] = {-33, -31, -18, -14, 14, 18, 31, 33};
int vectors[5] = { 8, 8, 4, 4, 8 };
int vector[5][8] = {
        { SW, SOUTH, SE, WEST, EAST, NW, NORTH, NE },
        { SW, SOUTH, SE, WEST, EAST, NW, NORTH, NE },
        { SOUTH, WEST, EAST, NORTH                 },
        { SW, SE, NW, NE                           },
        { -33, -31, -18, -14, 14, 18, 31, 33       }
};
char slide[5] = {0, 1, 1, 1, 0};
void make_null_move() {
    b.stm = !b.stm;
    b.hash ^= hash_black;
    b.ply++;
    b.flags |= NULLED;
    positions[b.ply] = b.hash;
}

void unmake_null_move() {
    b.stm = !b.stm;
    b.hash ^= hash_black;
    b.flags &= ~NULLED;
    b.ply--;
}

void make_move(move_t* m) {
    made_moves[b.ply] = m;
    b.stm = !b.stm;
    b.hash ^= hash_black;
    b.hash ^= b.flags & 0xf;
    square_t to = m->to_square;
    square_t from = m->from_square;
    piece_t piece = b.pieces[from];
    color_t color = b.colors[from];
    assert(piece != PIECE_EMPTY);
    assert(color != COLOR_EMPTY);
    assert(color == !b.stm);
    clear_square(to);
    fill_square(color, piece, to);
    clear_square(from);
    // Handle ep, castling, promotion
    if (piece == KING) {
        if (color == WHITE) {
            b.flags &= ~(CASTLE_WK | CASTLE_WQ);
        } else {
            b.flags &= ~(CASTLE_BK | CASTLE_BQ);
        }
        if (m->move_type == MOVE_CASTLE) {
            switch (to) {
                case C1:
                    clear_square(A1);
                    fill_square(WHITE, ROOK, D1);
                    break;
                case G1:
                    clear_square(H1);
                    fill_square(WHITE, ROOK, F1);
                    break;
                case C8:
                    clear_square(A8);
                    fill_square(BLACK, ROOK, D8);
                    break;
                case G8:
                    clear_square(H8);
                    fill_square(BLACK, ROOK, F8);
                    break;
            }
        }
    } else if (piece == ROOK) {
        if (color == WHITE) {
            if (from == A1) {
                b.flags &= ~CASTLE_WQ;
            } else if (from == H1) {
                b.flags &= ~CASTLE_WK;
            }
        } else {
            if (from == A8) {
                b.flags &= ~CASTLE_BQ;
            } else if (from == H8) {
                b.flags &= ~CASTLE_BK;
            }
        }
    }
    if (m->cap_piece == ROOK) {
        if (to == A1) {
            b.flags &= ~CASTLE_WQ;
        } else if (to == H1) {
            b.flags &= ~CASTLE_WK;
        } else if (to == A8) {
            b.flags &= ~CASTLE_BQ;
        } else if (to == H8) {
            b.flags &= ~CASTLE_BK;
        }
    }
    if (m->move_type & MOVE_PROMOTION) {
        piece_t promo = m->move_type & 0x7;
        clear_square(to);
        fill_square(color, promo, to);
    }
    if (m->move_type == MOVE_PAWN_PUSH) {
        b.ep = (from + to) >> 1;
    } else {
        b.ep = NO_SQUARE;
    }
    if (m->move_type == MOVE_EP) {
        square_t cap = (from & 0x70) | (to & 0x07);
        clear_square(cap);
    }
    b.material_score += m->delta_material;
    b.hash ^= b.flags & 0xf;
    b.ply++;
    b.flags &= ~NULLED;
    positions[b.ply] = b.hash;
}

void unmake_move(move_t* m) {
    b.stm = !b.stm;
    b.hash ^= hash_black;
    b.hash ^= b.flags & 0xf;

    square_t to = m->to_square;
    square_t from = m->from_square;
    piece_t piece = b.pieces[to];
    color_t color = b.colors[to];
    assert(color == b.stm);
    if (m->move_type & MOVE_PROMOTION) {
        piece = PAWN;
    }
    fill_square(color, piece, from);
    clear_square(to);
    if (m->move_type == MOVE_EP) {
        square_t cap = (from & 0x70) | (to & 0x7);
        fill_square(!b.stm, PAWN, cap);
    } else if (m->cap_piece != PIECE_EMPTY) {
        fill_square(!b.stm, m->cap_piece, to);
    }
    b.flags = m->oldflags;
    b.ep = m->oldep;
    if (m->move_type == MOVE_CASTLE) {
        switch (to) {
            case C1:
                clear_square(D1);
                fill_square(WHITE, ROOK, A1);
                break;
            case G1:
                clear_square(F1);
                fill_square(WHITE, ROOK, H1);
                break;
            case C8:
                clear_square(D8);
                fill_square(BLACK, ROOK, A8);
                break;
            case G8:
                clear_square(F8);
                fill_square(BLACK, ROOK, H8);
                break;
        }
    }
    b.material_score -= m->delta_material;
    b.hash ^= b.flags & 0xf;
    b.ply--;
}

void move_push(square_t from, square_t to, move_type_t move_type) {
    assert(IS_SQUARE(from));
    assert(IS_SQUARE(to));
    assert(to != from);
    assert(b.pieces[from] != PIECE_EMPTY);
    assert(b.colors[from] == b.stm);
    move_t* m = &(move_stack[move_idx]);
    if (move_idx > 4096) {
        plog("# move_idx = %d! %hhx->%hhx\n", move_idx, from, to);
    }
    m->to_square = to;
    m->from_square = from;
    m->cap_piece = b.pieces[to];
    m->oldflags = b.flags;
    m->oldep = b.ep;
    m->move_type = move_type;
    piece_t p = b.pieces[from];
    color_t c = b.colors[from];
    assert(p < 6);
    assert(c < 2);
    m->delta_material = evaluate_piece(c, p, to) - evaluate_piece(c, p, from);
    if (m->move_type == MOVE_EP) {
        m->cap_piece = PAWN;
        square_t cap = (from & 0x70) | (to & 0x7);
        m->delta_material -= evaluate_piece(!c, PAWN, cap);
    } else if (m->cap_piece != PIECE_EMPTY) {
        m->delta_material -= evaluate_piece(!c, m->cap_piece, to);
    }
    if (m->move_type == MOVE_CASTLE) {
        square_t rook_from;
        square_t rook_to;
        switch (to) {
            case C1:
                rook_from = A1; rook_to = D1;
                break;
            case G1:
                rook_from = H1; rook_to = F1;
                break;
            case C8:
                rook_from = A8; rook_to = D8;
                break;
            case G8:
                rook_from = H8; rook_to = F8;
                break;
        }
        m->delta_material += evaluate_piece(c, ROOK, rook_to) - evaluate_piece(c, ROOK, rook_from);
    }
    if (m->move_type & MOVE_PROMOTION) {
        piece_t promo = m->move_type & 0x7;
        m->delta_material += evaluate_piece(c, promo, to) - evaluate_piece(c, PAWN, to);
    }
    move_idx++;
}

void push_promotions(square_t from, square_t to, move_type_t move_type) {
    move_push(from, to, move_type | QUEEN);
    move_push(from, to, move_type | KNIGHT);
    move_push(from, to, move_type | ROOK);
    move_push(from, to, move_type | BISHOP);
}

void move_gen_pawn(square_t sq) {
    assert(b.pieces[sq] == PAWN);
    assert(b.colors[sq] == b.stm);
    if (b.stm == WHITE) {
        if (b.pieces[sq + NORTH] == PIECE_EMPTY) {
            if (SQRANK(sq) == 6) {
                push_promotions(sq, sq + NORTH, MOVE_PROMOTION);
            } else {
                move_push(sq, sq + NORTH, MOVE_NORMAL);
            }
            if ((SQRANK(sq) == 1) && (b.pieces[sq + NORTH2] == PIECE_EMPTY)) {
                move_push(sq, sq + NORTH2, MOVE_PAWN_PUSH);
            }
        }
    } else {
        if (b.pieces[sq + SOUTH] == PIECE_EMPTY) {
            if (SQRANK(sq) == 1) {
                push_promotions(sq, sq + SOUTH, MOVE_PROMOTION);
            } else {
                move_push(sq, sq + SOUTH, MOVE_NORMAL);
            }
            if ((SQRANK(sq) == 6) && (b.pieces[sq + SOUTH2] == PIECE_EMPTY)) {
                move_push(sq, sq + SOUTH2, MOVE_PAWN_PUSH);
            }
        }
    }
}

void move_gen_pawn_cap(square_t sq) {
    assert(b.pieces[sq] == PAWN);
    assert(b.colors[sq] == b.stm);
    if (b.stm == WHITE) {
        if ((b.colors[sq + NE] == BLACK) || 
                (!(b.flags & NULLED) && (b.ep == sq + NE))) {
            if (SQRANK(sq) == 6) {
                push_promotions(sq, sq + NE, MOVE_PROMOTION | MOVE_CAPTURE);
            } else {
                move_push(sq, sq + NE, (b.ep == sq + NE) ? MOVE_EP : MOVE_CAPTURE);
            }
        }
        if ((b.colors[sq + NW] == BLACK) || 
                (!(b.flags & NULLED) && (b.ep == sq + NW))) {
            if (SQRANK(sq) == 6) {
                push_promotions(sq, sq + NW, MOVE_PROMOTION | MOVE_CAPTURE);
            } else {
                move_push(sq, sq + NW, (b.ep == sq + NW) ? MOVE_EP : MOVE_CAPTURE);
            }
        }
    } else {
        if ((b.colors[sq + SE] == WHITE) || 
                (!(b.flags & NULLED) && (b.ep == sq + SE))) {
            if (SQRANK(sq) == 1) {
                push_promotions(sq, sq + SE, MOVE_PROMOTION | MOVE_CAPTURE);
            } else {
                move_push(sq, sq + SE, (b.ep == sq + SE) ? MOVE_EP : MOVE_CAPTURE);
            }
        }
        if ((b.colors[sq + SW] == WHITE) ||
                (!(b.flags & NULLED) && (b.ep == sq + SW))) {
            if (SQRANK(sq) == 1) {
                push_promotions(sq, sq + SW, MOVE_PROMOTION | MOVE_CAPTURE);
            } else {
                move_push(sq, sq + SW, (b.ep == sq + SW) ? MOVE_EP : MOVE_CAPTURE);
            }
        }
    }
}

void move_gen(int captures) {
    if (!captures) {
        if (b.stm == WHITE) {
            if ((b.flags & CASTLE_WQ) && 
                    IS_EMPTY(B1) && IS_EMPTY(C1) && IS_EMPTY(D1) &&
                    !is_attacked(BLACK, D1) && !is_attacked(BLACK, E1)) {
                move_push(E1, C1, MOVE_CASTLE);
            }
            if ((b.flags & CASTLE_WK) && 
                    IS_EMPTY(F1) && IS_EMPTY(G1) &&
                    !is_attacked(BLACK, F1) && !is_attacked(BLACK, E1)) {
                move_push(E1, G1, MOVE_CASTLE);
            }
        } else {
            if ((b.flags & CASTLE_BQ) && 
                    IS_EMPTY(B8) && IS_EMPTY(C8) && IS_EMPTY(D8) &&
                    !is_attacked(WHITE, D8) && !is_attacked(WHITE, E8)) {
                move_push(E8, C8, MOVE_CASTLE);
            }
            if ((b.flags & CASTLE_BK) &&
                    IS_EMPTY(F8) && IS_EMPTY(G8) &&
                    !is_attacked(WHITE, F8) && !is_attacked(WHITE, E8)) {
                move_push(E8, G8, MOVE_CASTLE);
            }
        }
    }
    square_t sq = b.king_sq[b.stm];
    for (int i = -1; i < b.piece_count[b.stm]; i++) {
        if (i >= 0) {
            sq = b.piece_sqs[b.stm][i];
        }
        assert(b.colors[sq] == b.stm);
        assert(b.pieces[sq] != PIECE_EMPTY);
        piece_t p = b.pieces[sq];
        assert(p < 6);
        for (int dir = 0; dir < vectors[p]; dir++) {
            int8_t vec = vector[p][dir];
            for (square_t pos = sq + vec; IS_SQUARE(pos); pos += vec) {
                if (b.colors[pos] == COLOR_EMPTY) {
                    if (!captures) move_push(sq, pos, MOVE_NORMAL);
                } else {
                    if (b.colors[pos] != b.stm) {
                        move_push(sq, pos, MOVE_CAPTURE);
                    }
                    break;
                }
                if (!slide[p]) break;
            }
        }
    } 
    for (int i = 0; i < b.pawn_count[b.stm]; i++) {
        sq = b.pawn_sqs[b.stm][i];
        assert(b.pieces[sq] == PAWN);
        assert(b.colors[sq] == b.stm);
        if (!captures) move_gen_pawn(sq);
        move_gen_pawn_cap(sq);
    }

}

int would_cause_check_slow(move_t* m) {
    make_move(m);
    int check = in_check(b.stm);
    unmake_move(m);
    return check;
}

int would_cause_check(move_t* m) {
    // These are complicated but rare, so fall back to the slow method
    if (m->move_type == MOVE_CASTLE ||
        m->move_type & MOVE_PROMOTION ||
        m->move_type == MOVE_EP) {
        return would_cause_check_slow(m);
    }
    square_t king_sq = b.king_sq[!b.stm];
    square_t from = m->from_square;
    square_t to = m->to_square;
    piece_t p = b.pieces[from];
    // direct check
    int delta;
    switch (p) {
        case PAWN:
            if (b.stm == WHITE) {
                if ((king_sq == to + NW) ||
                    (king_sq == to + NE)) {
                    return 1;
                }
            } else {
                if ((king_sq == to + SW) ||
                    (king_sq == to + SE)) {
                    return 1;
                }
            }
            break;
        case KNIGHT:
            for (int i = 0; i < 8; i++) {
                if (to + knight_dirs[i] == king_sq) {
                    return 1;
                }
            }
            break;
        case BISHOP:
        case ROOK:
        case QUEEN:
            delta = ((int) king_sq) - to;
            for (int i = 0; i < vectors[p]; i++) {
                int dir = vector[p][i];
                int steps = delta/dir;
                if ((steps > 0) && (steps < 9) && (delta == dir * steps)) {
                    int clear = 1;
                    for (int j = 1; j < steps; j++) {
                        square_t s = to + j * dir;
                        if ((b.pieces[s] != PIECE_EMPTY) && (s != from)) {
                            clear = 0;
                            break;
                        }
                    }
                    if (clear) return 1;
                    break;
                }
            }
            break;
    }
    // discovered check
    delta = ((int) king_sq) - from;
    for (int i = 0; i < 8; i++) {
        int dir = vector[0][i];
        int steps = delta/dir;
        if ((steps > 0) && (steps < 9) && (delta == dir * steps)) {
            for (square_t s = king_sq - dir; IS_SQUARE(s); s -= dir) {
                if (s == to) {
                    return 0;
                }
                if ((b.pieces[s] != PIECE_EMPTY) && (s != from)) {
                    if (b.colors[s] == !b.stm) {
                        return 0;
                    }
                    piece_t att = b.pieces[s];
                    if ((att < PAWN) && slide[att]) {
                        for (int j = 0; j < vectors[att]; j++) {
                            if (vector[att][j] == dir) return 1;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
    return 0;
}
int in_check(color_t side) {
    return is_attacked(!side, b.king_sq[side]);
}

int is_attacked(color_t side, square_t sq) {
    int pawn_dirs[2][2] = {{SW, SE}, {NW, NE}};
    for (int i = 0; i < 2; i++) {
        int dir = pawn_dirs[side][i];
        if (IS_SQUARE(sq + dir) && 
                (b.pieces[sq + dir] == PAWN) && 
                (b.colors[sq + dir] == side)) {
            return 1;
        }
    }

    for (int i = 0; i < 8; i++) {
        int dir = knight_dirs[i];
        if (IS_SQUARE(sq + dir) &&
                (b.pieces[sq + dir] == KNIGHT) &&
                (b.colors[sq + dir] == side)) {
            return 1;
        }
    }

    int dirs[8] = {NORTH, SOUTH, EAST, WEST, NE, SE, NW, SW};
    int has_dir[2][6] = {{1, 1, 1, 0, 0, 0}, {1, 1, 0, 1, 0, 0}};
    for (int i = 0; i < 8; i++) {
        int dir = dirs[i];
        int one_step = 1;
        for (square_t pos = sq + dir; IS_SQUARE(pos); pos += dir) {
            if (IS_EMPTY(pos)) {
                one_step = 0;
                continue;
            }
            if ((b.colors[pos] == side) && (b.pieces[pos] == KING)) {
                if (one_step) {
                    return 1;
                }
            } else if ((b.colors[pos] == side) && (has_dir[i >> 2][b.pieces[pos]])) {
                return 1;
            }
            break;
        }
    }

    return 0;
}

int is_attacked_slow(color_t side, square_t sq) {
    color_t oldstm = b.stm;
    int old_move_idx = move_idx;
    b.stm = side;
    move_gen(1);
    int attacked = 0;
    for (int i = old_move_idx; i < move_idx; i++) {
        if (move_stack[i].cap_piece == KING) {
            attacked = 1;
            break;
        }
    }
    b.stm = oldstm;
    move_idx = old_move_idx;
    
    if (is_attacked(side, sq) != attacked) {
        plog("Mismatch! %c\n", "WB"[side]);
        print_square(sq);   
        print_board();
        exit(1);
    }
    return attacked;
}

int is_legal(move_t* m) {
    make_move(m);
    int legal = in_check(!b.stm);
    unmake_move(m);
    return legal;
}

void filter_legal_moves(int base_idx) {
    while (base_idx < move_idx) {
        if (is_legal(&move_stack[base_idx])) {
            base_idx++;
        } else {
            move_stack[base_idx] = move_stack[move_idx - 1];
            move_idx--;
        }
    }
}
 
void parse_move(char* mov) {
    move_idx = 0;
    square_t from = SQUARE(mov[1] - '1', mov[0] - 'a');
    square_t to = SQUARE(mov[3] - '1', mov[2] - 'a');
    move_type_t move_type = MOVE_NORMAL;
    if (b.colors[to] != COLOR_EMPTY) {
        move_type = MOVE_CAPTURE;
    } else if ((b.pieces[from] == PAWN) && (to == b.ep)) {
        move_type = MOVE_EP;
    }
    if (b.pieces[from] == PAWN &&
            (((b.stm == WHITE) && (SQRANK(to) == 7)) ||
             ((b.stm == BLACK) && (SQRANK(to) == 0)))) {
        plog("Parsing promotion: %s", mov);
        move_type |= MOVE_PROMOTION;
        piece_t promo;
        switch (mov[4]) {
            case 'q':
            case 'Q':
                promo = QUEEN;
                break;
            case 'r':
            case 'R':
                promo = ROOK;
                break;
            case 'b':
            case 'B':
                promo = BISHOP;
                break;
            case 'n':
            case 'N':
                promo = KNIGHT;
                break;
            default:
                promo = QUEEN;
                plog("Promotion parsing failure");
                plog("# Failed to parse promotion (%s) - assumed queen\n", mov);
                break;
        }
        move_type |= promo;
    } else if (b.pieces[from] == KING && ((to == from + 2) || (from == to + 2))) {
        move_type = MOVE_CASTLE;
    }
    if (b.colors[from] != b.stm) {
        plog("# Warning: moving wrong colour (%s)\n", mov);
    }
    move_push(from, to, move_type);
    make_move(move_stack);
    move_idx = 0;
}

void perft(int depth, perft_t* p) {
    int old_idx = move_idx;
    move_gen(0);
    for (int idx = old_idx; idx < move_idx; idx++) {
        move_t* m = &move_stack[idx];
        make_move(m);
        if (!in_check(!b.stm)) {
            if (depth == 1) {
                p->moves++;
                if (m->move_type & MOVE_CAPTURE) p->captures++;
                if (m->move_type == MOVE_EP) p->eps++;
                if (m->move_type == MOVE_CASTLE) p->castles++;
                if (m->move_type & MOVE_PROMOTION) p->promotions++;
                if (in_check(b.stm)) p->checks++;
            } else {
                perft(depth - 1, p);
            }
        }
        unmake_move(m);
    }
    move_idx = old_idx;
}
