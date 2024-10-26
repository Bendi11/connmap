#include "ipgeo.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct ipgeodb {
    latlong_t avg[26][26];
};

static inline uint8_t cc_letter_to_idx(char letter) {
    return letter - 'A';
}

ipgeodb_t* ipgeodb_open(char const *ipdb, char const *ccdb) {
    static const size_t LINE_CAP = 1024;
    char linebuf[LINE_CAP];

    ipgeodb_t *db = malloc(sizeof(struct ipgeodb));
    if(db == NULL) {
        return db;
    }

    FILE *ccdb_fp = fopen(ccdb, "r");
    
    int len = 0;
    for(;;) {
        char code[2];
        float lat;
        float lon;
        char *buf = fgets(linebuf, LINE_CAP, ccdb_fp);
        if(buf == NULL) {
            break;
        }
        int rc = sscanf(
            linebuf,
            "\"%*[^\"]\", \"%c%c\", \"%*c%*c%*c\", \"%*d\", \"%f\", \"%f\"\n",
            code,
            code + 1,
            &lat,
            &lon
        );

        if(rc == 0) {
            continue;
        }


        db->avg[cc_letter_to_idx(code[0])][cc_letter_to_idx(code[1])] = (latlong_t){
            .lat = lat,
            .lon = lon,
        };
    }

    return db;
}
