#include "config.h"

int main(int argc, char *argv[])
{
    sleep(1); // to make sure that the players are created in order
    // check if the number of arguments is correct
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <id> <energy> <has_ball> <team>\n", argv[0]);
        exit(1);
    }

    struct Player player;

    // prepare the arguments for the player process
    int id = atoi(argv[1]);
    int energy = atoi(argv[2]);
    int has_ball = atoi(argv[3]);
    char team = argv[4][0];

    // initialize the player
    initialize_player(&player, id, energy, has_ball, team);

    // print the player's information
    print_player(player);

    // make the player able to handle the kill signal when received from the parent
    struct sigaction act;
    act.sa_handler = player_quit_handler;
    act.sa_flags = SA_RESTART;
    sigemptyset(&act.sa_mask);

    if (sigaction(SIGQUIT, &act, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    while (1)
    {
        pause();
    }

    return 0;
}
