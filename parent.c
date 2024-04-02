#include "config.h"

typedef void (*sighandler_t)(int);

sighandler_t sigset(int sig, sighandler_t disp);

int num_of_balls = 0;
int current_round = 0;
int num_of_lost_rounds_teamA = 0;
int num_of_lost_rounds_teamB = 0;
int num_of_balls_teamA = 0;
int num_of_balls_teamB = 0;

void create_public_fifo();
void initialize_player(int id, pid_t next_player_pid, pid_t team_leader_pid);
void parent_signals_handler(int signum);
void print_player(struct Player player);
int generate_energy(int type);
void parent_set_signals();

pid_t players_pids[2 * NUM_PLAYERS];
struct Player players[2 * NUM_PLAYERS]; // 6 players each team. 12 players in total

// int public_fifo_fd;
pid_t pid;
pid_t next_player_pid;
pid_t team_leader_pid;

int running = 1;

int main(int argc, char *argv[])
{

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

    create_public_fifo();
    parent_set_signals();
    sleep(1);
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
    printf("====================================\n");
    printf("Sending signals to the players to receive their info from the public fifo\n");
    printf("====================================\n");
    fflush(stdout);
    // sen signal for each player and then write the player on its private fifo!
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        kill(players_pids[i], SIGUSR1);
        sleep(1);
        char fifo_name[20];
        sprintf(fifo_name, "/tmp/fifo%d", players_pids[i]);
        int private_fifo_fd = open(fifo_name, O_WRONLY);
        if (private_fifo_fd == -1)
        {
            perror("open");
            exit(1);
        }
        if (write(private_fifo_fd, &players[i], sizeof(struct Player)) == -1)
        {
            perror("writing to private fifo");
            exit(1);
        }
        printf("Writing player of id %d and pid %d to private fifo %s\n", players[i].id, players_pids[i], fifo_name);
        close(private_fifo_fd);
        sleep(2);
        fflush(stdout);
    }
    printf("Players written to private fifos\n");
    int index = 0;
    while (running)
    {
        if (!index)
        {
            printf("Parent process is waiting for a signal\n");
        }
        index++;
    }

    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        wait(NULL);
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
        kill(players_pids[5], SIGUSR2); // team A leader
        printf("Sent signal to team A leader with pid %d and id %d\n", players_pids[5], players[5].id);
        kill(players_pids[11], SIGUSR2); // team B leader
        printf("Sent signal to team B leader with pid %d and id %d\n", players_pids[11], players[11].id);
        printf("Round %d has started\n", current_round);
        sleep(ROUND_TIME);
        printf("Round %d has ended\n", current_round);
        printf("====================================\n");
        // send signals to the players to stop
        for (int i = 0; i < 2 * NUM_PLAYERS; i++)
        {
            printf("Stopping player %d with pid %d\n", i, players_pids[i]);
            kill(players_pids[i], SIGTSTP);
        }
        sleep(1);
        // at the end of the round, read the players info from the private fifos
        for (int i = 0; i < 2 * NUM_PLAYERS; i++)
        {
            char fifo_name[20];
            sprintf(fifo_name, "/tmp/fifo%d", players_pids[i]);
            int read_fd = open(fifo_name, O_RDONLY);
            if (read_fd == -1)
            {
                perror("Error opening private fifo for reading");
                exit(1);
            }
            if (read(read_fd, &players[i], sizeof(struct Player)) == -1)
            {
                perror("Error reading from private fifo");
                exit(1);
            }
            printf("Reading player of id %d and pid %d to private fifo %s\n", players[i].id, players_pids[i], fifo_name);
            close(read_fd);
            // print current player info
            print_player(players[i]);
            // fflush(stdout);
        }

        // check who won that round and update the lost rounds for each team
        for (int i = 0; i < 2 * NUM_PLAYERS; i++)
        {
            if (players[i].team_name == 'A')
            {
                if (players[i].has_ball)
                {
                    num_of_balls_teamA++;
                }
            }
            else if (players[i].team_name == 'B')
            {
                if (players[i].has_ball)
                {
                    num_of_balls_teamB++;
                }
            }
        }

        if (num_of_balls_teamA > num_of_balls_teamB)
        {
            num_of_lost_rounds_teamA++;
            printf("Team B Won round %d\n", current_round);
        }
        else if (num_of_balls_teamA < num_of_balls_teamB)
        {
            num_of_lost_rounds_teamB++;
            printf("Team A Won round %d\n", current_round);
        }
        else
        {
            printf("Round %d is a draw\n", current_round);
        }
        current_round++;
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

// create a public fifo
void create_public_fifo()
{

    // create private fifos for each player , nameing will belike this "fifo%d" where %d is the pid of the player
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        char fifo_name[20];
        sprintf(fifo_name, "/tmp/fifo%d", players_pids[i]);
        if (access(fifo_name, F_OK) == 0)
        {
            if (remove(fifo_name) == -1)
            {
                perror("Error removing fifo");
                exit(-1);
            }
        }
        if (mkfifo(fifo_name, 0666) == -1)
        {
            perror("Error creating fifo");
            exit(-1);
        }
    }
    printf("Private fifos created\n");

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
        return (rand() % range) + MIN_LEADER_ENERGY;
    }
    else
    {
        range = MAX_PLAYER_ENERGY - MIN_PLAYER_ENERGY + 1;
        return (rand() % range) + MIN_PLAYER_ENERGY;
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
    struct sigaction sa;
    sa.sa_handler = parent_signals_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("Error setting signal handler for SIGUSR1");
        exit(1);
    }

    struct sigaction sa2;
    sa2.sa_handler = parent_signals_handler;
    sa2.sa_flags = SA_RESTART;
    sigemptyset(&sa2.sa_mask);
    if (sigaction(SIGQUIT, &sa2, NULL) == -1)
    {
        perror("Error setting signal handler for SIGQUIT");
        exit(1);
    }
}

void print_player(struct Player player)
{
    printf("Player %d from team %c has %d energy and %s the ball and next player is %d and team leader is %d\n",
           player.id, player.team_name, player.energy, player.has_ball ? "has" : "does not have", player.next_player_pid, player.team_leader_pid);
}
