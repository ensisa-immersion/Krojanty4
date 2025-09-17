/**
 * @file logging.c
 * @brief Implémentation du système de logging pour l'application
 * 
 * Ce fichier contient toutes les fonctions liées au système de logging, incluant :
 * - L'initialisation et la configuration du logger avec niveaux de log
 * - L'écriture formatée de messages dans des fichiers de log horodatés
 * - La gestion des différents niveaux de log (DEBUG, INFO, WARN, ERROR, SUCCESS)
 * - La fermeture propre et le nettoyage des ressources du logger
 * - Les utilitaires de formatage de timestamps et de conversion de niveaux
 * 
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>

#include "logging.h"
#include "const.h"

/**
 * @struct logger_t
 * @brief Structure de configuration du logger global
 * 
 * Cette structure contient toute la configuration et l'état du système
 * de logging de l'application, incluant le fichier de sortie et les niveaux.
 */
typedef struct {
    char filename[MAX_FILENAME_LEN];  /**< Nom du fichier de log actuel */
    FILE* file;                       /**< Pointeur vers le fichier ouvert */
    log_level_t min_level;           /**< Niveau minimum de log à enregistrer */
    int initialized;                  /**< Flag d'initialisation du logger */
} logger_t;

/** @brief Instance globale unique du logger */
static logger_t g_logger = {0};

// Déclarations des fonctions privées
static const char* log_level_to_string(log_level_t level);
static void get_timestamp(char* buffer, size_t buffer_size);

/**
 * @brief Initialise le logger avec fichier de base et niveau minimum
 * 
 * Cette fonction configure le système de logging en ouvrant un fichier
 * de destination et en définissant le niveau minimum de log. Le fichier
 * est ouvert en mode append et configuré avec un buffer de ligne pour
 * des écritures immédiates. Nettoie tout logger précédent avant l'init.
 * 
 * @param base_filename Le nom de fichier de base pour les logs
 * @param min_level Le niveau minimum de log à enregistrer
 * @return int 0 si succès, -1 si erreur
 */
int logger_init(const char* base_filename, log_level_t min_level) {
    // Validation du nom de fichier fourni
    if (!base_filename || strlen(base_filename) == 0) {
        fprintf(stderr, "Logger init error: Invalid base filename\n");
        return -1;
    }

    // Nettoyage de toute instance précédente du logger
    logger_cleanup();

    // Copie sécurisée du nom de fichier avec terminaison null garantie
    strncpy(g_logger.filename, base_filename, MAX_FILENAME_LEN - 1);
    g_logger.filename[MAX_FILENAME_LEN - 1] = '\0';

    // Ouverture du fichier en mode append avec gestion d'erreur
    g_logger.file = fopen(g_logger.filename, "a");
    if (!g_logger.file) {
        fprintf(stderr, "Erreur init logger : Impossible d'ouvrir le fichier '%s': %s\n", 
                g_logger.filename, strerror(errno));
        return -1;
    }

    // Configuration du buffer de ligne pour écriture immédiate
    setvbuf(g_logger.file, NULL, _IOLBF, 0);

    // Finalisation de la configuration du logger
    g_logger.min_level = min_level;
    g_logger.initialized = 1;

    return 0;
}

/**
 * @brief Nettoie et ferme proprement le logger
 * 
 * Cette fonction ferme le fichier de log ouvert, écrit un message
 * de fermeture, et remet à zéro toute la structure du logger global.
 * Elle peut être appelée de manière sécurisée même si le logger
 * n'était pas initialisé.
 * 
 * @return void
 */
void logger_cleanup(void) {
    // Fermeture propre avec message de fin si logger actif
    if (g_logger.initialized && g_logger.file) {
        LOG_INFO_MSG("Extinction du logger...");
        fclose(g_logger.file);
    }
    
    // Remise à zéro complète de la structure globale
    memset(&g_logger, 0, sizeof(logger_t));
}

/**
 * @brief Enregistre un message formaté dans le fichier de log
 * 
 * Cette fonction écrit un message avec timestamp et niveau dans le fichier
 * de log. Elle vérifie le niveau minimum configuré, formate le message avec
 * les arguments variables (style printf), et assure l'écriture immédiate
 * avec flush. Gère la troncature des messages trop longs automatiquement.
 * 
 * @param level Le niveau de log du message
 * @param format La chaîne de format (comme printf)
 * @param ... Les arguments pour la chaîne de format
 * @return int 0 si succès, -1 si erreur
 */
