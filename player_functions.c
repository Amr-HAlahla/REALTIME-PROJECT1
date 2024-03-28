// this file contains the functions that are called by the child process
#include "config.h"

// initialize the player
void initialize_player(struct Player *player, int id)
{
    player->id = id;
    player->energy = generate_energy(player->id % 6 == 5 ? 1 : 0);
    player->has_ball = 0;
    player->team_name = (id < 6) ? 'A' : 'B';
}

void print_player(struct Player player)
{
    printf("Player %d from team %c has %d energy and %s the ball\n", player.id, player.team_name, player.energy, player.has_ball ? "has" : "does not have");
}

void player_quit_handler(int signum)
{
    printf("Player %d has been killed\n", getpid());
    exit(0);
}

void child_handler_usr1(int signum)
{
    printf("Player %d has received the signal to start\n", getpid());
    sleep(1);
    printf("Player %d is starting a fake simulation\n", getpid());
}

int generate_energy(int type)
{
    int energy;
    // type 0 => normal player, type 1 => leader (diff in defined energy ranges)
    if (type == 0)
    {
        energy = (rand() % (MAX_PLAYER_ENERGY - MIN_PLAYER_ENERGY + 1)) + MIN_PLAYER_ENERGY;
    }
    else
    {
        energy = (rand() % (MAX_LEADER_ENERGY - MIN_LEADER_ENERGY + 1)) + MIN_LEADER_ENERGY;
    }
    return energy;
}

void reset_players_status(struct Player *player)
{
    // reset the status of the players
    player->has_ball = 0;
    player->energy = generate_energy(player->id % 6 == 5 ? 1 : 0);
}

void set_player_signals()
{
    if (sigset(SIGUSR1, child_handler_usr1) == -1)
    {
        perror("sigset USR1 for player: ");
        exit(1);
    }

    if (sigset(SIGQUIT, player_quit_handler) == -1)
    {
        perror("sigset QUIT for player: ");
        exit(1);
    }
}
