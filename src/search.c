#include "chess.h"
#include "board.h"
#include "move.h"
#include "print.h"
#include "eval.h"
#include "search.h"
#include "table.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAXPLY 1024
#define FULLMOVES 4
#define REDUCTION_LIMIT 3

uint64_t nodes_checked;
uint64_t nodes_side[2];

move_t killers[MAXPLY][2];

move_t* search_line[256];
int search_depth;

uint64_t hash_available;
uint64_t beta_tries[256];
uint64_t beta_counts[256];
uint64_t beta_first = 0;
uint64_t futile_tries, futile_count;
uint64_t iid_tries, iid_successes;

uint64_t total_nodes = 0;
uint64_t total_time = 0;

int history[2][7][128];
uint64_t history_reductions;

int history_order[2][7][128];
int bugout = 0;

void age_history() {
    for (int c = 0; c < 2; c++) {
        for (int p = 0; p < 7; p++) {
            for (int sq = 0; sq < 128; sq++) {
                history_order[c][p][sq] /= 10;
            }
        }
    }
}
void reset_history() {
    for (int c = 0; c < 2; c++) {
        for (int p = 0; p < 7; p++) {
            for (int sq = 0; sq < 128; sq++) {
                history[c][p][sq] = 0;
            }
        }
    }
    history_reductions = 0;
}

void push_search(move_t* m) {
    search_line[search_depth] = m;
    search_depth++;
}

void pop_search() {
    search_depth--;
}

void print_line() {
    for (int i = search_depth - 1; i >= 0; i--) {
        if (search_line[i] != NULL) {
            unmake_move(search_line[i]);
        } else {
            unmake_null_move();
        }
    }
    for (int i = 0; i < search_depth; i++) {
        if (search_line[i] != NULL) {
            print_move(search_line[i]);    
            plog(" ");
            make_move(search_line[i]);
        } else {
            make_null_move();
            plog("NULL ");
        }
    }
}
        
int timeout(float fraction) {
    struct timeval now;
    int move_time = fraction * (time_remaining / 30) - 1;
    gettimeofday(&now, NULL);
    int64_t microsecs = time_diff(now, start_clock);
    if (microsecs / 10000 > move_time) {
        plog("# TIMEOUT %lld cs > %d\n", microsecs/10000, move_time);
        return 1;
    }
    return 0;
}

void check_time() {
    nodes_checked++;
    nodes_side[b.stm]++;
    if (nodes_checked % 1000 == 0) {
        /*read_input();*/
        if (timeout(1.5)) {
            plog("# Bugging out\n");
            bugout = 1;
        }
        if (nodes_checked % 500000 == 0) {
            plog("# LINE ");
            print_line();
            plog("\n");
        }
    }
}

