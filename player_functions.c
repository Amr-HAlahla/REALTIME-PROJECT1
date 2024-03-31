// this file contains the functions that are called by the child process
#include "config.h"

// create pids and players arrays to store them after read from public fifo
pid_t players_pids[2 * (NUM_PLAYERS + 1)];
struct Player players[2 * (NUM_PLAYERS + 1)];

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
        if (read(public_fifo_fd, &players[i], sizeof(struct Player)) == -1)
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
        if (read(public_fifo_fd, &players_pids[i], sizeof(pid_t)) == -1)
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

void player_int_handler(int signum)
{
    // signal to notify the player that the round has ended
    // player should write its content to the public fifo
    int id;
    // extract the index of the current player!
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        if (players_pids[i] == getpid())
        {
            printf("Player %d turn has come\n", players[i].id);
            id = players[i].id;
            break;
        }
    }
    int public_fifo_fd = open(PUBLIC, O_WRONLY);
    if (public_fifo_fd == -1)
    {
        perror("open");
        exit(1);
    }
    if (write(public_fifo_fd, &players[id], sizeof(struct Player)) == -1)
    {
        perror("writing player to public fifo: ");
        exit(1);
    }

    printf("Player %d has written its content to the public fifo\n", id);
    close(public_fifo_fd);
}

void child_usr2_handler(int signum)
{
    /* Function when the leader receive the ball back from the last player in the team
    leader will pass the ball to the leader of the other team
    and then ask the parent to send a new ball to him (EACH LEADER WILL USE UNIQUE SIGNAL)
    */
    int id;
    // extract the index of the current player!
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        if (players_pids[i] == getpid())
        {
            printf("Leader %d turn has come\n", players[i].id);
            id = players[i].id;
            break;
        }
    }
    // implement receive the ball logic
    printf("Leader %d is receiving the ball\n", id);
    // update the player's status
    players[id].has_ball = 1;
    players[id].energy -= THROW_ENERGY_COST;

    sleep(2);
    // throw the ball to the leader of the other team
    int next_leader_id;
    if (id == 5)
    {
        next_leader_id = 11;
    }
    else if (id == 11)
    {
        next_leader_id = 5;
    }
    else
    {
        perror("Error: next leader id cannot be determined\n");
        exit(1);
    }

    if (next_leader_id < 0 || next_leader_id > 11)
    {
        perror("Error: next leader id is out of range\n");
        exit(1);
    }

    printf("Leader %d is throwing the ball to the leader of team %c\n", id, players[next_leader_id].team_name);
    // send USR1 signal to the leader of the other team
    kill(players_pids[next_leader_id], SIGUSR1);

    // // ask the parent to send a new ball to the leader
    // kill(getppid(), SIGUSR2);
}

void child_handler_usr1(int signum)
{
    printf("Print array of pids\n");
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        printf("Player %d has pid %d\n", players[i].id, players_pids[i]);
    }
    printf("====================================\n");
    sleep(2);
    // print pid of the player
    printf("Player %d has received the signal\n", getpid());
    // this function will be called when the player receives the signal when its turn comes
    int id;
    // extract the index of the current player!
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        if (players_pids[i] == getpid())
        {
            printf("Player %d turn has come\n", players[i].id);
            id = players[i].id;
            break;
        }
    }
    // implement receive the ball logic
    printf("Player %d is receiving the ball\n", id);
    // update the player's status
    players[id].has_ball = 1;
    players[id].energy -= THROW_ENERGY_COST;

    sleep(2);
    // throw the ball to the next player
    int next_player_id;
    if (id == 5)
    {
        next_player_id = 0;
    }
    else if (id == 11)
    {
        next_player_id = 6;
    }
    else
    {
        next_player_id = id + 1;
    }

    if (next_player_id < 0 || next_player_id > 11)
    {
        perror("Error: next player id is out of range\n");
        exit(1);
    }

    if (next_player_id == 5 || next_player_id == 11) // if the next player is the leader of the team
    {
        printf("Player %d is throwing the ball to the leader of team %c\n", id, players[next_player_id].team_name);
        // send USR2 signal to the leader
        kill(players_pids[next_player_id], SIGUSR2);
    }
    else
    {
        if (players[id].energy < THROW_ENERGY_COST)
        {
            printf("Player %d does not have enough energy to throw the ball\n", id);
            // send signal to the next player
            kill(players_pids[next_player_id], SIGUSR1);
        }
        else
        {

            printf("Player %d is throwing the ball to player %d\n", id, next_player_id);
            // send signal to the next player
            kill(players_pids[next_player_id], SIGUSR1);
        }
    }
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

    if (sigset(SIGUSR2, child_usr2_handler) == -1)
    {
        perror("sigset USR2 for player: ");
        exit(1);
    }

    if (sigset(SIGINT, player_int_handler) == -1)
    {
        perror("sigset INT for player: ");
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

void print_arrays()
{
    // print the players and their pids
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        print_player(players[i]);
        printf("Player %d has pid %d\n", players[i].id, players_pids[i]);
    }
}

// void throw_ball(int player_id)
// {
//     // throw the ball to the next player
//     int next_player_id = (player_id == 5) ? 0 : (player_id == 11) ? 6
//                                                                   : -1;
//     if (next_player_id == -1)
//     {
//         perror("Error: next player id is -1\n");
//         exit(1);
//     }
//     printf("Player %d is throwing the ball to player %d\n", player_id, next_player_id);
//     // send signal to the next player
//     kill(players_pids2[next_player_id], SIGUSR1);
// }

// void receive_ball(int player_id)
// {
//     // receive the ball from the previous player
//     printf("Player %d is receiving the ball\n", player_id);
//     // update the player's status
//     players2[player_id].has_ball = 1;
//     players2[player_id].energy -= THROW_ENERGY_COST;

//     sleep(2);
// }
