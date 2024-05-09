#ifndef SKI_BUS
#define SKI_BUS

// Standard library and system headers necessary for process control, file I/O, and IPC
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>


// Constants defining the limits and expected parameters of the ski bus simulation
#define MAX_PASSENGERS 100 // Maximum number of passengers that can be handled
#define EXPECTED_ARGS 6 // Number of expected command-line arguments
#define MAX_STOPS 10 // Maximum number of bus stops
#define MAX_SKIERS 19999 // Maximum number of skiers allowed in the simulation

// External declarations of global shared variables used across multiple source files
extern sem_t *bus_sem[MAX_STOPS]; // Semaphores for managing boarding at each bus stop
extern sem_t *ready_sem; // Semaphore to signal readiness of skiers to board/leave
extern sem_t *final_sem; // Semaphore to handle final operations at the last stop
extern sem_t *log_sem; // Semaphore to control access to logging operations
extern FILE *out_file; // File pointer for output logging

extern int *action_count; // Shared counter for actions logged in the output file
extern int *skiers_waiting_ptr; // Array counting waiting skiers at each stop
extern int *skiers_boarded_ptr; // Array counting skiers who have boarded at each stop

// Prototypes of functions used in the ski bus simulation
void log_action(const char *action); // Logs actions to the output file
void cleanUp(int num_stops); // Cleans up resources at program end
int initializeResources(int num_stops); // Initializes IPC resources
void skier_process(int id, int stop_id, int wait_time); // Process routine for a single skier
void bus_process(int stops, int travel_time, int max_capacity, int total_skiers); // Bus operation routine
int validateArguments(int argc, char *argv[]); // Validates the command-line arguments
int passengersDistributed(int stop[], int num_stops, int num_skiers); // Distribution of passengers across stops
int main(int argc, char *argv[]); // Main program entry point

#endif // SKI_BUS
