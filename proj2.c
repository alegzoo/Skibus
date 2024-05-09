#include "proj2.h"

// xmarina00

int stop[MAX_STOPS]; // Array tracking the number of skiers at each stop

// Global shared variables
sem_t *bus_sem[MAX_STOPS];   // Semaphores for each bus stop to manage bus arrivals
sem_t *ready_sem;            // Semaphore to indicate readiness of skiers to board or leave
sem_t *final_sem;            // Semaphore for the final bus stop operation
sem_t *log_sem;              // Semaphore to control access to logging operations
FILE *out_file;              // File for logging actions

int *action_count;           // Pointer to shared memory counter for actions logged
int *skiers_waiting;         // Shared memory array of skiers waiting at each stop
int *skiers_boarded;         // Shared memory array of skiers who have boarded at each stop


// Logs actions to a file, ensuring exclusive access through semaphore control.
// It increments a shared memory counter to track the number of actions logged
void log_action(const char *action) {
    sem_wait(log_sem);  // Wait for the semaphore to ensure exclusive access
    fprintf(out_file, "%d: %s\n", (*action_count)++, action);
    fflush(out_file);
    sem_post(log_sem);  // Release the semaphore after writing
}

// Cleans up resources at the end of the simulatio, this includes unmapping and unlinking
// of shared memory objects, closing files, and unlinking and closing semaphores
// Handles each type of resource individually to ensure all allocated resources are properly released.
void cleanUp(int num_stops) {
    
    if (skiers_waiting) {
        if (munmap(skiers_waiting, sizeof(int) * num_stops) == -1) {
            perror("Error unmapping skiers_waiting");
        }
        if (shm_unlink("/xmarina00_skiers_waiting") == -1) {
            perror("Error unlinking /xmarina00_skiers_waiting");
        }
        skiers_waiting = NULL;
    }

    if (skiers_boarded) {
        if (munmap(skiers_boarded, sizeof(int) * num_stops) == -1) {
            perror("Error unmapping /xmarina00_skiers_boarded");
        }
        if (shm_unlink("/xmarina00_skiers_boarded") == -1) {
            perror("Error unlinking /xmarina00_skiers_boarded");
        }
        skiers_boarded = NULL;
    }

    if (action_count) {
        if (munmap(action_count, sizeof(*action_count)) == -1) {
            perror("Error unmapping action_count");
        }
        if (shm_unlink("/xmarina00_action_count") == -1) {
            perror("Error unlinking /xmarina00_action_count");
        }
        action_count = NULL;
    }

    if (out_file) {
        fclose(out_file);
        out_file = NULL;
    }

    if (ready_sem) {
        sem_close(ready_sem);
        sem_unlink("/xmarina00_ready_sem");
        ready_sem = SEM_FAILED;
    }

    if (final_sem) {
        sem_close(final_sem);
        sem_unlink("/xmarina00_final_sem");
        final_sem = SEM_FAILED;
    }

    if (log_sem) {
        sem_close(log_sem);
        sem_unlink("/xmarina00_log_sem");
        log_sem = SEM_FAILED;
    }

    for (int i = 0; i < MAX_STOPS; i++) {
        if (bus_sem[i]) {
            sem_close(bus_sem[i]);
            char sem_name[256];
            snprintf(sem_name, sizeof(sem_name), "/xmarina00_bus_sem_%d", i);
            sem_unlink(sem_name);
            bus_sem[i] = SEM_FAILED;
        }
    }
}

// Initializes the necessary resources for the simulation, including semaphores for each bus stop and for
// synchronization purposes, shared memory for skier counts, and a log file for output. It sets up each
// resource and handles errors by cleaning up and returning an error status.

