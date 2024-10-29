#include "ipgeo.h"
#include <dirent.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct iprange {
    uint32_t begin;
    uint32_t end;
    char code[2];
} iprange_t;

typedef struct countrycode {
    char code[2];
} countrycode_t;

typedef struct ipgeodb_entry {
    uint32_t max;
    countrycode_t cc;
} ipgeodb_entry_t;

struct ipgeodb {
    latlong_t avg[26][26];
    ipgeodb_entry_t *sortbuf;
    uint32_t cap;
    uint32_t len;
};

void ipgeodb_new(ipgeodb_t *db) {
    const size_t PRE_KNOWN_CAPACITY = 350000;
    db->sortbuf = malloc(sizeof(ipgeodb_entry_t) * PRE_KNOWN_CAPACITY);
    db->cap = PRE_KNOWN_CAPACITY;
    db->len = 0;
}

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

    ipgeodb_new(db);

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
    

    FILE *ipdb_fp = fopen(ipdb, "r");
    for(;;) {
        uint32_t end;
        countrycode_t cc;

        char *buf = fgets(linebuf, LINE_CAP, ipdb_fp);
        if(buf == NULL) {
            break;
        }

        int rc = sscanf(
            buf,
            "%*u,%u,%c%c\n",
            &end,
            &cc.code[0],
            &cc.code[1]
        );

        cc.code[0] -= 'A';
        cc.code[1] -= 'A';

        if(rc == 0) {
            continue;
        }

        db->sortbuf[db->len++] = (ipgeodb_entry_t){
            .cc = cc,
            .max = end
        };
    }

    for(size_t i = 1; i < db->len; ++i) {
        if(db->sortbuf[i].max < db->sortbuf[i - 1].max) {
            exit(-1);
        }
    }

    fclose(ipdb_fp);

    return db;
}


bool ipgeodb_lookup(ipgeodb_t *db, in_addr_t addr, latlong_t *loc) {
    if(addr == 0 || addr == htonl(0x0100007f)) {
        return false;
    }

    size_t alpha = 0;
    size_t beta = db->len;
    size_t i;
    while(alpha + 1 < beta) {
        i = (alpha + beta) >> 1;
        if(db->sortbuf[i].max > addr && db->sortbuf[i - 1].max < addr) {
            break;
        }

        if(db->sortbuf[i].max > addr) {
            beta = i;
        } else {
            alpha = i;
        }
    }
    
    countrycode_t cc = db->sortbuf[i].cc;
    *loc = db->avg[cc.code[0]][cc.code[1]];
    return true;
}
