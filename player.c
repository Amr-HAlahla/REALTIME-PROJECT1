#include "config.h"

int main(int argc, char *argv[])
{
    sleep(1); // to make sure that the players are created in order
    // check if the number of arguments is correct
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <read_fd> <write_fd>\n", argv[0]);
        exit(1);
    }

    // get the read and write file descriptors
    int read_fd = atoi(argv[1]);
    int write_fd = atoi(argv[2]);

    struct Player player;
    // read the player from the pipe
    read_player_from_pipe(read_fd, &player);

    set_player_signals();

    while (1)
    {
        printf("Player %d is waiting for a signal\n", player.id);
        pause();
        // update energy of the player by (energy - THROW_ENERGY_COST)
        // printf("Update energy of player %d\n", player.id);
        player.energy -= THROW_ENERGY_COST;
        player.has_ball = 1;
        write_player_to_pipe(write_fd, &player);
    }

    // print_player(player);

    return 0;
}