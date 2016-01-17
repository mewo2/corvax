#include "chess.h"
#include "board.h"
#include "move.h"
#include "print.h"
#include "search.h"
#include "table.h"
#include "eval.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>

cmd_queue_t* queue;
cmd_queue_t* tail;
FILE* infile;

int force;

int64_t time_diff(struct timeval t1, struct timeval t2) {
    int64_t diff = (t1.tv_sec - t2.tv_sec) * 1000000;
    diff += t1.tv_usec;
    diff -= t2.tv_usec;
    return diff;
}

void cmd_move(char* args) {
    parse_move(args);
    if (!force) {
        queue_command("go");
    }
}

void cmd_quit(char* args) {
    fclose(logfile);
    exit(0);
}

void cmd_board(char* args) {
    print_board();
}

void cmd_new(char* args) {
    load_fen(initial_fen);
}
void cmd_fen(char* args) {
    load_fen(args);
}
void cmd_force(char* args) {
    force = 1;
}

void cmd_perft(char* args) {
    for (int i = 1; i < 6; i++) {
        perft_t p = {0,0,0,0,0,0};
        perft(i, &p);
        plog("# perft(%d) = %llu %llu %llu %llu %llu %llu\n", i, 
                p.moves, p.captures, p.eps, p.castles, p.promotions, p.checks);
    }
}

void cmd_go(char* args) {
    force = 0;
    gettimeofday(&start_clock, 0);
    /*unblock();*/
    move_t* m = do_move(5);
    /*block();*/
    plog("move ");
    print_move(m);
    plog("\n");
    struct timeval now;
    gettimeofday(&now, NULL);
    time_remaining -= time_diff(now, start_clock) / 10000;
}

void cmd_protover(char* args) {
    plog("feature myname=\"Corvax\" setboard=1 done=1\n");
}

void cmd_time(char* args) {
    time_remaining = atoi(args);
    gettimeofday(&start_clock, 0);
    plog("# got time %s\n", args);
    plog("# interpreted as %d\n", time_remaining);
}

void cmd_eval(char* args) {
    int score = evaluate(0);
    plog("# eval = %d\n", score);
}

void cmd_list(char* args) {
    int old_idx = move_idx;
    move_gen(0);
    for (int idx = old_idx; idx < move_idx; idx++) {
        plog("# %d ", idx - old_idx + 1);
        print_move(&move_stack[idx]);
        plog("\n");
    }
    move_idx = old_idx;
}

void cmd_st(char* args) {
    max_depth = atoi(args);
}

verb_t verbs[] = {
    {"move", cmd_move},
    {"quit", cmd_quit},
    {"board", cmd_board},
    {"new", cmd_new},
    {"protover", cmd_protover},
    {"go", cmd_go},
    {"fen", cmd_fen},
    {"perft", cmd_perft},
    {"force", cmd_force},
    {"time", cmd_time},
    {"eval", cmd_eval},
    {"list", cmd_list},
    {"st", cmd_st},
    {"setboard", cmd_fen}
};
int n_verbs = 14;


void block() {
    int fd = fileno(infile);
    int flags = fcntl(fd, F_GETFL);
    assert(flags != -1);
    flags &= ~O_NONBLOCK;
    flags = fcntl(fd, F_SETFL, flags);
    assert(flags != -1);
}

void unblock() {
    int fd = fileno(infile);
    int flags = fcntl(fd, F_GETFL);
    assert(flags != -1);
    flags |= O_NONBLOCK;
    flags = fcntl(fd, F_SETFL, flags);
    assert(flags != -1);
}
void push_command(verb_t verb, char* args) {
    cmd_t* cmd = malloc(sizeof(cmd_t));
    cmd_queue_t* new_tail = malloc(sizeof(cmd_queue_t));
    new_tail->cmd = cmd;
    new_tail->next = NULL;
    cmd->execute = verb.execute;
    cmd->args = strdup(args);
    if (tail) {
        tail->next = new_tail;
    } else {
        queue = new_tail;
    }
    tail = new_tail;
}

void queue_command(char* input) {
    for (int i = 1; i < n_verbs; i++) {
        int length = strlen(verbs[i].name);
        if (strncmp(input, verbs[i].name, length) == 0) {
            push_command(verbs[i], input + length + 1);
            break;
        }
    }
    if ((input[1] >= '1') && (input[1] <= '8')) {
        push_command(verbs[0], input);
    }
}

void read_input() {
    char input[256];
    while (1) {
        if (fgets(input, 256, infile) == NULL) break;
        queue_command(input);
    }
}

void empty_queue() {
    while (queue) {
        cmd_t* cmd = queue->cmd;
        cmd_queue_t* next = queue->next;
        if (queue == tail) tail = NULL;
        free(queue);
        queue = next;
        cmd->execute(cmd->args);
        fflush(stdout);
        free(cmd->args);
        free(cmd);
    }
}

void initialize(int argc, char** argv) {
    char logname[1024];
    sprintf(logname, "%s.log", argv[0]);
    logfile = fopen(logname, "a");
    queue = NULL;
    tail = NULL;
    create_table(25);
    create_pawn_table(16);
    if (argc > 1) {
        plog("Setting input file to %s\n", argv[1]);
        infile = fopen(argv[1], "r");
    } else {
        infile = stdin;
    }
    max_depth = 64;
    plog("# Corvax v0.1\n");
#if CORVAX_ASPIRATION
    plog("# Aspiration search enabled\n");
#endif
#if CORVAX_KILLERS
    plog("# Killer moves enabled\n");
#endif
#if CORVAX_MOVEORDER
    plog("# Move ordering enabled\n");
#endif
#if CORVAX_LMR
    plog("# LMR enabled\n");
#endif
#if CORVAX_QUIESCENCE
    plog("# Quiescence search enabled\n");
#endif
#if CORVAX_NULLMOVE
    plog("# Null move enabled\n");
#endif
    fflush(stdout);
}

int main(int argc, char** argv) {
    initialize(argc, argv);
    while (1) {
        char input[256];
        if (fgets(input, 256, infile) == NULL) break;
        fprintf(logfile, "<<< %s", input);
        queue_command(input);
        empty_queue();
        fflush(stdout);
    }
}

