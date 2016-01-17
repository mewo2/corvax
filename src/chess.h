#include <sys/time.h>
#include <inttypes.h>

typedef struct {
    char name[256];
    void (*execute)(char*);
} verb_t;

typedef struct {
    char* args;
    void (*execute)(char*);
} cmd_t;

typedef struct cmd_queue_s {
    cmd_t* cmd;
    struct cmd_queue_s* next;
} cmd_queue_t;

struct timeval start_clock;
int time_remaining;

int64_t time_diff(struct timeval t1, struct timeval t2);
void block();
void unblock();
void queue_command(char* input);
