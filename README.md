# Synchronization – Skibus
This project, inspired by Allen B. Downey's book The Little Book of Semaphores (The Senate Bus problem), simulates a ski bus system using synchronization mechanisms in C. The system includes three types of processes: the main process, the ski bus, and the skiers. Each skier waits at a bus stop after breakfast, boards the ski bus upon its arrival, and gets off at the final stop near the ski lift. If there are more skiers waiting than the bus capacity, the remaining skiers wait for the next bus.

Project Details
Language: C
Processes: Main process, Ski bus, Skiers
Synchronization: Shared memory and semaphores
Output File: proj2.out
Program Execution
Run the program using the following command:

sh
Copy code
./proj2 L Z K TL TB
Parameters
L: Number of skiers (L < 20000)
Z: Number of bus stops (0 < Z <= 10)
K: Bus capacity (10 <= K <= 100)
TL: Max time in microseconds for a skier to arrive at the stop (0 <= TL <= 10000)
TB: Max bus travel time between stops in microseconds (0 <= TB <= 1000)
Error Handling
If input parameters are invalid or out of range, the program prints an error message, releases all allocated resources, and exits with code 1.
On semaphore or shared memory operation failure, the program prints an error message, releases all allocated resources, and exits with code 1.
Implementation Notes
Processes log their actions in proj2.out with sequential action numbers.
Use shared memory for action counters and synchronization variables.
Use semaphores for synchronization, avoid busy-waiting.
Ensure proper resource deallocation upon process termination.
Compilation and Execution
Use make to compile the project. Ensure the presence of a Makefile.
The executable will be named proj2 and should reside in the same directory as the Makefile.
