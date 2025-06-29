#ifndef ZAPPY_UTILS_H
#define ZAPPY_UTILS_H

#include <stdio.h>

/**
 * @brief Affiche un message d’erreur et quitte.
 */
void die(const char *msg);

/**
 * @brief Trace d’informations (debug).
 */
void log_info(const char *fmt, ...);

#endif // ZAPPY_UTILS_H
