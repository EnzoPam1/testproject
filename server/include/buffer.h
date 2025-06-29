/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Buffer header - CORRECTED VERSION
*/

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
 * @brief Initialize the buffer.
 */
void buffer_init(buffer_t *buf);

/**
 * @brief Add bytes to the buffer.
 */
size_t buffer_write(buffer_t *buf, const char *data, size_t len);

/**
 * @brief Read a complete line (until '\n' or '\r') if available.
 * Returns the number of bytes read, 0 if no complete line available.
 */
size_t buffer_readline(buffer_t *buf, char *out, size_t maxlen);

/**
 * @brief Get number of bytes available in buffer.
 */
size_t buffer_available(buffer_t *buf);

#endif // ZAPPY_BUFFER_H