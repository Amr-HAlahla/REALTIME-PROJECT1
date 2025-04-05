# Real-Time Ball Passing Game Simulation

A multi-process simulation of a team-based ball passing game implemented in C, featuring real-time visualization and process management.

## Overview

This project simulates a ball passing game between two teams, where players must manage their energy while passing balls between team members and across teams through their leaders. The simulation includes a graphical interface for real-time visualization of the game.

## Features

- **Team-based Gameplay**
  - Two teams (Team A and Team B) with 6 players each
  - Special leader players with unique abilities
  - Team coordination and strategy

- **Energy Management System**
  - Players consume energy when performing actions
  - Different energy thresholds for leaders and regular players
  - Energy recharge mechanics

- **Real-time Visualization**
  - Graphical interface showing player positions
  - Ball movement tracking
  - Team color coding
  - Real-time game state updates

- **Game Mechanics**
  - Ball passing between players
  - Special leader-to-leader passing
  - Success/failure probabilities
  - Round-based gameplay
  - Pause/resume functionality

## Technical Implementation

- **Process Management**
  - Parent process for game coordination
  - Individual player processes
  - GUI process for visualization

- **Inter-process Communication**
  - Unix signals for event handling
  - FIFO pipes for data transfer
  - Process groups for coordinated actions

## Requirements

- C compiler (gcc)
- OpenGL/GLUT libraries
- Unix-like operating system
- Make build system

## Building the Project

1. Clone the repository:
```bash
git clone [repository-url]
cd [repository-name]
```

2. Build the project:
```bash
make
```

Optional build flags:
- `GUI=1`: Enable graphical interface
- `ROUND_TIME=<seconds>`: Set round duration
- `GAME_TIME=<seconds>`: Set total game duration

Example:
```bash
make GUI=1 ROUND_TIME=45 GAME_TIME=180
```

## Running the Game

```bash
./parent
```

## Game Rules

1. Each team starts with a ball
2. Players must maintain energy to participate
3. Regular players pass to next player in sequence
4. Leaders can pass to other team's leader
5. Game ends when:
   - Time limit reached
   - A team loses maximum allowed rounds

## Configuration

The game can be configured by modifying `config.h`:

- `NUM_PLAYERS`: Players per team
- `MIN_PLAYER_ENERGY`/`MAX_PLAYER_ENERGY`: Energy thresholds
- `MIN_LEADER_ENERGY`/`MAX_LEADER_ENERGY`: Leader energy thresholds
- `MAX_LOST_ROUNDS`: Maximum rounds a team can lose
- `THROW_ENERGY_COST`: Energy cost for throwing
- `PICKUP_ENERGY_COST`: Energy cost for picking up

## Project Structure

```
.
├── parent.c         # Main game process
├── player.c         # Player process implementation
├── gui.c           # Graphical interface
├── config.h        # Configuration and shared definitions
└── Makefile        # Build system
```

## Signal Handling

The game uses various Unix signals for communication:

- `SIGUSR1`: Ball receiving
- `SIGUSR2`: Normal ball passing
- `SIGINT`: Leader-specific ball passing
- `SIGTSTP`: Game pause/resume
- `SIGQUIT`: Game termination
