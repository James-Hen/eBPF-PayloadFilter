#include <arpa/inet.h>
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#define A_PORT 1145
#define B_PORT 5141

int xdp_func(struct xdp_md *ctx) {
    /*
        In this ePBF function we drop a TCP packet
        "Are you OK?" from port A_PORT to port B_PORT
    */
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;
    char match_pattern[] = "Are you OK?";
    unsigned int payload_size, i;
    struct ethhdr *eth = data;
    unsigned char *payload;
    struct tcphdr *tcp;
    struct iphdr *ip;

    if ((void *)eth + sizeof(*eth) > data_end)
        return XDP_PASS;

    ip = data + sizeof(*eth);
    if ((void *)ip + sizeof(*ip) > data_end)
        return XDP_PASS;

    if (ip->protocol != IPPROTO_TCP)
        return XDP_PASS;

    tcp = (void *)ip + sizeof(*ip);
    if ((void *)tcp + sizeof(*tcp) > data_end)
        return XDP_PASS;

    if (tcp->source != ntohs(A_PORT) && tcp->dest != ntohs(B_PORT))
        return XDP_PASS;

    payload_size = ntohs(tcp->len) - sizeof(*tcp);
    // Here we use "size - 1" to account for the final '\0' in "test".
    // This '\0' may or may not be in your payload, adjust if necessary.
    if (payload_size != sizeof(match_pattern) - 1) 
        return XDP_PASS;

    // Point to start of payload.
    payload = (unsigned char *)tcp + sizeof(*tcp);
    if ((void *)payload + payload_size > data_end)
        return XDP_PASS;

    // Compare each byte, exit if a difference is found.
    for (i = 0; i < payload_size; i++)
        if (payload[i] != match_pattern[i])
            return XDP_PASS;

    // Same payload, drop.
    return XDP_DROP;
}
