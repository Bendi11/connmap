#pragma once

typedef struct latlong {
    float lat;
    float lon;
} latlong_t;

typedef struct ipgeodb ipgeodb_t;

ipgeodb_t* ipgeodb_open(char const *ipdb, char const *ccode_db);
