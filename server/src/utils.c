/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Utilities implementation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "utils.h"

char *str_trim(char *str)
{
    if (!str) return NULL;
    
    // Trim leading spaces
    while (isspace(*str)) str++;
    
    // All spaces
    if (*str == 0) return str;
    
    // Trim trailing spaces
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    
    // Null terminate
    end[1] = '\0';
    
    return str;
}

char **str_split(const char *str, char delim)
{
    if (!str) return NULL;
    
    // Count tokens
    int count = 1;
    for (const char *p = str; *p; p++) {
        if (*p == delim) count++;
    }
    
    // Allocate array
    char **result = calloc(count + 1, sizeof(char *));
    if (!result) return NULL;
    
    // Split string
    char *copy = strdup(str);
    char *token = strtok(copy, &delim);
    int i = 0;
    
    while (token) {
        result[i++] = strdup(token);
        token = strtok(NULL, &delim);
    }
    
    free(copy);
    return result;
}

void str_array_free(char **array)
{
    if (!array) return;
    
    for (int i = 0; array[i]; i++) {
        free(array[i]);
    }
    free(array);
}

int str_array_len(char **array)
{
    if (!array) return 0;
    
    int len = 0;
    while (array[len]) len++;
    return len;
}

void log_info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("[INFO] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void log_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void log_debug(const char *format, ...)
{
#ifdef DEBUG
    va_list args;
    va_start(args, format);
    printf("[DEBUG] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
#endif
}

void die(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[FATAL] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(84);
}