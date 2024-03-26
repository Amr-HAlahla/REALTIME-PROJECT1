// this file contains the functions that are called by the child process
#include "config.h"

// initialize the player
// initialize the player
void initialize_player(struct Player *player, int id, int energy, int has_ball, char team)
{
    player->id = id;
    player->energy = energy;
    player->has_ball = has_ball;
    player->team_name = team;
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

void reset_players_status(struct Player *player)
{
    // reset the status of the players
    player->has_ball = 0;
    player->energy = generate_energy(player->id % 6 == 5 ? 1 : 0);
}