int initializeResources(int num_stops) {
    char sem_name[256];
    // Initialize named semaphores for each bus stop
    for (int i = 0; i < num_stops; i++) {
        snprintf(sem_name, sizeof(sem_name), "/xmarina00_bus_sem_%d", i);
        sem_unlink(sem_name);
        bus_sem[i] = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 0);
        if (bus_sem[i] == SEM_FAILED) {
            perror("Failed to open semaphore for a bus stop");
            return 1;
        }
    }

    // Initialize named semaphore for skiers boarding
    sem_unlink("/xmarina00_ready_sem");
    ready_sem = sem_open("/xmarina00_ready_sem", O_CREAT | O_EXCL, 0644, 0);
    if (ready_sem == SEM_FAILED) {
        perror("Failed to open semaphore");
        return 1;
    }

    // Initialize named semaphore for final stop
    sem_unlink("/xmarina00_final_sem");
    final_sem = sem_open("/xmarina00_final_sem", O_CREAT | O_EXCL, 0644, 0);
    if (final_sem == SEM_FAILED) {
        perror("Failed to open semaphore");
        return 1;
    }

    // Initialize named semaphore for handling logging of actions
    sem_unlink("/xmarina00_log_sem");
    log_sem = sem_open("/xmarina00_log_sem", O_CREAT | O_EXCL, 0644, 1);
    if (log_sem == SEM_FAILED) {
        perror("Failed to open semaphore");
        return 1;
    }

    // Setting up shared memory for the number of action logged to the file
    int fd1 = shm_open("/xmarina00_action_count", O_CREAT | O_RDWR, 0666);
    ftruncate(fd1, sizeof(*action_count)); // sets size of the shared memory object
    action_count = mmap(NULL, sizeof(*action_count), PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0);
    *action_count = 1;
    close(fd1);

    // Setting up shared memory for array of skiers waiting on the bus stop to be boarded
    int fd2 = shm_open("/xmarina00_skiers_waiting", O_CREAT | O_RDWR, 0666);
    ftruncate(fd2, sizeof(int) * num_stops);
    skiers_waiting = mmap(NULL, sizeof(int) * num_stops, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);
    if (skiers_waiting == MAP_FAILED) {
        perror("mmap xmarina00_skiers_waiting");
        return 1;
    }
    close(fd2);

    memset(skiers_waiting, 0, sizeof(int) * num_stops);

    // Setting up shared memory for array of skiers that have already boarded to keep track of number of passenges
    int fd3 = shm_open("/xmarina00_skiers_boarded", O_CREAT | O_RDWR, 0666);
    ftruncate(fd3, sizeof(int) * num_stops);
    skiers_boarded = mmap(NULL, sizeof(int) * num_stops, PROT_READ | PROT_WRITE, MAP_SHARED, fd3, 0);
    if (skiers_boarded == MAP_FAILED) {
        perror("mmap xmarina00_skiers_boarded");
        return 1;
    }
    close(fd3);

    memset(skiers_boarded, 0, sizeof(int) * num_stops);

    // Open a file for output
    out_file = fopen("proj2.out", "w");
    if (out_file == NULL) {
        return 1; // Return 1 on error
    }

    return 0; // Return 0 on success
}

// Represents a single skier's journey from arrival at a bus stop to boarding the bus and eventually going skiing.
// It includes randomized waiting times, interaction with semaphores to wait for and board the bus, and logs each action.
void skier_process(int id, int stop_id, int wait_time) {
    char action[256];

    sprintf(action, "L %d: started", id);
    log_action(action);
 
    srand(time(NULL) * getpid());
    usleep(rand() % (wait_time + 1));  // Finishes eating breakfast, walking to the bus stop
    
    // Log skier arrived at the stop
    sprintf(action, "L %d: arrived to %d", id, stop_id + 1);
    log_action(action);

    __sync_fetch_and_add(&skiers_waiting[stop_id], 1); // Atomically increment the count of skiers who are waiting on the stop

    // Wait for the bus to arrive at this stop
    sem_wait(bus_sem[stop_id]);

    // Log skier boarding
    sprintf(action, "L %d: boarding", id);
    log_action(action);

    __sync_fetch_and_add(&skiers_boarded[stop_id], 1);  // Atomically increment the count of skiers who have boarded

    // Signal to bus that the skier has boarded
    sem_post(ready_sem);

    // Wait for the bus to signal departure from the final stop
    sem_wait(final_sem);

    // Log skier going to ski
    sprintf(action, "L %d: going to ski", id);
    log_action(action);

    sem_post(ready_sem);
    exit(0);
}

