#include "ipgeo.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct iprange {
    uint32_t begin;
    uint32_t end;
    char code[2];
} iprange_t;

struct ipgeodb {
    latlong_t avg[26][26];
    iprange_t *range;
    size_t range_len;
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
            buf,
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

    fclose(ccdb_fp);
    
    db->range = malloc(sizeof(iprange_t) * 133500);

    FILE *ipdb_fp = fopen(ipdb, "r");
    for(;;) {
        uint32_t begin;
        uint32_t end;
        char cc[2];

        char *buf = fgets(linebuf, LINE_CAP, ipdb_fp);
        if(buf == NULL) {
            break;
        }

        int rc = sscanf(
            buf,
            "%u,%u,%c%c\n",
            &begin,
            &end,
            cc,
            cc + 1
        );

        if(rc == 0) {
            continue;
        }

        db->range[db->range_len++] = (iprange_t){
            .begin = begin,
            .end = end,
            .code[0] = cc[0],
            .code[1] = cc[1],
        };
    }

    fclose(ipdb_fp);

    return db;
}


bool ipgeodb_lookup(ipgeodb_t *db, in_addr_t addr, latlong_t *loc) {
    for(size_t i = 0; i < db->range_len; ++i) {
        if(db->range[i].begin <= addr && db->range[i].end >= addr) {
            char cc1 = db->range[i].code[0] - 'A';
            char cc2 = db->range[i].code[1] - 'A';
            *loc = db->avg[cc1][cc2];
            return true;
        }
    }

    return false;
}
