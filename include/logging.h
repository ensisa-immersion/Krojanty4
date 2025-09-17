/**
 * @file logging.h
 * @brief Système de journalisation avancé pour le projet
 * @author Équipe IMM2526-GR4
 * @date 17 septembre 2025
 * 
 * Ce fichier contient les déclarations pour le système de logging du projet, incluant :
 * - Les niveaux de journalisation hiérarchiques (DEBUG, INFO, WARN, ERROR, SUCCESS)
 * - L'initialisation et la gestion automatique des fichiers de logs
 * - La rotation automatique des fichiers de logs avec numérotation
 * - Les fonctions de logging thread-safe avec formatage printf
 * - Les macros de convenance pour simplifier l'utilisation
 * - La gestion des ressources et nettoyage automatique
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <stddef.h>

/**
 * @enum log_level_t
 * @brief Énumération des niveaux de journalisation
 * 
 * Définit les différents niveaux de sévérité pour les messages de log,
 * organisés par ordre croissant d'importance. Le système peut être configuré
 * pour filtrer les messages selon un niveau minimum.
 */
typedef enum {
    LOG_DEBUG,   /**< Messages de débogage détaillés (développement uniquement) */
    LOG_INFO,    /**< Messages informatifs sur le fonctionnement normal */
    LOG_SUCCESS, /**< Messages confirmant le succès d'opérations importantes */
    LOG_WARN,    /**< Avertissements sur des situations potentiellement problématiques */
    LOG_ERROR    /**< Erreurs nécessitant une attention immédiate */
} log_level_t;

// ============================================================================
// FONCTIONS D'INITIALISATION ET GESTION DU SYSTÈME
// ============================================================================

/**
 * @brief Initialise le système de journalisation avec rotation automatique
 * 
 * Cette fonction configure le système de logging avec un nom de fichier de base
 * et trouve automatiquement le prochain numéro de fichier disponible au démarrage.
 * Le système créera des fichiers de la forme "base_filename.1.log", "base_filename.2.log", etc.
 * Elle configure également le niveau minimum de logging à enregistrer.
 * 
 * @param base_filename Nom de base pour les fichiers de logs (ex: "application.log")
 * @param min_level Niveau minimum de log à enregistrer (filtrage automatique)
 * @return int 0 en cas de succès, -1 en cas d'erreur
 */
int logger_init(const char* base_filename, log_level_t min_level);

/**
 * @brief Nettoie les ressources du système de logging
 * 
 * Cette fonction ferme proprement tous les fichiers ouverts, libère la mémoire
 * allouée et réinitialise l'état interne du logger. Elle doit impérativement
 * être appelée avant la fin du programme pour éviter les fuites de ressources.
 * 
 * @return void
 */
void logger_cleanup(void);

// ============================================================================
// FONCTIONS DE JOURNALISATION PRINCIPALE
// ============================================================================

/**
 * @brief Enregistre un message avec le niveau spécifié
 * 
 * Cette fonction centrale du système de logging écrit un message formaté
 * dans le fichier de log actuel. Elle ajoute automatiquement un timestamp,
 * le niveau de log, et formate le message selon le style printf. La fonction
 * est thread-safe et gère automatiquement le filtrage par niveau.
 * 
 * @param level Niveau de sévérité du message
 * @param format Chaîne de format de style printf
 * @param ... Arguments variables pour la chaîne de format
 * @return int 0 en cas de succès, -1 en cas d'erreur
 */
int logger_log(log_level_t level, const char* format, ...);

// ============================================================================
// FONCTIONS D'INFORMATION ET ÉTAT
// ============================================================================

/**
 * @brief Récupère le nom du fichier de log actuellement utilisé
 * 
 * Cette fonction retourne le chemin complet du fichier de log en cours
 * d'utilisation par le système. Utile pour afficher des informations de
 * débogage ou pour permettre à l'utilisateur de localiser les logs.
 * 
 * @return const char* Nom du fichier de log actuel, ou NULL si non initialisé
 */
const char* logger_get_filename(void);

/**
 * @brief Vérifie si le système de logging est initialisé
 * 
 * Cette fonction utilitaire permet de vérifier l'état d'initialisation
 * du logger avant d'effectuer des opérations de logging. Prévient les
 * erreurs dues à l'utilisation du système avant son initialisation.
 * 
 * @return int 1 si initialisé, 0 si non initialisé
 */
int logger_is_initialized(void);

// ============================================================================
// MACROS DE CONVENANCE POUR SIMPLIFIER L'UTILISATION
// ============================================================================

/**
 * @brief Macro pour enregistrer un message de succès
 * @param fmt Chaîne de format printf
 * @param ... Arguments pour la chaîne de format
 */
#define LOG_SUCCESS_MSG(fmt, ...) logger_log(LOG_SUCCESS, fmt, ##__VA_ARGS__)

/**
 * @brief Macro pour enregistrer un message de débogage
 * @param fmt Chaîne de format printf
 * @param ... Arguments pour la chaîne de format
 */
#define LOG_DEBUG_MSG(fmt, ...) logger_log(LOG_DEBUG, fmt, ##__VA_ARGS__)

/**
 * @brief Macro pour enregistrer un message informatif
 * @param fmt Chaîne de format printf
 * @param ... Arguments pour la chaîne de format
 */
#define LOG_INFO_MSG(fmt, ...)  logger_log(LOG_INFO, fmt, ##__VA_ARGS__)

/**
 * @brief Macro pour enregistrer un avertissement
 * @param fmt Chaîne de format printf
 * @param ... Arguments pour la chaîne de format
 */
#define LOG_WARN_MSG(fmt, ...)  logger_log(LOG_WARN, fmt, ##__VA_ARGS__)

/**
 * @brief Macro pour enregistrer une erreur
 * @param fmt Chaîne de format printf
 * @param ... Arguments pour la chaîne de format
 */
#define LOG_ERROR_MSG(fmt, ...) logger_log(LOG_ERROR, fmt, ##__VA_ARGS__)

#endif /* LOGGING_H */