// Simulates the bus's operation, travelling between stops, picking up skiers until the bus is full, and then moving directly to the final stop.
// Uses semaphores to manage skier boarding and employs a loop to handle multiple rounds of travel if not all skiers are serviced in one go
// The bus_process function simulates the operations of a ski bus as it travels between various stops,
// picks up skiers, and ensures all skiers reach their destination.
void bus_process(int stops, int travel_time, int max_capacity, int total_skiers) {
    char action[256];  // Buffer for logging actions
    int total_boarded = 0;  // Counter for the total number of skiers boarded on the bus
    int rounds = 0;  // Counter for the number of complete rounds made by the bus
    int skiers_delivered = 0;  // Counter for the number of skiers delivered to the final destination
    int stop_boarded = 0;  // Counter for the number of skiers boarded at the current stop
    bool all_boarded = false;  // Flag to check if all skiers have been serviced
    bool bus_full = false;  // Flag to indicate if the bus is at full capacity

    // Log the start of the bus operation
    log_action("BUS: started");

    // Simulate random delay before the bus arrives at the first stop
    srand(time(NULL) * getpid());
    usleep(rand() % (travel_time + 1));

    // Main loop continues until all skiers are boarded and delivered
    while (!all_boarded) {
        // Loop through each bus stop
        for (int i = 0; i < stops; i++) {
            // Simulate travel time between the last and the first stop across rounds
            if (rounds != 0 && i == 0) {
                srand(time(NULL) * getpid());
                usleep(rand() % (travel_time + 1));
            }

            // Log the bus's arrival at the current stop
            sprintf(action, "BUS: arrived to %d", i + 1);
            log_action(action);

            int waiting = skiers_waiting[i];  // Number of skiers waiting at the current stop

            // Board skiers at the current stop if the bus is not full
            if (waiting > 0) {
                for (int j = 0; j < waiting; j++) {
                    if (total_boarded < max_capacity) {
                        sem_post(bus_sem[i]);  // Signal skier to board the bus
                        total_boarded++;
                        stop_boarded++;
                    }
                }
            }

            // Check if the bus has reached its capacity
            if (total_boarded == max_capacity) {
                bus_full = true;
            }

            // Wait for all boarded skiers at the current stop to signal that they're boarded
            if (waiting > 0) {
                for (int j = 0; j < stop_boarded; j++) {
                    sem_wait(ready_sem);
                }
            }

            // Update the number of skiers waiting at the current stop
            skiers_waiting[i] -= skiers_boarded[i];
            skiers_boarded[i] = 0;  // Reset boarded count for the next round

            // Log the bus's departure from the current stop
            sprintf(action, "BUS: leaving %d", i + 1);
            log_action(action);

            // Simulate travel time to the next stop
            srand(time(NULL) * getpid());
            usleep(rand() % (travel_time + 1));

            stop_boarded = 0;  // Reset the stop boarded count for the next stop

            // If the bus is full, skip the remaining stops
            if (bus_full) {
                int total_waitTime = stops - i - 1;  // Calculate remaining stops to simulate
                for (int k = i + 1; k < total_waitTime + i; k++) {
                    sprintf(action, "BUS: arrived to %d", k + 1);
                    log_action(action);
                    usleep(rand() % (travel_time + 1));
                    sprintf(action, "BUS: leaving %d", k + 1);
                    log_action(action);
                }
                bus_full = false;
                stop_boarded = 0;
                break;  // Exit the for-loop to start a new round
            }
        }

        // Log the arrival at the final stop
        log_action("BUS: arrived to final");

        // Signal all skiers to disembark at the final stop
        for (int i = 0; i < total_boarded; i++) {
            sem_post(final_sem);
            sem_wait(ready_sem);
            skiers_delivered++;
        }

        total_boarded = 0;  // Reset the total boarded count for a new round
        rounds++;  // Increment the rounds counter

        // Log the departure from the final stop
        log_action("BUS: leaving final");

        // Check if all skiers have been delivered
        if (skiers_delivered != total_skiers) {
            all_boarded = false;
        } else {
            log_action("BUS: finish");
            break;  // Exit the while-loop if all skiers are delivered
        }
    }
    
    exit(0);  // Terminate the bus process
}


