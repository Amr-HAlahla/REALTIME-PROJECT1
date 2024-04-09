#ifndef CONFIG_H
#define CONFIG_H

#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <wait.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <asm-generic/signal-defs.h>
#include <bits/sigaction.h>

#define PUBLIC "/tmp/PUBLIC" // Public FIFO for communication between teams and parent
#define B_SIZ (PIPE_BUF / 2)

// Number of players in each team
#define NUM_PLAYERS 6

// Minimum and maximum initial energy levels for players
#define MIN_PLAYER_ENERGY 100
#define MAX_PLAYER_ENERGY 150

// Minimum and maximum initial energy levels for team leads
#define MIN_LEADER_ENERGY 150
#define MAX_LEADER_ENERGY 200

// Maximum number of rounds a team can lose before the game ends
#define MAX_LOST_ROUNDS 4

// Energy cost for throwing a ball
#define THROW_ENERGY_COST 12

// Energy cost for picking up a dropped ball
#define PICKUP_ENERGY_COST 5 // (1 / energy) ratio will be used as a probability of failuer to receive the ball.

// Constants for game settings
#define NUM_ROUNDS 3

#ifndef ROUND_TIME
#define ROUND_TIME 60
#endif

#ifndef GAME_TIME
#define GAME_TIME 480
#endif

struct Player
{
    int id;
    int energy;
    int has_ball;
    char team_name;
    pid_t next_player_pid;
    pid_t team_leader_pid;
};

struct Ball
{
    int ball_id;
    int last_player_id; // initial value is -1
    int next_player_id; // initial value is -1
    float x, y;
};

// struct GameState
// {

// };

#endif // CONFIG_H