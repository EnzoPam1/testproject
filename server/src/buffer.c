/*
** EPITECH PROJECT, 2025
** zappy_server
** File description:
** Buffer management - CORRECTED VERSION
*/

#include "buffer.h"
#include <string.h>

void buffer_init(buffer_t *buf) {
    buf->head = buf->tail = 0;
}

size_t buffer_write(buffer_t *buf, const char *data, size_t len) {
    size_t written = 0;
    while (written < len && ((buf->tail + 1) % BUF_SIZE) != buf->head) {
        buf->data[buf->tail] = data[written++];
        buf->tail = (buf->tail + 1) % BUF_SIZE;
    }
    return written;
}

size_t buffer_readline(buffer_t *buf, char *out, size_t maxlen) {
    if (buf->head == buf->tail) {
        return 0; // Buffer is empty
    }
    
    // Look for '\n' or '\r'
    size_t i = buf->head;
    size_t line_len = 0;
    
    while (i != buf->tail) {
        if (buf->data[i] == '\n' || buf->data[i] == '\r') {
            // Found end of line, copy the line (without terminator)
            if (line_len >= maxlen) line_len = maxlen - 1;
            
            for (size_t j = 0; j < line_len; j++) {
                out[j] = buf->data[(buf->head + j) % BUF_SIZE];
            }
            out[line_len] = '\0';
            
            // Advance head past the line and terminator
            buf->head = (i + 1) % BUF_SIZE;
            
            // Skip additional terminators (\r\n or \n\r)
            while (buf->head != buf->tail && 
                   (buf->data[buf->head] == '\n' || buf->data[buf->head] == '\r')) {
                buf->head = (buf->head + 1) % BUF_SIZE;
            }
            
            return line_len;
        }
        line_len++;
        i = (i + 1) % BUF_SIZE;
    }
    return 0; // No complete line found
}

size_t buffer_available(buffer_t *buf) {
    if (buf->tail >= buf->head) {
        return buf->tail - buf->head;
    } else {
        return (BUF_SIZE - buf->head) + buf->tail;
    }
}