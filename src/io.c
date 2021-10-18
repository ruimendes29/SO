#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "io.h"

ssize_t read_block(Buffer *buffer, size_t size) {
    ssize_t r = 0;
    buffer->pt = 0;
    buffer->size = 0;

    if (buffer->capacity < size) {
        buffer->capacity += 2 * size;
        buffer->body = (char *)realloc(buffer->body, buffer->capacity);
    }

    r = read(buffer->fd, buffer->body, buffer->capacity);
    buffer->size += r;

    return r;
}

void create_buffer(int fd, Buffer *buffer, size_t initial_capacity) {
    buffer->fd = fd;
    buffer->size = 0;
    buffer->pt = 0;
    buffer->capacity = initial_capacity;

    buffer->body = (char *)malloc(sizeof(char) * buffer->capacity);
}

void destroy_buffer(Buffer *buffer) { free(buffer->body); }

ssize_t readln(Buffer *buffer, void *buf, size_t nbyte) {
    size_t size = 0;
    ssize_t r = 0;
    char *buff = (char *)buf;
    int i, j = 0, flag = 1;

    while (size < nbyte && flag) {
        if (buffer->pt >= buffer->size) {
            if (read_block(buffer, nbyte) == 0) flag = 0;
        }

        for (i = buffer->pt; i < buffer->size && flag; j++, i++, r++) {
            if (buffer->body[i] == '\n' || buffer->body[i] == '\0') {
                buff[j] = '\0';
                flag = 0;
            } else {
                buff[j] = buffer->body[i];
            }
        }

        buffer->pt = i;
        size += r;

        if (r == 0) flag = 0;
    }

    return size;
}

int contaEspacos(char *string) {
    int r = 0;
    while (*string) {
        if ((*(string++)) == ' ') r++;
    }
    return r;
}