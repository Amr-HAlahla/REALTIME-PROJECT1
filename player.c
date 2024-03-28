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

    close(write_fd); // close the write end of the pipe

    struct Player player;
    // read the player from the pipe
    if (read(read_fd, &player, sizeof(player)) == -1)
    {
        perror("read player from pipe: ");
        exit(1);
    }

    set_player_signals();

    while (1)
    {
        printf("Player %d is waiting for the signal to start\n", player.id);
        pause();
    }

    return 0;
}