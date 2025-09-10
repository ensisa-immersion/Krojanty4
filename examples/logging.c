#include "src/logging.c"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("=== Exemple Logging ===\n");

    // Mettre ici le nom de fichier de base.
    // Quand on démarre l'app, un nouveau fichier se crée
    // Un nombre va s'incrémenter dans le nom pour faire référence à la partie
    if (logger_init("application.log", LOG_DEBUG) != 0) {
        fprintf(stderr, "Impossible d'initialiser le logger\n");
        return 1;
    }

    // Exemples de messages
    LOG_INFO_MSG("Démarrage application");
    LOG_DEBUG_MSG("ID processus = %d", getpid());
    LOG_WARN_MSG("Message d'alerte");
    LOG_ERROR_MSG("Erreur simulé : %s", "Connection timeout");

    // Clean up
    logger_cleanup();

    return 0;
}