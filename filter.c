/* SPDX-License-Identifier: MIT */
/*
 * Copyright (c) 2022 Junyang Zhang
 */

// Uncomment this will enable printing debug messages to /sys/kernel/debug/tracing/trace_pipe
// #define DEBUG
// #define VERBOSE

#include <arpa/inet.h>
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <string.h>

char LICENSE[] SEC("license") = "Dual MIT/GPL";

#define printk(fmt, ...)                                        \
({                                                              \
        char ____fmt[] = fmt;                                   \
        bpf_trace_printk(____fmt, sizeof(____fmt),              \
                         ##__VA_ARGS__);                        \
})

#ifdef __FAVOR_BSD
    #define tcp_hdrl(hdr) ((uint32_t)(hdr->th_off) << 2)
#else
    #define tcp_hdrl(hdr) ((uint32_t)(hdr->doff) << 2)
#endif

// The pattern that will be matched
const char match_pattern[] = "Are you OK?";
// The ports of src and dst
#define A_PORT 1145
#define B_PORT 5141
// Packet drop counts, implement as eBPF map, just using map[0] as counter
#define DROP_COUNT 1
#define MAX_ENTRIES	16
struct bpf_map_def SEC("maps") drop_map = {
	.type = BPF_MAP_TYPE_HASH,
	.key_size = sizeof(uint32_t),
	.value_size = sizeof(uint32_t),
	.max_entries = MAX_ENTRIES,
};

/*
    In this ePBF function we drop all TCP packets with the payload
    "Are you OK?" from port A_PORT to port B_PORT

    This implementation is not suitable for patterns over 64 due to
    instn count limits
*/
SEC("prog")
int xdp_func(struct xdp_md *ctx) {

#ifdef DEBUG
#ifdef VERBOSE
    printk("Payload filter debugging enabled!\n");
#endif
#endif

    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;
    
    struct ethhdr *eth = data;
    const char *payload;
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

    // Match the source and target port
    if (tcp->source != htons(A_PORT) || tcp->dest != htons(B_PORT))
        return XDP_PASS;

#ifdef DEBUG
    printk("Protocol and port matched!\n");
#endif
    
    // Here we begin to check payloads
    uint32_t payload_size = // ip total length - ip header length - tcp data offset
        ntohs(ip->tot_len) - ((uint32_t)(ip->ihl) << 2) - tcp_hdrl(tcp);
    uint32_t pattern_size = sizeof(match_pattern) - 1;

#ifdef DEBUG
    printk("Matching payload size %d and %d\n", payload_size, pattern_size);
#endif
    // Here we use "size - 1" to account for the final '\0' in "test".
    // This '\0' may or may not be in your payload, adjust if necessary.
    if (payload_size != pattern_size) 
        return XDP_PASS;
    
#ifdef DEBUG
    printk("Payload size matched!\n");
#endif

    // Point to start of payload.
    payload = (const char *)tcp + tcp_hdrl(tcp);
    if ((void *)payload + payload_size > data_end)
        return XDP_PASS;

    // Compare first bytes, exit if a difference is found.
#pragma unroll
    for (int i = 0; i < payload_size; i++)
        if (match_pattern[i] != payload[i])
            return XDP_PASS;
    
#ifdef DEBUG
    printk("Payload matched! Packet may be dropped.\n");
#endif

    // Stateful: only drop DROP_COUNT times
    uint32_t key = 0, init_val = DROP_COUNT;
    // Update only if it doesn't exist
	bpf_map_update_elem(&drop_map, &key, &init_val, BPF_NOEXIST);
    uint32_t *val = bpf_map_lookup_elem(&drop_map, &key);
	if (val && *val) {
        int new_val = *val - 1;
        bpf_map_update_elem(&drop_map, &key, &new_val, BPF_ANY);
        return XDP_DROP;
    }
    
    // Match but had dropped, pass
    return XDP_PASS;
}
