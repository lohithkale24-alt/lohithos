#include "net.h"
#include "e1000.h"
#include "timer.h"
#include "heap.h"

net_config_t g_net;

static uint16_t ip_checksum(const void *vdata, size_t length) {
    const uint8_t *data = (const uint8_t *)vdata;
    uint32_t acc = 0;
    for (size_t i = 0; i + 1 < length; i += 2) {
        uint16_t word;
        memcpy(&word, data + i, 2);
        acc += word;
    }
    if (length & 1) {
        acc += data[length - 1];
    }
    while (acc >> 16) {
        acc = (acc & 0xFFFF) + (acc >> 16);
    }
    return (uint16_t)~acc;
}

void ip_to_str(uint32_t ip, char *buf) {
    uint8_t *b = (uint8_t *)&ip;
    sprintf(buf, "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
}

uint32_t str_to_ip(const char *str) {
    int parts[4] = {0, 0, 0, 0};
    int idx = 0;
    while (*str && idx < 4) {
        if (*str == '.') {
            idx++;
        } else if (*str >= '0' && *str <= '9') {
            parts[idx] = parts[idx] * 10 + (*str - '0');
        }
        str++;
    }
    return IP4(parts[0], parts[1], parts[2], parts[3]);
}

void net_init(void) {
    memset(&g_net, 0, sizeof(g_net));

    /* VirtualBox NAT default network parameters */
    g_net.my_ip       = IP4(10, 0, 2, 15);
    g_net.gateway_ip  = IP4(10, 0, 2, 2);
    g_net.subnet_mask = IP4(255, 255, 255, 0);
    g_net.dns_ip      = IP4(10, 0, 2, 3);

    /* Default VirtualBox NAT Gateway MAC (52:54:00:12:35:02) */
    g_net.gateway_mac[0] = 0x52;
    g_net.gateway_mac[1] = 0x54;
    g_net.gateway_mac[2] = 0x00;
    g_net.gateway_mac[3] = 0x12;
    g_net.gateway_mac[4] = 0x35;
    g_net.gateway_mac[5] = 0x02;
    g_net.has_gateway_mac = true;
    g_net.dhcp_configured = true;

    /* VirtualBox passes host Wi-Fi connectivity directly through NAT */
    g_net.is_wifi_bridge = true;
    strcpy(g_net.wifi_ssid, "Host-WiFi (Bridged)");

    g_net.ping_seq = 1;
    g_net.last_ping_replied = false;
    g_net.last_ping_time_ms = 0;
}

void net_send_arp_request(uint32_t target_ip) {
    uint8_t packet[sizeof(struct ethernet_header) + sizeof(struct arp_packet)];
    struct ethernet_header *eth = (struct ethernet_header *)packet;
    struct arp_packet *arp = (struct arp_packet *)(packet + sizeof(struct ethernet_header));

    memset(eth->dest_mac, 0xFF, 6); /* Broadcast */
    memcpy(eth->src_mac, g_nic.mac, 6);
    eth->ethertype = htons(ETHERTYPE_ARP);

    arp->hw_type = htons(1);       /* Ethernet */
    arp->proto_type = htons(ETHERTYPE_IPV4);
    arp->hw_len = 6;
    arp->proto_len = 4;
    arp->opcode = htons(1);        /* ARP Request */
    memcpy(arp->src_mac, g_nic.mac, 6);
    arp->src_ip = g_net.my_ip;
    memset(arp->target_mac, 0x00, 6);
    arp->target_ip = target_ip;

    e1000_send_packet(packet, sizeof(packet));
}

void net_send_ipv4(uint32_t dest_ip, uint8_t proto, const void *payload, uint16_t len) {
    uint16_t total_len = sizeof(struct ethernet_header) + sizeof(struct ipv4_header) + len;
    uint8_t *packet = (uint8_t *)kmalloc(total_len);
    if (!packet) return;

    struct ethernet_header *eth = (struct ethernet_header *)packet;
    struct ipv4_header *ip = (struct ipv4_header *)(packet + sizeof(struct ethernet_header));
    uint8_t *data = packet + sizeof(struct ethernet_header) + sizeof(struct ipv4_header);

    /* Use gateway MAC for non-local or broadcast */
    memcpy(eth->dest_mac, g_net.gateway_mac, 6);
    memcpy(eth->src_mac, g_nic.mac, 6);
    eth->ethertype = htons(ETHERTYPE_IPV4);

    ip->version_ihl = (4 << 4) | 5; /* IPv4, 5 DWORDs (20 bytes) */
    ip->tos = 0;
    ip->total_length = htons(sizeof(struct ipv4_header) + len);
    ip->id = htons(1);
    ip->flags_fragment = htons(0x4000); /* Don't fragment */
    ip->ttl = 64;
    ip->protocol = proto;
    ip->checksum = 0;
    ip->src_ip = g_net.my_ip;
    ip->dest_ip = dest_ip;
    ip->checksum = ip_checksum(ip, sizeof(struct ipv4_header));

    memcpy(data, payload, len);

    e1000_send_packet(packet, total_len);
    kfree(packet);
}

void net_send_ping(uint32_t dest_ip) {
    struct icmp_header icmp;
    icmp.type = ICMP_TYPE_ECHO_REQUEST;
    icmp.code = 0;
    icmp.checksum = 0;
    icmp.id = htons(0x1337);
    icmp.sequence = htons(g_net.ping_seq++);

    icmp.checksum = ip_checksum(&icmp, sizeof(struct icmp_header));
    g_net.last_ping_replied = false;
    g_net.last_ping_time_ms = timer_get_ticks() * 10;

    net_send_ipv4(dest_ip, IP_PROTO_ICMP, &icmp, sizeof(struct icmp_header));
}

void net_poll(void) {
    uint8_t buffer[2048];
    int len = e1000_receive_packet(buffer, sizeof(buffer));
    if (len < (int)sizeof(struct ethernet_header)) return;

    struct ethernet_header *eth = (struct ethernet_header *)buffer;
    uint16_t ethertype = ntohs(eth->ethertype);

    if (ethertype == ETHERTYPE_ARP) {
        struct arp_packet *arp = (struct arp_packet *)(buffer + sizeof(struct ethernet_header));
        if (ntohs(arp->opcode) == 1 && arp->target_ip == g_net.my_ip) {
            /* Respond with ARP reply */
            uint8_t reply[sizeof(struct ethernet_header) + sizeof(struct arp_packet)];
            struct ethernet_header *r_eth = (struct ethernet_header *)reply;
            struct arp_packet *r_arp = (struct arp_packet *)(reply + sizeof(struct ethernet_header));

            memcpy(r_eth->dest_mac, eth->src_mac, 6);
            memcpy(r_eth->src_mac, g_nic.mac, 6);
            r_eth->ethertype = htons(ETHERTYPE_ARP);

            r_arp->hw_type = htons(1);
            r_arp->proto_type = htons(ETHERTYPE_IPV4);
            r_arp->hw_len = 6;
            r_arp->proto_len = 4;
            r_arp->opcode = htons(2); /* ARP Reply */
            memcpy(r_arp->src_mac, g_nic.mac, 6);
            r_arp->src_ip = g_net.my_ip;
            memcpy(r_arp->target_mac, eth->src_mac, 6);
            r_arp->target_ip = arp->src_ip;

            e1000_send_packet(reply, sizeof(reply));
        } else if (ntohs(arp->opcode) == 2) {
            /* Cache gateway MAC */
            if (arp->src_ip == g_net.gateway_ip) {
                memcpy(g_net.gateway_mac, arp->src_mac, 6);
                g_net.has_gateway_mac = true;
            }
        }
    } else if (ethertype == ETHERTYPE_IPV4) {
        struct ipv4_header *ip = (struct ipv4_header *)(buffer + sizeof(struct ethernet_header));
        if (ip->protocol == IP_PROTO_ICMP) {
            uint8_t *payload = buffer + sizeof(struct ethernet_header) + ((ip->version_ihl & 0x0F) * 4);
            struct icmp_header *icmp = (struct icmp_header *)payload;

            if (icmp->type == ICMP_TYPE_ECHO_REPLY) {
                g_net.last_ping_replied = true;
                g_net.last_ping_time_ms = (timer_get_ticks() * 10) - g_net.last_ping_time_ms;
            } else if (icmp->type == ICMP_TYPE_ECHO_REQUEST) {
                /* Echo back */
                icmp->type = ICMP_TYPE_ECHO_REPLY;
                icmp->checksum = 0;
                icmp->checksum = ip_checksum(icmp, ntohs(ip->total_length) - ((ip->version_ihl & 0x0F) * 4));
                net_send_ipv4(ip->src_ip, IP_PROTO_ICMP, icmp, ntohs(ip->total_length) - ((ip->version_ihl & 0x0F) * 4));
            }
        }
    }
}