int logger_log(log_level_t level, const char* format, ...) {
    // Vérification de l'état d'initialisation du logger
    if (!g_logger.initialized || !g_logger.file) {
        fprintf(stderr, "Erreur logger : Non initialisé\n");
        return -1;
    }

    // Validation de la chaîne de format fournie
    if (!format) {
        fprintf(stderr, "Erreur logger : String de format est NULL\n");
        return -1;
    }

    // Filtrage selon le niveau minimum configuré
    if (level < g_logger.min_level) {
        return 0; // Skip logging en dessous du niveau minimum
    }

    // Génération du timestamp formaté pour l'entrée de log
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));

    // Formatage du message avec arguments variables
    char message[MAX_LOG_MESSAGE_LEN];
    va_list args;
    va_start(args, format);
    int msg_len = vsnprintf(message, MAX_LOG_MESSAGE_LEN, format, args);
    va_end(args);

    // Gestion d'erreur de formatage
    if (msg_len < 0) {
        fprintf(stderr, "Erreur logger : Impossible de formatter le message\n");
        return -1;
    }

    // Troncature automatique si message trop long
    if (msg_len >= MAX_LOG_MESSAGE_LEN) {
        message[MAX_LOG_MESSAGE_LEN - 1] = '\0';
    }

    // Écriture de l'entrée complète dans le fichier de log
    int result = fprintf(g_logger.file, "[%s] [%s] %s\n", 
                        timestamp, log_level_to_string(level), message);
    
    // Gestion d'erreur d'écriture dans le fichier
    if (result < 0) {
        fprintf(stderr, "Erreur logger : Impossible d'écrire dans le fichier de log : %s\n", strerror(errno));
        return -1;
    }

    // Flush pour garantir l'écriture immédiate sur disque
    fflush(g_logger.file);

    return 0;
}

/**
 * @brief Obtient le nom du fichier de log actuellement utilisé
 * 
 * Cette fonction retourne le nom du fichier de log configuré lors
 * de l'initialisation. Utile pour affichage ou vérification du
 * fichier de destination des logs.
 * 
 * @return const char* Le nom du fichier de log, ou NULL si non initialisé
 */
const char* logger_get_filename(void) {
    if (!g_logger.initialized) {
        return NULL;
    }
    return g_logger.filename;
}

/**
 * @brief Vérifie l'état d'initialisation du logger
 * 
 * Cette fonction permet de vérifier si le logger a été correctement
 * initialisé avant d'effectuer des opérations de logging. Utile pour
 * éviter les erreurs dans les modules qui utilisent le logger.
 * 
 * @return int 1 si initialisé et prêt à l'usage, 0 sinon
 */
int logger_is_initialized(void) {
    return g_logger.initialized;
}

/**
 * @brief Convertit un niveau de log en chaîne de caractères
 * 
 * Cette fonction utilitaire convertit les valeurs énumérées des niveaux
 * de log en chaînes lisibles pour l'affichage dans les fichiers de log.
 * Gère tous les niveaux définis et retourne "UNKN" pour valeurs invalides.
 * 
 * @param level Le niveau de log à convertir
 * @return const char* La chaîne de caractères correspondante au niveau
 */
static const char* log_level_to_string(log_level_t level) {
    switch (level) {
        case LOG_SUCCESS: return "SUCCESS";
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "UNKN";
    }
}

/**
 * @brief Génère un timestamp formaté pour les entrées de log
 * 
 * Cette fonction obtient l'heure système actuelle et la formate
 * en chaîne lisible (YYYY-MM-DD HH:MM:SS). Gère les erreurs de
 * système et fournit un fallback en cas de problème avec l'horloge.
 * 
 * @param buffer Le buffer de destination pour le timestamp
 * @param buffer_size La taille disponible dans le buffer
 * @return void
 */
static void get_timestamp(char* buffer, size_t buffer_size) {
    // Validation des paramètres d'entrée
    if (!buffer || buffer_size == 0) return;
    
    // Obtention de l'heure système actuelle
    time_t raw_time;
    struct tm* time_info;
    
    time(&raw_time);
    time_info = localtime(&raw_time);
    
    // Formatage avec fallback en cas d'erreur système
    if (time_info) {
        strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", time_info);
    } else {
        strncpy(buffer, "UNKNOWN-TIME", buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
}
