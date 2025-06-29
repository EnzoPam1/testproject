/*
** EPITECH PROJECT, 2025
** Zappy
** File description:
** Utilities
*/

#ifndef UTILS_H_
#define UTILS_H_

#include <stdarg.h>

// String utilities
char *str_trim(char *str);
char **str_split(const char *str, char delim);
void str_array_free(char **array);
int str_array_len(char **array);

// Logging
void log_info(const char *format, ...);
void log_error(const char *format, ...);
void log_debug(const char *format, ...);

// Error handling
void die(const char *format, ...);

#endif /* !UTILS_H_ */