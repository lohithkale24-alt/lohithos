#ifndef _NET_H_
#define _NET_H_

#include "types.h"

#define ETHERTYPE_ARP  0x0806
#define ETHERTYPE_IPV4 0x0800

#define IP_PROTO_ICMP 0x01
#define IP_PROTO_TCP  0x06
#define IP_PROTO_UDP  0x11

#define ICMP_TYPE_ECHO_REPLY   0x00
#define ICMP_TYPE_ECHO_REQUEST 0x08

/* Big-endian / Little-endian helpers */
#define htons(n) (((((uint16_t)(n)) & 0xFF)) << 8 | ((((uint16_t)(n)) & 0xFF00) >> 8))
#define ntohs(n) htons(n)
#define htonl(n) (((((uint32_t)(n)) & 0x000000FF)) << 24 | \
                  ((((uint32_t)(n)) & 0x0000FF00)) << 8  | \
                  ((((uint32_t)(n)) & 0x00FF0000)) >> 8  | \
                  ((((uint32_t)(n)) & 0xFF000000)) >> 24)
#define ntohl(n) htonl(n)

#define IP4(a,b,c,d) (((uint32_t)(a)) | (((uint32_t)(b))<<8) | (((uint32_t)(c))<<16) | (((uint32_t)(d))<<24))

struct ethernet_header {
    uint8_t  dest_mac[6];
    uint8_t  src_mac[6];
    uint16_t ethertype;
} __attribute__((packed));

struct arp_packet {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t  hw_len;
    uint8_t  proto_len;
    uint16_t opcode;
    uint8_t  src_mac[6];
    uint32_t src_ip;
    uint8_t  target_mac[6];
    uint32_t target_ip;
} __attribute__((packed));

struct ipv4_header {
    uint8_t  version_ihl;
    uint8_t  tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed));

struct icmp_header {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed));

struct udp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed));

typedef struct {
    uint32_t my_ip;
    uint32_t gateway_ip;
    uint32_t subnet_mask;
    uint32_t dns_ip;
    uint8_t  gateway_mac[6];
    bool     has_gateway_mac;
    bool     dhcp_configured;
    bool     is_wifi_bridge;
    char     wifi_ssid[32];
    int      ping_seq;
    bool     last_ping_replied;
    uint32_t last_ping_time_ms;
} net_config_t;

extern net_config_t g_net;

void net_init(void);
void net_poll(void);
void net_send_arp_request(uint32_t target_ip);
void net_send_ipv4(uint32_t dest_ip, uint8_t proto, const void *payload, uint16_t len);
void net_send_ping(uint32_t dest_ip);
void net_dhcp_discover(void);
void net_dns_query(const char *domain);
void ip_to_str(uint32_t ip, char *buf);
uint32_t str_to_ip(const char *str);

#endif
