#ifndef LOGGING_H
#define LOGGING_H

#include <stddef.h>

/**
 * Log levels enumeration
 */
typedef enum {
    LOG_SUCCESS,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

/**
 * Initialize the logger with base filename
 * Automatically finds the next available log file number at startup
 * 
 * @param base_filename Base name for log files (e.g., "application.log")
 * @param min_level Minimum log level to write
 * @return 0 on success, -1 on error
 */
int logger_init(const char* base_filename, log_level_t min_level);

/**
 * Clean up logger resources
 * Must be called before program exit
 */
void logger_cleanup(void);

/**
 * Log a message with specified level
 * 
 * @param level Log level
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 * @return 0 on success, -1 on error
 */
int logger_log(log_level_t level, const char* format, ...);

/**
 * Get the current log filename being used
 * 
 * @return Current log filename, or NULL if logger not initialized
 */
const char* logger_get_filename(void);

/**
 * Check if logger is initialized
 * 
 * @return 1 if initialized, 0 if not
 */
int logger_is_initialized(void);

// Convenience macros for different log levels
#define LOG_SUCCESS_MSG(fmt, ...) logger_log(LOG_SUCCESS, fmt, ##__VA_ARGS__)
#define LOG_DEBUG_MSG(fmt, ...) logger_log(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO_MSG(fmt, ...)  logger_log(LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN_MSG(fmt, ...)  logger_log(LOG_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR_MSG(fmt, ...) logger_log(LOG_ERROR, fmt, ##__VA_ARGS__)

#endif /* LOGGING_H */