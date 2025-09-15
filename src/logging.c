#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>

#include "logging.h"
#include "const.h"

typedef struct {
    char filename[MAX_FILENAME_LEN];
    FILE* file;
    log_level_t min_level;
    int initialized;
} logger_t;

// Variable globale du logger
static logger_t g_logger = {0};

// Fonctions privées
static const char* log_level_to_string(log_level_t level);
static void get_timestamp(char* buffer, size_t buffer_size);
static int find_next_log_file(const char* base_filename, char* result_filename, size_t filename_len);

/**
 * Initialise le logger avec le nom de fichier de base et le niveau minimum.
 * Crée un nouveau fichier de log avec un numéro incrémental si le fichier existe déjà.
 * @param base_filename Le nom de fichier de base pour les logs
 * @param min_level Le niveau minimum de log à enregistrer
 * @return 0 si succès, -1 si erreur
 */
int logger_init(const char* base_filename, log_level_t min_level) {
    if (!base_filename || strlen(base_filename) == 0) {
        fprintf(stderr, "Logger init error: Invalid base filename\n");
        return -1;
    }

    logger_cleanup();

    if (find_next_log_file(base_filename, g_logger.filename, MAX_FILENAME_LEN) != 0) {
        fprintf(stderr, "Erreur init logger : Impossible de déterminer le nom du fichier.\n");
        return -1;
    }

    g_logger.file = fopen(g_logger.filename, "w");  // Create new file
    if (!g_logger.file) {
        fprintf(stderr, "Erreur init logger : Impossible d'ouvrir le fichier '%s': %s\n", 
                g_logger.filename, strerror(errno));
        return -1;
    }

    // Set line buffering for immediate writes
    setvbuf(g_logger.file, NULL, _IOLBF, 0);

    g_logger.min_level = min_level;
    g_logger.initialized = 1;

    return 0;
}

/**
 * Nettoie et ferme le logger.
 * @return void
 */
void logger_cleanup(void) {
    if (g_logger.initialized && g_logger.file) {
        LOG_INFO_MSG("Extinction du logger...");
        fclose(g_logger.file);
    }
    
    memset(&g_logger, 0, sizeof(logger_t));
}

/**
 * Enregistre un message de log avec le niveau spécifié.
 * @param level Le niveau de log
 * @param format La chaîne de format (comme printf)
 * @param ... Les arguments pour la chaîne de format
 * @return 0 si succès, -1 si erreur
 */
int logger_log(log_level_t level, const char* format, ...) {
    if (!g_logger.initialized || !g_logger.file) {
        fprintf(stderr, "Erreur logger : Non initialisé\n");
        return -1;
    }

    if (!format) {
        fprintf(stderr, "Erreur logger : String de format est NULL\n");
        return -1;
    }

    if (level < g_logger.min_level) {
        return 0; // Skip logging en dessous du niveau minimum
    }

    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));

    // Formattage du message
    char message[MAX_LOG_MESSAGE_LEN];
    va_list args;
    va_start(args, format);
    int msg_len = vsnprintf(message, MAX_LOG_MESSAGE_LEN, format, args);
    va_end(args);

    if (msg_len < 0) {
        fprintf(stderr, "Erreur logger : Impossible de formatter le message\n");
        return -1;
    }

    // Tronquer si le message est trop long
    if (msg_len >= MAX_LOG_MESSAGE_LEN) {
        message[MAX_LOG_MESSAGE_LEN - 1] = '\0';
    }

    int result = fprintf(g_logger.file, "[%s] [%s] %s\n", 
                        timestamp, log_level_to_string(level), message);
    
    if (result < 0) {
        fprintf(stderr, "Erreur logger : Impossible d'écrire dans le fichier de log : %s\n", strerror(errno));
        return -1;
    }

    // Flush pour write immédiat
    fflush(g_logger.file);

    return 0;
}

/**
 * Obtient le nom du fichier de log actuel.
 * @return Le nom du fichier de log, ou NULL si non initialisé
 */
const char* logger_get_filename(void) {
    if (!g_logger.initialized) {
        return NULL;
    }
    return g_logger.filename;
}

/**
 * Vérifie si le logger est initialisé.
 * @return 1 si initialisé, 0 sinon
 */
int logger_is_initialized(void) {
    return g_logger.initialized;
}

/**
 * Convertit le niveau de log en chaîne de caractères.
 * @param level Le niveau de log
 * @return La chaîne de caractères correspondante
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
 * Obtient le timestamp actuel formaté en chaîne.
 * @param buffer Le buffer pour stocker le timestamp
 * @param buffer_size La taille du buffer
 * @return void
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
 * Obtient le prochain nom de fichier de log disponible en incrémentant un numéro.
 * @param base_filename Le nom de fichier de base
 * @param result_filename Le buffer pour stocker le nom de fichier résultant
 * @param filename_len La taille du buffer
 * @return 0 si succès, -1 si erreur (par exemple, trop de fichiers
 */
static int find_next_log_file(const char* base_filename, char* result_filename, size_t filename_len) {
    struct stat st;
    int file_number = 0;
    
    if (stat(base_filename, &st) != 0) {
        strncpy(result_filename, base_filename, filename_len - 1);
        result_filename[filename_len - 1] = '\0';
        return 0;
    }

    do {
        file_number++;
        snprintf(result_filename, filename_len, "%s.%d", base_filename, file_number);
        
        if (stat(result_filename, &st) != 0) {
            return 0;
        }
        
    } while (file_number < MAX_LOG_FILES);
    
    fprintf(stderr, "Erreur logger : Trop de fichier de log (max: %d)\n", MAX_LOG_FILES);
    return -1;
}