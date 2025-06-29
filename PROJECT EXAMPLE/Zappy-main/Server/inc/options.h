#pragma once

#include "list.h"

typedef char* team_name_t;

typedef struct
{
    double freq;
    int clientsNb;
    int height;
    int port;
    int width;
    list_t teams;
} option_t;

option_t *options_new(int argc, char *argv[]);
void options_destroy(option_t *options);
