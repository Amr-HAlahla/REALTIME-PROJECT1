#include "config.h"

typedef void (*sighandler_t)(int);

sighandler_t sigset(int sig, sighandler_t disp);

int num_of_balls = 0;
int current_round = 0;
int num_of_lost_rounds_teamA = 0;
int num_of_lost_rounds_teamB = 0;

void create_public_fifo();
void initialize_player(int id, pid_t next_player_pid, pid_t team_leader_pid);
void parent_signals_handler(int signum);
void print_player(struct Player player);
int generate_energy(int type);
void parent_set_signals();

pid_t players_pids[2 * NUM_PLAYERS];
struct Player players[2 * NUM_PLAYERS]; // 6 players each team. 12 players in total

// int pipe_fd[2 * NUM_PLAYERS][2];

// int public_fifo_fd;
pid_t pid;
pid_t next_player_pid;
pid_t team_leader_pid;

int running = 1;

int main(int argc, char *argv[])
{
    create_public_fifo();

    int i = 2 * NUM_PLAYERS - 1; // 11
    for (; i > -1; i--)
    {
        sleep(1); // to make sure that the players are created in order
        pid = fork();
        if (pid == -1)
        {
            perror("fork");
            exit(1);
        }

        if (pid == 0)
        {
            execl("./player", "player", NULL);
            perror("execl");
            exit(1);
        }
        else
        {
            // sleep(2);
            players_pids[i] = pid;

            if (i == 11)
            {
                next_player_pid = -1; // supposed to be the pid of player with index 6 (team B first player)
                team_leader_pid = -1; // supposed to be the pid of player with index 5 (team A leader)
            }
            else if (i == 5)
            {
                next_player_pid = -1;               // should be player with id 0 (team A first player)
                team_leader_pid = players_pids[11]; // point to the leader of other team (team B leader)
            }
            else
            {
                next_player_pid = players_pids[i + 1]; // previous created player
                team_leader_pid = -1;
            }
            initialize_player(i, next_player_pid, team_leader_pid);
        }
    }

    // printf("Print players before update their info!\n");
    // for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    // {
    //     print_player(players[i]);
    // }
    // print players pids after creating them
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        printf("Player %d pid: %d\n", i, players_pids[i]);
    }
    // set next player and team leader for each player
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        if (i == 11) // team B leader
        {
            players[i].next_player_pid = players_pids[6]; // first player in team B
            players[i].team_leader_pid = players_pids[5]; // team A leader
        }
        else if (i == 5) // team A leader
        {
            players[i].next_player_pid = players_pids[0];  // first player in team A
            players[i].team_leader_pid = players_pids[11]; // team B leader
        }
        else
        {
            players[i].next_player_pid = players_pids[i + 1]; // next player
            players[i].team_leader_pid = -1;                  // no team leader
        }
    }

    parent_set_signals();

    printf("Print players after update their info!\n");
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        print_player(players[i]);
    }
    printf("====================================\n");
    printf("Sending signals to the players to receive their info from the public fifo\n");
    // fflush(stdout);
    // sleep(1);
    // send signals to the players to receive their info from the public fifo
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        sleep(1); // to avoid race condition between players on reading from the public fifo
        kill(players_pids[i], SIGUSR1);
    }

    int public_fifo_fd = open(PUBLIC, O_WRONLY);
    if (public_fifo_fd == -1)
    {
        perror("open");
        exit(1);
    }
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        if (write(public_fifo_fd, &players[i], sizeof(struct Player)) == -1)
        {
            perror("writing to public fifo");
            exit(1);
        }
        sleep(1);
        printf("Writing player %d to public fifo\n", i);
    }
    close(public_fifo_fd);
    printf("Players written to public fifo\n");
    // wait for the players to finish reading from the public fifo
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        wait(NULL);
    }
    sleep(1);
    int index = 0;
    while (running)
    {
        if (!index)
        {
            printf("Parent process is waiting for a signal\n");
        }
        index++;
    }
    printf("Parent process is done\n");

    return 0;
}

// handler for the parent process signals
void parent_signals_handler(int signum)
{
    if (signum == SIGUSR1)
    {
        printf("Parent received the signal to start the round\n");
        printf("=============================\n");
        // send usr2 signal to all players to start the round
        for (int i = 0; i < 2 * NUM_PLAYERS; i++)
        {
            printf("Waking up player %d with pid %d\n", i, players_pids[i]);
            kill(players_pids[i], SIGUSR2);
        }
        // start_round();
    }
    else if (signum == SIGQUIT)
    {
        printf("Parent received the signal to end the game\n");
        // kill all childs
        for (int i = 0; i < 2 * NUM_PLAYERS; i++)
        {
            printf("Killing player %d with pid %d\n", i, players_pids[i]);
            kill(players_pids[i], SIGTERM);
        }
        exit(0);
    }
}
/*

*/

// create a public fifo
void create_public_fifo()
{
    remove(PUBLIC);

    if (mkfifo(PUBLIC, 0666) == -1)
    {
        perror("Error");
        exit(-1);
    }
}

int generate_energy(int id)
{
    srand(time(NULL));
    int range;
    // it its id is 5 or 11 then it is a team leader, else its normal player
    if (id == 5 || id == 11)
    {
        range = MAX_LEADER_ENERGY - MIN_LEADER_ENERGY + 1;
        return rand() % range;
    }
    else
    {
        range = MAX_PLAYER_ENERGY - MIN_PLAYER_ENERGY + 1;
        return rand() % range;
    }
}
// initialize the player
void initialize_player(int id, pid_t next_player_pid, pid_t team_leader_pid)
{
    struct Player player;
    player.id = id;
    player.energy = generate_energy(id);
    player.has_ball = 0;
    player.team_name = id < 6 ? 'A' : 'B';
    player.next_player_pid = next_player_pid;
    player.team_leader_pid = team_leader_pid;
    players[id] = player;
}

void parent_set_signals()
{
    if (sigset(SIGUSR1, parent_signals_handler) == -1)
    {
        perror("sigset");
        exit(1);
    }

    if (sigset(SIGQUIT, parent_signals_handler) == -1)
    {
        perror("sigset");
        exit(1);
    }
}

void print_player(struct Player player)
{
    printf("Player %d from team %c has %d energy and %s the ball and next player is %d and team leader is %d\n",
           player.id, player.team_name, player.energy, player.has_ball ? "has" : "does not have", player.next_player_pid, player.team_leader_pid);
}
