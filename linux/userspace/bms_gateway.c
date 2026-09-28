#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <errno.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BMS_CAN_BASE_ID 0x300U
#define BMS_CAN_LAST_ID 0x307U
#define BMS_COMPLETE_MASK 0xFFU

struct telemetry {
    uint16_t sequence;
    uint16_t cells_mv[12];
    uint16_t pack_mv;
    int16_t current_ca;
    uint16_t faults;
    int16_t ntc_dc[4];
    uint16_t min_cell_mv;
    uint16_t max_cell_mv;
    uint32_t uptime_ms;
    uint8_t dropped;
    uint8_t state;
    uint8_t received_mask;
};

static volatile sig_atomic_t running = 1;

static uint16_t get_u16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void on_signal(int signo)
{
    (void)signo;
    running = 0;
}

static bool ingest(struct telemetry *t, const struct can_frame *frame)
{
    uint16_t sequence;
    uint32_t slot;
    uint32_t i;

    if ((frame->can_id & CAN_EFF_FLAG) != 0U || frame->len != 8U) {
        return false;
    }
    if (frame->can_id < BMS_CAN_BASE_ID || frame->can_id > BMS_CAN_LAST_ID) {
        return false;
    }

    sequence = get_u16(&frame->data[0]);
    if (t->received_mask != 0U && t->sequence != sequence) {
        memset(t, 0, sizeof(*t));
    }
    t->sequence = sequence;
    slot = frame->can_id - BMS_CAN_BASE_ID;
    t->received_mask |= (uint8_t)(1U << slot);

    if (slot < 4U) {
        const uint32_t first = slot * 3U;
        for (i = 0U; i < 3U; ++i) {
            t->cells_mv[first + i] = get_u16(&frame->data[2U + (2U * i)]);
        }
    } else if (frame->can_id == 0x304U) {
        t->pack_mv = get_u16(&frame->data[2]);
        t->current_ca = (int16_t)get_u16(&frame->data[4]);
        t->faults = get_u16(&frame->data[6]);
    } else if (frame->can_id == 0x305U) {
        t->ntc_dc[0] = (int16_t)get_u16(&frame->data[2]);
        t->ntc_dc[1] = (int16_t)get_u16(&frame->data[4]);
        t->ntc_dc[2] = (int16_t)get_u16(&frame->data[6]);
    } else if (frame->can_id == 0x306U) {
        t->ntc_dc[3] = (int16_t)get_u16(&frame->data[2]);
        t->min_cell_mv = get_u16(&frame->data[4]);
        t->max_cell_mv = get_u16(&frame->data[6]);
    } else {
        t->uptime_ms = get_u32(&frame->data[2]);
        t->dropped = frame->data[6];
        t->state = frame->data[7];
    }

    return t->received_mask == BMS_COMPLETE_MASK;
}

static void print_json(const struct telemetry *t)
{
    uint32_t i;
    printf("{\"sequence\":%u,\"uptime_ms\":%u,\"cells_mv\":[",
           (unsigned)t->sequence, t->uptime_ms);
    for (i = 0U; i < 12U; ++i) {
        printf("%s%u", (i == 0U) ? "" : ",", (unsigned)t->cells_mv[i]);
    }
    printf("],\"pack_mv\":%u,\"current_a\":%.2f,\"ntc_c\":[",
           (unsigned)t->pack_mv, (double)t->current_ca / 100.0);
    for (i = 0U; i < 4U; ++i) {
        printf("%s%.1f", (i == 0U) ? "" : ",", (double)t->ntc_dc[i] / 10.0);
    }
    printf("],\"min_cell_mv\":%u,\"max_cell_mv\":%u,"
           "\"faults\":%u,\"dropped\":%u,\"state\":%u}\n",
           (unsigned)t->min_cell_mv, (unsigned)t->max_cell_mv,
           (unsigned)t->faults, (unsigned)t->dropped, (unsigned)t->state);
    fflush(stdout);
}

static int open_can_socket(const char *interface)
{
    struct ifreq ifr;
    struct sockaddr_can address;
    struct can_filter filters[8];
    int fd;
    uint32_t i;

    fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    if (strlen(interface) >= sizeof(ifr.ifr_name)) {
        fprintf(stderr, "interface name is too long\n");
        close(fd);
        return -1;
    }
    (void)snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        close(fd);
        return -1;
    }

    for (i = 0U; i < 8U; ++i) {
        filters[i].can_id = BMS_CAN_BASE_ID + i;
        filters[i].can_mask = CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG;
    }
    if (setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER,
                   filters, sizeof(filters)) < 0) {
        perror("setsockopt(CAN_RAW_FILTER)");
        close(fd);
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.can_family = AF_CAN;
    address.can_ifindex = ifr.ifr_ifindex;
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }
    return fd;
}

int main(int argc, char **argv)
{
    const char *interface = (argc > 1) ? argv[1] : "vcan0";
    struct telemetry telemetry = {0};
    struct can_frame frame;
    int fd;

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    fd = open_can_socket(interface);
    if (fd < 0) {
        return EXIT_FAILURE;
    }

    fprintf(stderr, "bms-gateway listening on %s\n", interface);
    while (running) {
        const ssize_t bytes = read(fd, &frame, sizeof(frame));
        if (bytes < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("read");
            close(fd);
            return EXIT_FAILURE;
        }
        if ((size_t)bytes != sizeof(frame)) {
            continue;
        }
        if (ingest(&telemetry, &frame)) {
            print_json(&telemetry);
            telemetry.received_mask = 0U;
        }
    }

    close(fd);
    return EXIT_SUCCESS;
}