// Validates command-line arguments to ensure they meet expected criteria for the simulation parameters, such as the number of skiers,
// number of stops, bus capacity, and times. It provides specific error messages for each type of validation failure
int validateArguments(int argc, char *argv[]) {

    if (argc != EXPECTED_ARGS) {
        fprintf(stderr, "Usage: %s L Z K TL TB\n", argv[0]);
        return 1;
    }

    // Convert arguments to integers and check their ranges
    int L = atoi(argv[1]); // Number of skiers
    if (L >= 20000) {
        fprintf(stderr, "Error: Number of skiers (L) must be less than 20000 and more than -1.\n");
        return 1;
    }

    int Z = atoi(argv[2]); // Number of bus stops
    if (Z <= 0 || Z > 10) {
        fprintf(stderr, "Error: Number of stops (Z) must be between 1 and 10.\n");
        return 1;
    }

    int K = atoi(argv[3]); // Capacity of the skibus
    if (K < 10 || K > 100) {
        fprintf(stderr, "Error: Skibus capacity (K) must be between 10 and 100.\n");
        return 1;
    }

    int TL = atoi(argv[4]); // Maximum wait time at stop in microseconds
    if (TL < 0 || TL > 10000) {
        fprintf(stderr, "Error: Maximum wait time (TL) must be between 0 and 10000 microseconds.\n");
        return 1;
    }

    int TB = atoi(argv[5]); // Maximum travel time between stops
    if (TB < 0 || TB > 1000) {
        fprintf(stderr, "Error: Maximum travel time (TB) must be between 0 and 1000.\n");
        return 1;
    }

    // If all checks pass
    return 0;
}

// Entry point of the program, setting up the initial conditions based on command-line arguments,
// initializing resources, and creating processes for the bus and skiers. Manages process creation
// errors and ensures all processes are cleaned up properly after execution
int main(int argc, char *argv[]) {
    if (validateArguments(argc, argv)) {
        exit(1);
    }

    int num_skiers = atoi(argv[1]); // Number of skiers
    int num_stops = atoi(argv[2]); // Number of stations
    int max_capacity = atoi(argv[3]); // Number of the maximum capacity of the bus
    int walk_time = atoi(argv[4]); // Max time skier takes to reach bus stop
    int travel_time = atoi(argv[5]); // Max travel time between stations

    if (initializeResources(num_stops)) {
        cleanUp(num_stops);
        exit(1);
    }

    // Create the bus process
    pid_t bus = fork();
    if (bus == 0) {
        bus_process(num_stops, travel_time, max_capacity, num_skiers);
        exit(0);
    } else if (bus < 0) {
        perror("Failed to fork bus process");
        cleanUp(num_stops);
        exit(1);
    }

    int stopId = 0;

    // Create skier processes for each stop
    for (int skierId = 0; skierId < num_skiers; skierId++) {
        stopId = (rand() % num_stops);
        stop[stopId]++;
        pid_t pidPassenger = fork();
        if (pidPassenger == 0) {  // Child process for each skier
            skier_process(skierId + 1, stopId, walk_time);
            exit(0);
        } else if (pidPassenger < 0) {
            perror("Failed to fork skier process");
            cleanUp(num_stops);
            exit(1);
        }
    }

    // Wait for all child processes to finish
    int status;
    while (wait(&status) > 0);
    cleanUp(num_stops);
    exit(0);
}