move_t* do_move(int depth) {
    move_t m;
    bugout = 0;
    plog("# fen ");
    print_fen();
    plog("\n");
    int score = get_table(-99999, 99999, 0, NULL);
    if (score == HASH_UNKNOWN) score = 0;
    nodes_checked = 1;
    for (int d = 1; d <= max_depth; d++) {
#if CORVAX_ASPIRATION
        int dalpha = 50;
        int dbeta = 50;
#else
        int dalpha = 10000;
        int dbeta = 10000;
#endif
        uint64_t last_nodes = nodes_checked;
        nodes_checked = 0;
        nodes_side[WHITE] = 0;
        nodes_side[BLACK] = 0;
        move_t best = NO_MOVE;
        struct timeval tick;
        gettimeofday(&tick, NULL);
        while (!bugout) {
            /*for (int i = 0; i < 256; i++) {*/
                /*beta_tries[i] = 0;*/
                /*beta_counts[i] = 0;*/
            /*}*/
            hash_available = 0;
            futile_count = 0;
            futile_tries = 0;
            iid_tries = 0;
            iid_successes = 0;
            search_depth = 0;
            reset_history();
            int alpha = score - dalpha;
            alpha = alpha < -CHECKMATE ? -CHECKMATE : alpha;
            int beta = score + dbeta;
            beta = beta > CHECKMATE ? CHECKMATE : beta;
            int newscore = alpha_beta(alpha, beta, d, &best);
            uint64_t beta_total = 0;
            for (int i = 0; i < 256; i++) {
                beta_total += beta_counts[i];
            }
            if (beta_total > 0) {
                plog("# betas:");
                for (int i = 0; i < 10; i++) {
                    plog(" %d/%llu", i, beta_tries[i] > 0 ? (beta_counts[i] * 100) / beta_tries[i] : 0);
                }
                plog(" [%llu]\n", (100 * beta_first)/beta_total);
            }
            if (futile_tries > 0) {
                plog("# futility: %llu/%llu (%.1f%%)\n", futile_count, futile_tries, (100.0 * futile_count) / futile_tries);
            }
            if (history_reductions > 0) {
                plog("# history: %llu\n", history_reductions);
            }
            if (iid_tries > 0) {
                plog("# iid: %llu/%llu (%.1f%%)\n", iid_successes, iid_tries, (100.0 * iid_successes) / iid_tries);
            }
            if (bugout) break;
            if (newscore == score - dalpha) {
                plog("# Failed low (%d, %d)\n", score - dalpha, score + dbeta);
                dalpha *= 2;
                continue;
            }
            if (newscore == score + dbeta) {
                plog("# Failed high (%d, %d)\n", score - dalpha, score + dbeta);
                dbeta *= 2;
                continue;
            }
            score = newscore;
            break;
        }
        if (bugout) break;
        if (best.from_square == NO_SQUARE) {
            plog("# Best is NO_MOVE! Falling back\n");
            best = fallback_move();
        }
        m = best;
        plog("# at depth %d: ", d);
        print_move(&m);
        struct timeval tock;
        gettimeofday(&tock, NULL);
        int64_t deltat = time_diff(tock, tick);
        total_time += deltat;
        total_nodes += nodes_checked;
        if (score > CHECKMATE - 1000) {
            plog(" [Mate in %d] ", (CHECKMATE - score - b.ply + 1) / 2);    
        } else if (score < -CHECKMATE + 1000) {
            plog(" [Mated in %d] ", (CHECKMATE + score - b.ply) / 2);
        } else {
            plog(" [%d] ", score);
        }
        plog("- %llu(%llu/%llu) knodes in %lld ms, BF:%.2f EBF:%.2f NPS:%.2fk\n",
                nodes_checked/1000, nodes_side[WHITE]/1000, nodes_side[BLACK]/1000, 
                deltat / 1000, (1.0 * nodes_checked) / last_nodes, pow(nodes_checked, 1.0/d),
                (1000.0 * total_nodes) / total_time);
        fflush(stdout);
        if (timeout(1.0)) break;
    }
    move_push(m.from_square, m.to_square, m.move_type);
    make_move(&m);
    plog("# Current position repeats %d times\n", reps());
    return &move_stack[move_idx - 1];
}

int move_order_cmp_max(const void* v1, const void* v2) {
    move_t* m1 = (move_t*) v1;
    move_t* m2 = (move_t*) v2;
    return (m2->estimated_score > m1->estimated_score) ? 1 : -1;
}

void sort_moves(int old_idx) {
#if CORVAX_MOVEORDER
#else
    return;
#endif
    move_t best = NO_MOVE;
    int score_now = get_table(-99999, 99999, 0, &best);
    int promo_scores[5] = {0, 8000, 2001, 2000, 2002};

    for (int idx = old_idx; idx < move_idx; idx++) {
        move_t* m = &move_stack[idx];
        piece_t p = b.pieces[m->from_square];
        if (SAME_MOVE(m, &best)) {
            hash_available++;
            m->estimated_score = 9999;
#if CORVAX_KILLERS
        } else if (SAME_MOVE(m, &killers[b.ply][0])) {
            m->estimated_score = 5003;
        } else if (SAME_MOVE(m, &killers[b.ply][1])) {
            m->estimated_score = 5002;
        } else if ((b.ply > 1) && SAME_MOVE(m, &killers[b.ply-2][0])) {
            m->estimated_score = 5001;
        } else if ((b.ply > 1) && SAME_MOVE(m, &killers[b.ply-2][0])) {
            m->estimated_score = 5000;
#endif
        } else if (m->move_type & MOVE_PROMOTION) {
            piece_t promo = m->move_type & 0x7;
            m->estimated_score = promo_scores[promo];
        } else if ((m->move_type & MOVE_CAPTURE)) {
            piece_t cap = m->cap_piece;
            int mvvlaa = piece_scores[cap]/5 - piece_scores[p]/50;
            m->estimated_score = (piece_scores[p] <= piece_scores[cap] ? 4000 : 3000) + mvvlaa;
            if ((b.ply > 0) && (m->to_square == made_moves[b.ply - 1]->to_square)) {
                m->estimated_score += 3000;
            }
        } else {
            m->estimated_score = history_order[b.stm][p][m->to_square] / 1000;
            if (m->estimated_score < 0) m->estimated_score = 0;
        }
    }
}

