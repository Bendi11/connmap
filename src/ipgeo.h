#pragma once

#include <netinet/in.h>
#include <stdbool.h>


typedef struct latlong {
    float lat;
    float lon;
} latlong_t;

typedef struct ipgeodb ipgeodb_t;

ipgeodb_t* ipgeodb_open(char const *ipdb, char const *ccode_db);

bool ipgeodb_lookup(ipgeodb_t *db, in_addr_t addr, latlong_t *loc);
