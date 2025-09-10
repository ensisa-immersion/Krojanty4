#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>

#define MAX_FILENAME_LEN 256
#define MAX_LOG_MESSAGE_LEN 1024
#define MAX_LOG_FILES 1000

typedef struct {
    char filename[MAX_FILENAME_LEN];
    FILE* file;
    log_level_t min_level;
    int initialized;
} logger_t;

// Global logger instance
static logger_t g_logger = {0};

// Private function prototypes
static const char* log_level_to_string(log_level_t level);
static void get_timestamp(char* buffer, size_t buffer_size);
static int find_next_log_file(const char* base_filename, char* result_filename, size_t filename_len);

/**
 * Initialize the logger with base filename
 */
int logger_init(const char* base_filename, log_level_t min_level) {
    if (!base_filename || strlen(base_filename) == 0) {
        fprintf(stderr, "Logger init error: Invalid base filename\n");
        return -1;
    }

    // Clean up any existing logger
    logger_cleanup();

    // Find the next available log file
    if (find_next_log_file(base_filename, g_logger.filename, MAX_FILENAME_LEN) != 0) {
        fprintf(stderr, "Logger init error: Cannot determine log filename\n");
        return -1;
    }

    // Open the log file
    g_logger.file = fopen(g_logger.filename, "w");  // Create new file
    if (!g_logger.file) {
        fprintf(stderr, "Logger init error: Cannot open log file '%s': %s\n", 
                g_logger.filename, strerror(errno));
        return -1;
    }

    // Set line buffering for immediate writes
    setvbuf(g_logger.file, NULL, _IOLBF, 0);

    g_logger.min_level = min_level;
    g_logger.initialized = 1;

    printf("Logger initialized: %s\n", g_logger.filename);
    
    // Log initialization message
    LOG_INFO_MSG("Logger initialized - %s", g_logger.filename);
    
    return 0;
}

/**
 * Clean up logger resources
 */
void logger_cleanup(void) {
    if (g_logger.initialized && g_logger.file) {
        LOG_INFO_MSG("Logger shutting down");
        fclose(g_logger.file);
    }
    
    memset(&g_logger, 0, sizeof(logger_t));
}

/**
 * Log a message with specified level
 */
int logger_log(log_level_t level, const char* format, ...) {
    if (!g_logger.initialized || !g_logger.file) {
        fprintf(stderr, "Logger error: Logger not initialized\n");
        return -1;
    }

    if (!format) {
        fprintf(stderr, "Logger error: Format string is NULL\n");
        return -1;
    }

    if (level < g_logger.min_level) {
        return 0; // Skip logging below minimum level
    }

    // Get timestamp
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));

    // Format the log message
    char message[MAX_LOG_MESSAGE_LEN];
    va_list args;
    va_start(args, format);
    int msg_len = vsnprintf(message, MAX_LOG_MESSAGE_LEN, format, args);
    va_end(args);

    if (msg_len < 0) {
        fprintf(stderr, "Logger error: Failed to format message\n");
        return -1;
    }

    // Truncate message if too long
    if (msg_len >= MAX_LOG_MESSAGE_LEN) {
        message[MAX_LOG_MESSAGE_LEN - 1] = '\0';
    }

    // Write to log file
    int result = fprintf(g_logger.file, "[%s] [%s] %s\n", 
                        timestamp, log_level_to_string(level), message);
    
    if (result < 0) {
        fprintf(stderr, "Logger error: Failed to write to log file: %s\n", strerror(errno));
        return -1;
    }

    // Flush the buffer to ensure immediate write
    fflush(g_logger.file);

    return 0;
}

/**
 * Get the current log filename being used
 */
const char* logger_get_filename(void) {
    if (!g_logger.initialized) {
        return NULL;
    }
    return g_logger.filename;
}

/**
 * Check if logger is initialized
 */
int logger_is_initialized(void) {
    return g_logger.initialized;
}

/**
 * Convert log level enum to string
 */
static const char* log_level_to_string(log_level_t level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO ";
        case LOG_WARN:  return "WARN ";
        case LOG_ERROR: return "ERROR";
        default:        return "UNKN ";
    }
}

/**
 * Get formatted timestamp
 */
static void get_timestamp(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    
    time_t raw_time;
    struct tm* time_info;
    
    time(&raw_time);
    time_info = localtime(&raw_time);
    
    if (time_info) {
        strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", time_info);
    } else {
        strncpy(buffer, "UNKNOWN-TIME", buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
}

/**
 * Find the next available log file number
 */
static int find_next_log_file(const char* base_filename, char* result_filename, size_t filename_len) {
    struct stat st;
    int file_number = 0;
    
    // First try the base filename without number
    if (stat(base_filename, &st) != 0) {
        // File doesn't exist, use base filename
        strncpy(result_filename, base_filename, filename_len - 1);
        result_filename[filename_len - 1] = '\0';
        return 0;
    }
    
    // Base filename exists, try numbered versions
    do {
        file_number++;
        snprintf(result_filename, filename_len, "%s.%d", base_filename, file_number);
        
        if (stat(result_filename, &st) != 0) {
            // File doesn't exist, use this number
            return 0;
        }
        
    } while (file_number < MAX_LOG_FILES);
    
    fprintf(stderr, "Logger error: Too many log files (max: %d)\n", MAX_LOG_FILES);
    return -1;
}