void grab_move(int cur_idx) {
    int best_idx = cur_idx;
    int best_score = move_stack[cur_idx].estimated_score;
    for (int idx = cur_idx; idx < move_idx; idx++) {
        if (move_stack[idx].estimated_score > best_score) {
            best_idx = idx;
            best_score = move_stack[idx].estimated_score;
        }
    }
    if (best_idx == cur_idx) return;
    move_t tmp = move_stack[cur_idx];
    move_stack[cur_idx] = move_stack[best_idx];
    move_stack[best_idx] = tmp;
}

int quiesce(int alpha, int beta) {
#if CORVAX_QUIESCENCE
    check_time();
    if (bugout) return 0;

    int score = get_table(alpha, beta, 0, NULL);
    if (score != HASH_UNKNOWN) {
        assert(score <= CHECKMATE);
        assert(score >= -CHECKMATE);
        return score;
    }

    int stand = (b.stm == WHITE) ? evaluate(0) : -evaluate(0);
    assert(stand <= CHECKMATE);
    assert(stand >= -CHECKMATE);
    if (stand >= beta) {
        /*put_table(0, beta, HASH_BETA, NO_MOVE);*/
        assert(beta <= CHECKMATE);
        assert(beta >= -CHECKMATE);
        return beta;
    }
    if (stand > alpha) {
        alpha = stand;
    }
    int status = HASH_ALPHA;
    int old_idx = move_idx;
    move_gen(1);
    sort_moves(old_idx);
    int movecount = 0;
    int move_idx_saved = move_idx;
    for (int idx = old_idx; idx < move_idx; idx++) {
        assert(move_idx_saved == move_idx);
        grab_move(idx);
        move_t* m = &move_stack[idx];
#if CORVAX_DELTA_PRUNE
        if (stand + piece_scores[m->cap_piece] + 200 < alpha) {
            continue;
        }
#endif
        make_move(m);
        if (in_check(!b.stm)) {
            unmake_move(m);
            continue;
        }
        push_search(m);            
        int score = -quiesce(-beta, -alpha);
        assert(score <= CHECKMATE);
        assert(score >= -CHECKMATE);
        unmake_move(m);
        pop_search();
        if (bugout) {
            move_idx = old_idx;
            return 0;
        }
        if (score >= beta) {
            /*put_table(0, beta, HASH_BETA, NO_MOVE);*/
            move_idx = old_idx;
            assert(beta <= CHECKMATE);
            assert(beta >= -CHECKMATE);
            return beta;
        }
        if (score > alpha) {
            status = HASH_EXACT;
            alpha = score;
        }
        movecount++;
    }
    /*put_table(0, alpha, status, NO_MOVE);*/
    move_idx = old_idx;
    if (movecount == 0) return stand;
    assert(alpha <= CHECKMATE);
    assert(alpha >= -CHECKMATE);
    return alpha;
#else
    return (b.stm == WHITE) ? evaluate(0) : evaluate(0);
#endif
}

int is_nullable() {
    if (b.flags & NULLED) return 0;
    if (b.piece_count[WHITE] == 0 || b.piece_count[BLACK] == 0) return 0;
    return 1;
}
int reps() {
    int repeats = 0;
    for (int i = 0; i < b.ply; i++) {
        if (positions[i] == b.hash) repeats++;
    }
    return repeats;
}

