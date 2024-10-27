#include "ipgeo.h"
#include <libmnl/libmnl.h>
#include <linux/inet_diag.h>
#include <linux/netlink.h>
#include <linux/netlink_diag.h>
#include <locale.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <ncurses.h>
#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

#include "term.h"

int mnl_msg_cb(struct nlmsghdr const *msg, void *data);
void fput_inaddr(FILE *fp, in_addr_t addr) {
    fprintf(fp, "%d.%d.%d.%d", (addr >> 0) & 0xff, (addr >> 8) & 0xff, (addr >> 16) & 0xff, (addr >> 24) & 0xff);
}
char const* tcp_state_str(uint8_t state);

struct state {
    mapchars_t *map;
    ipgeodb_t *db;
};

int main(int argc, char const *argv[]) {
    WINDOW *win = initscr();
    setlocale(LC_ALL, "");
    
    int termwidth, termheight;
    getmaxyx(win, termheight, termwidth);

    int min_extent = termwidth < termheight ? termwidth : termheight;
    printf("Min is %d\n", min_extent);
    struct state state;
    state.map = image_to_chbuf("../assets/mercator-projection.svg", min_extent);
    state.db = ipgeodb_open(
        "../asset/asn-country-ipv4-num.csv",
        "../asset/iso3166-1-cc.csv"
    );

    for(int y = 0; y < state.map->height; ++y) {
        for(int x = 0; x < state.map->width; ++x) {
            mvaddch(y, x, *(const chtype*)(state.map->data + y * state.map->width * 3 + x * 3));
        }
        addch('\n');
    }

    refresh();

    struct mnl_socket *sock = mnl_socket_open(NETLINK_SOCK_DIAG);
    if(sock == NULL) {
        perror("mnl_socket_open");
        return -1;
    }

    int rc = mnl_socket_bind(sock, IPPROTO_TCP, MNL_SOCKET_AUTOPID);
    if(rc == -1) {
        perror("mnl_socket_bind");
        return -1;
    }
    

    uint8_t buf[MNL_SOCKET_BUFFER_SIZE];
    for(;;) {
        ssize_t recvsz;
        if((recvsz = mnl_socket_recvfrom(sock, buf, MNL_SOCKET_BUFFER_SIZE)) == -1) {
            perror("mnl_socket_recvfrom");
            return -1;
        }
        
        struct nlmsghdr *msg = (struct nlmsghdr*)buf;
        rc = mnl_cb_run(
            msg,
            recvsz,
            msg->nlmsg_seq,
            msg->nlmsg_pid,
            mnl_msg_cb,
            &state
        );
        if(rc < 0) {
            perror("mnl_cb_run");
            return -1;
        }
    }
}


int mnl_msg_cb(struct nlmsghdr const *msg, void *data) {
    struct state *state = (struct state*)data;

    struct inet_diag_msg const *diag = mnl_nlmsg_get_payload(msg);
    size_t len = mnl_nlmsg_get_payload_len(msg);
    
    if(diag->idiag_state == TCP_ESTABLISHED) {
        latlong_t loc;
        uint32_t dest = ntohl(diag->id.idiag_dst[0]);
        if(ipgeodb_lookup(state->db, dest, &loc)) {
            float radius = (float)state->map->width / (2 * M_PI);
            int32_t x = (radius * (loc.lon * (M_PI / 180.f))) + ((float)state->map->width / 2.f);
            int32_t y = radius * logf(tanf(M_PI_4 + (loc.lat * (M_PI / 180.f)) / 2.f));
            
            curs_set(0);
            mvaddch(y, x, 'x');
        }
    }
    return 0;
}

char const* tcp_state_str(uint8_t state) {
    switch(state) {
        case TCP_ESTABLISHED: return "TCP_ESTABLISHED";
        case TCP_SYN_SENT: return "TCP_SYN_SENT";
        case TCP_SYN_RECV: return "TCP_SYN_RECV";
        case TCP_FIN_WAIT1: return "TCP_FIN_WAIT1";
        case TCP_FIN_WAIT2: return "TCP_FIN_WAIT2";
        case TCP_TIME_WAIT: return "TCP_TIME_WAIT";
        case TCP_CLOSE: return "TCP_CLOSE";
        case TCP_CLOSE_WAIT: return "TCP_CLOSE_WAIT";
        case TCP_LAST_ACK: return "TCP_LAST_ACK";
        case TCP_LISTEN: return "TCP_LISTEN";
        case TCP_CLOSING: return "TCP_CLOSING";
        //case TCP_NEW_SYN_RECV: return "TCP_NEW_SYN_RECV";
        //case TCP_BOUND_INACTIVE: return "TCP_BOUND_INACTIVE";
        default: return "TCP_UNKNOWN";
    }
}
