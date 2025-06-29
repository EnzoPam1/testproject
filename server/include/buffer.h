#ifndef ZAPPY_BUFFER_H
#define ZAPPY_BUFFER_H

#include <stddef.h>

#define BUF_SIZE 4096

typedef struct s_buffer {
    char data[BUF_SIZE];
    size_t head;
    size_t tail;
} buffer_t;

/**
 * @brief Initialise le buffer.
 */
void buffer_init(buffer_t *buf);

/**
 * @brief Ajoute des octets dans le buffer.
 */
size_t buffer_write(buffer_t *buf, const char *data, size_t len);

/**
 * @brief Lit une ligne complète (jusqu’à ‘\n’) si disponible.
 * Retourne le nombre d’octets lus, 0 si aucune ligne complète.
 */
size_t buffer_readline(buffer_t *buf, char *out, size_t maxlen);

#endif // ZAPPY_BUFFER_H
