// this file contains the functions that are called by the child process
#include "config.h"

// create pids and players arrays to store them after read from public fifo
pid_t players_pids2[2 * (NUM_PLAYERS + 1)];
struct Player players2[2 * (NUM_PLAYERS + 1)];

// initialize the player
void initialize_player(struct Player *player, int id)
{
    player->id = id;
    player->energy = generate_energy(player->id % 6 == 5 ? 1 : 0);
    player->has_ball = 0;
    player->team_name = (id < 6) ? 'A' : 'B';
}

void read_players_from_fifo()
{
    int public_fifo_fd = open(PUBLIC, O_RDONLY);
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        if (read(public_fifo_fd, &players2[i], sizeof(struct Player)) == -1)
        {
            perror("reading players from public fifo: ");
            exit(1);
        }
    }

    printf("Players read from public fifo\n");
    close(public_fifo_fd);
}

void read_pids_from_fifo()
{
    int public_fifo_fd = open(PUBLIC, O_RDONLY);
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        if (read(public_fifo_fd, &players_pids2[i], sizeof(pid_t)) == -1)
        {
            perror("reading pids from public fifo: ");
            exit(1);
        }
    }

    printf("Pids read from public fifo\n");
    close(public_fifo_fd);
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

void write_player_to_pipe(int write_fd, struct Player *player)
{
    if (write(write_fd, player, sizeof(struct Player)) == -1)
    {
        perror("writing player to pipe: ");
        exit(1);
    }
}

void read_player_from_pipe(int pipe_fd, struct Player *player)
{
    if (read(pipe_fd, player, sizeof(struct Player)) == -1)
    {
        perror("reading player from pipe: ");
        exit(1);
    }
}

// update players will be used by the parent process to read the status of the players, needed when round ends
void update_players(int read_fd, struct Player *players)
{
    struct Player player;
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        read_player_from_pipe(read_fd, &player);
        // players[i] = player;
    }
}
