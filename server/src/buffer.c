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
    // cherche '\n'
    size_t i = buf->head;
    while (i != buf->tail) {
        if (buf->data[i] == '\n') {
            // longueur de la ligne (+1 pour '\n')
            size_t len = (i + BUF_SIZE - buf->head) % BUF_SIZE + 1;
            if (len > maxlen - 1) len = maxlen - 1;
            // copie
            for (size_t j = 0; j < len; j++) {
                out[j] = buf->data[(buf->head + j) % BUF_SIZE];
            }
            out[len] = '\0';
            // avance head
            buf->head = (buf->head + len) % BUF_SIZE;
            return len;
        }
        i = (i + 1) % BUF_SIZE;
    }
    return 0;
}
