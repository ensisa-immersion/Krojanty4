#include "src/logging.c"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("=== Logger Example ===\n");
    
    // Initialize logger
    if (logger_init("application.log", LOG_DEBUG) != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }
    
    printf("Using log file: %s\n", logger_get_filename());
    
    // Example log messages
    LOG_INFO_MSG("Application started successfully");
    LOG_DEBUG_MSG("Debug information: process ID = %d", getpid());
    LOG_WARN_MSG("This is a warning message");
    LOG_ERROR_MSG("Simulated error: %s", "Connection timeout");
    
    // Log some activity
    for (int i = 1; i <= 5; i++) {
        LOG_INFO_MSG("Processing item %d of 5", i);
        sleep(1);  // Simulate some work
    }
    
    LOG_INFO_MSG("All items processed successfully");
    LOG_INFO_MSG("Application shutting down gracefully");
    
    // Clean up
    logger_cleanup();
    
    printf("Logging complete. Check %s\n", "application.log (or numbered version)");
    printf("Run this program multiple times to see log file numbering in action!\n");
    
    return 0;
}