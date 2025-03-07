# ARP-DroneSimGame3

## Short Definitions of Active Components:

### Master (master.c)
- **Purpose:** Creates and manages pipes, and logs drone positions.
- **Primitives Used:** Pipes, semaphores, file operations, fork.

### Server (server.c)
- **Purpose:** Creates and manages pipes, sockets, and logs drone positions.
- **Primitives Used:** Pipes, semaphores, file operations, fork, sockets.

### UI (ui.c)
- **Purpose:** Uses ncurses to display the drone position and UI messages.
- **Primitives Used:** Pipes, semaphores, ncurses.

### Keyboard (keyboard.c)
- **Purpose:** Captures keyboard input and sends it to the drone through a FIFO.
- **Primitives Used:** Pipes, semaphores, FIFO.

### Drone (drone.c)
- **Purpose:** Simulates a drone's movement based on keyboard input.
- **Primitives Used:** Pipes, semaphores.

### Watchdog (watchdog.c)
- **Purpose:** Monitors the status of all processes and logs them.
- **Primitives Used:** Pipes, semaphores.

### Targets (targets.c)
- **Purpose:** Creates targets and communicates with the server.
- **Primitives Used:** Pipes, semaphores, sockets.

### Obstacles (obstacles.c)
- **Purpose:** Creates obstacles and keeps changing their positions periodically.
- **Primitives Used:** Pipes, semaphores, sockets.

## List of Components, Directories, Files:

### Directories:
- `include/`: Contains header files.
- `log/`: Contains log files.
- `/`: Contains source code files.

### Files:
- `master.c`: Main program.
- `server.c`: Server component.
- `ui.c`: UI component.
- `keyboard.c`: Keyboard component.
- `drone.c`: Drone component.
- `targets.c`: Targets component.
- `obstacles.c`: Obstacles component.
- `watchdog.c`: Watchdog component.
- `constants.h`: Header file with constant values.
- `Makefile`: Build automation file.
- `README.md`: Documentation file.

## Instructions for Installing and Running:

### Prerequisites:
- Ensure you have the necessary libraries installed:
  - `ncurses`
  - `konsole`

### Installation:
1. Clone the repository.
2. Create the build directory:
   ```sh
   mkdir build
   ```
3. Compile the components using make:
   ```sh
   make
   ```

### Running:
1. First terminal:
   ```sh
   make
   ./build/master 8080
   ```
2. Second terminal:
   ```sh
   make
   ./build/master 127.0.0.1 8080
   ```

   ps: change the port and host address according to your needs

## Operational Instructions:

### UI Window:
- Displays the drone's position and coordinates.
- No user interaction; visualizes the drone's movement.

### Keyboard Window:
- Press keys to control the drone's movement.
- Logs keypresses.

### Watchdog Window:
- Monitors and logs the status of all processes.

## Control Keys:

### W (Move Up):
- Increments the upward force on the drone, simulating an upward movement.

### A (Move Left):
- Decrements the horizontal force on the drone, simulating a leftward movement.

### S (Move Down):
- Decrements the downward force on the drone, simulating a downward movement.

### D (Move Right):
- Increments the horizontal force on the drone, simulating a rightward movement.

### Q (Move Up-Left):
- Simultaneously decrements horizontal force and increments upward force.

### E (Move Up-Right):
- Simultaneously increments horizontal force and increments upward force.

### Z (Move Down-Left):
- Simultaneously decrements horizontal force and decrements downward force.

### C (Move Down-Right):
- Simultaneously increments horizontal force and decrements downward force.

### X (Stop):
- Sets both horizontal and vertical forces to zero, stopping the drone.

### ESC (Exit):
- Exits the program.