int is_valid_move(move_t* m) {
    int old_idx = move_idx;
    move_gen(0);
    for (int idx = old_idx; idx < move_idx; idx++) {
        if (SAME_MOVE(m, &move_stack[idx])) {
            move_idx = old_idx;
            return is_legal(m);
        }
    }
    move_idx = old_idx;
    return 0;
}
int alpha_beta(int alpha, int beta, int depth, move_t* best_move) {
    assert(alpha >= -CHECKMATE);
    assert(beta <= CHECKMATE);
    int at_root = (best_move != NULL);

    check_time();
    if (bugout) return 0;

    // Repetition detection
    if (!at_root) {
        if (reps() > 0) {
            if (0 < alpha) return alpha;
            if (0 > beta) return beta;
            return 0;
        }
    }

    // Check hash table
    int score = get_table(alpha, beta, depth, best_move);
    if ((score != HASH_UNKNOWN) && ((best_move == NULL) || (best_move->from_square != NO_SQUARE))) {
        if (best_move != NULL) {
            // We don't trust the hash move if it would lead to a rep
            if (is_valid_move(best_move)) {
                make_move(best_move);
                int r = reps();
                unmake_move(best_move);
                if (r < 2) return score;
            }
        } else {
            return score;
        }
    }
    
    // Quiescence search
    if (depth <= 0) {
        score = quiesce(alpha, beta);
        assert(score <= CHECKMATE);
        assert(score >= -CHECKMATE);
        if (bugout) return 0;
        return score;
    }

    int was_in_check = in_check(b.stm);

#if CORVAX_NULLMOVE
    if (!at_root && (depth > 2) && !was_in_check && is_nullable()) {
        push_search(NULL);
        make_null_move();
        score = -alpha_beta(-beta, -beta + 1, depth - 3, NULL);
        assert(score <= CHECKMATE);
        assert(score >= -CHECKMATE);
        unmake_null_move();
        pop_search();
        if (bugout) return 0;
        if (score >= beta) {
            put_table(depth, beta, HASH_BETA, NO_MOVE);
            return beta;
        }
    }
#endif
  
    move_t subst_best = NO_MOVE;
    if (best_move == NULL) best_move = &subst_best; 

    move_t hash_move = NO_MOVE;
    get_table(-99999, 99999, 0, &hash_move);
    if ((hash_move.to_square == NO_SQUARE) && (depth > 2)) {
        iid_tries++;
        alpha_beta(alpha, beta, depth - 2, NULL);
        if (bugout) return 0;
        get_table(-99999, 99999, 0, &hash_move);
        if (hash_move.to_square != NO_SQUARE) {
            iid_successes++;
        }
    }
    int status = HASH_ALPHA;
    int old_idx = move_idx;
    move_gen(0);
    sort_moves(old_idx);
    *best_move = NO_MOVE;
    int best_score = -99999;
    int legals = 0;
    score = HASH_UNKNOWN;
    int eval = (b.stm == WHITE) ? evaluate(0) : -evaluate(0);
    int move_idx_saved = move_idx;
    for (int idx = old_idx; idx < move_idx; idx++) {
        assert(move_idx_saved == move_idx);
        grab_move(idx);
        assert(move_idx_saved == move_idx);
        move_t* m = &move_stack[idx];
        piece_t p = b.pieces[m->from_square];
        int quiet_move = (m->move_type == MOVE_NORMAL) || (m->move_type == MOVE_PAWN_PUSH) || (m->move_type == MOVE_CASTLE);
        int stand = eval + ((b.stm == WHITE) ? m->delta_material : -m->delta_material);
        int would_check = would_cause_check(m);
        int prepromote = (p == PAWN) && (SQRANK(m->to_square) == ((b.stm == WHITE) ? 6 : 1));
        int reducible = quiet_move && !at_root && !was_in_check && !would_check && (legals > 0) && !prepromote;

        /*if (would_cause_check(m) != would_cause_check_slow(m)) {*/
            /*plog("Check detection failure\n");*/
            /*print_board();*/
            /*print_move(m);*/
            /*exit(1);*/
        /*}*/
        int new_depth = depth - 1;
 
#if CORVAX_LMR
        if ((legals >= FULLMOVES) && (depth > REDUCTION_LIMIT) && reducible) {
            new_depth--;
        }
#endif

        if (would_check) new_depth++;
#if CORVAX_FUTILITY
        if ((new_depth <= 2) && reducible) {
            futile_tries++;
            if (stand + 100 + 200 * new_depth <= alpha) {
                futile_count++;
                continue;
            }
        }
#endif

        make_move(m);
        if (in_check(!b.stm)) {
            unmake_move(m);
            continue;
        }
        // We have a legal move - now do extensions/reductions
        push_search(m);
        legals++;

        if (reducible &&
                stand < alpha &&
                history[!b.stm][p][m->to_square] <= -10) {
            history[!b.stm][p][m->to_square] = -10;
            new_depth--;
            history_reductions++;
        }

        if ((m->cap_piece < PAWN) && (b.piece_count[WHITE] == 0) && (b.piece_count[BLACK] == 0)) {
            new_depth += 3;
        }
        if (new_depth < 0) new_depth = 0;

        beta_tries[m->estimated_score / 1000]++;
        // Now do search
#if CORVAX_PVS
        if (legals == 1) {
            score = -alpha_beta(-beta, -alpha, new_depth, NULL);
        } else {
            score = -alpha_beta(-alpha - 1, -alpha, new_depth, NULL);
            if (score >= alpha + 1) {
                score = -alpha_beta(-beta, -alpha, new_depth, NULL);
            }
        }
#else
        score = -alpha_beta(-beta, -alpha, new_depth, NULL);
#endif
        assert(move_idx_saved == move_idx);
        assert(score <= CHECKMATE);
        assert(score >= -CHECKMATE);
        unmake_move(m);
        pop_search();

        if (bugout) {
            move_idx = old_idx;
            return 0;
        }

        history[b.stm][p][m->to_square] += 5;

        if (score > best_score) {
            best_score = score;
            *best_move = *m;
        }
        if (score >= beta) {
            put_table(depth, score, HASH_BETA, *m);
            beta_counts[m->estimated_score / 1000]++;
            if (legals == 1) beta_first++;
            move_idx = old_idx;
#if CORVAX_KILLERS
            if (!(m->move_type & MOVE_CAPTURE)) {
                if (SAME_MOVE(m, &killers[b.ply][1])) {
                    killers[b.ply][1] = killers[b.ply][0];
                    killers[b.ply][0] = *m;
                } else if (!SAME_MOVE(m, &killers[b.ply][0])) {
                    killers[b.ply][1] = killers[b.ply][0];
                    killers[b.ply][0] = *m;
                }
            }
#endif
            history_order[b.stm][p][m->to_square] += depth * depth;
            int n_moves = idx - old_idx;
            for (int midx = old_idx; midx < idx; midx++) {
                move_t* m2 = &move_stack[midx];
                history_order[b.stm][b.pieces[m2->from_square]][m2->to_square] -= (depth * depth) / n_moves;
            }
            if (history_order[b.stm][p][m->to_square] > 1000000) {
                age_history();
            }
            return beta;
        }
        if (score > alpha) {
            status = HASH_EXACT;
            alpha = score;
        } else {
            history[b.stm][p][m->to_square] -= 6;
        }
    }
    if (legals == 0) {
        if (was_in_check) {
            score = -CHECKMATE + b.ply;
        } else {
            score = 0;
        }
        put_table(999, score, HASH_EXACT, NO_MOVE);
        move_idx = old_idx;
        return score;
    }
    move_idx = old_idx;
    put_table(depth, alpha, status, *best_move);
    return alpha;
}

move_t fallback_move() {
    int old_idx = move_idx;
    move_t best = NO_MOVE;
    move_gen(0);
    sort_moves(old_idx);
    for (int idx = old_idx; idx < move_idx; idx++) {
        move_t* m = &move_stack[idx];
        if (!is_legal(m)) {
            continue;
        }
        move_idx = old_idx;
        return *m;
    }
    plog("# No legal moves!\n");
    move_idx = old_idx;
    return NO_MOVE;
}
