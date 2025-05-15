// B发送-A加密-A接收密文
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stddef.h>
#include <linux/if_packet.h>
#include <linux/icmp.h>
#include <bpf/bpf_endian.h>
#include <linux/tcp.h>
#include <linux/ptrace.h>
#include <linux/bpf_common.h>
#include <string.h>
#define u8 unsigned char
#define u32 unsigned int
#define u16 unsigned short

#define SM4_BLOCK_SIZE 16
// S盒（放在只读段）
static const u8 Sbox[256] __attribute__((section(".rodata"))) = {
    0xd6, 0x90, 0xe9, 0xfe, 0xcc, 0xe1, 0x3d, 0xb7, 0x16, 0xb6, 0x14, 0xc2, 0x28, 0xfb, 0x2c, 0x05,
    0x2b, 0x67, 0x9a, 0x76, 0x2a, 0xbe, 0x04, 0xc3, 0xaa, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
    0x9c, 0x42, 0x50, 0xf4, 0x91, 0xef, 0x98, 0x7a, 0x33, 0x54, 0x0b, 0x43, 0xed, 0xcf, 0xac, 0x62,
    0xe4, 0xb3, 0x1c, 0xa9, 0xc9, 0x08, 0xe8, 0x95, 0x80, 0xdf, 0x94, 0xfa, 0x75, 0x8f, 0x3f, 0xa6,
    0x47, 0x07, 0xa7, 0xfc, 0xf3, 0x73, 0x17, 0xba, 0x83, 0x59, 0x3c, 0x19, 0xe6, 0x85, 0x4f, 0xa8,
    0x68, 0x6b, 0x81, 0xb2, 0x71, 0x64, 0xda, 0x8b, 0xf8, 0xeb, 0x0f, 0x4b, 0x70, 0x56, 0x9d, 0x35,
    0x1e, 0x24, 0x0e, 0x5e, 0x63, 0x58, 0xd1, 0xa2, 0x25, 0x22, 0x7c, 0x3b, 0x01, 0x21, 0x78, 0x87,
    0xd4, 0x00, 0x46, 0x57, 0x9f, 0xd3, 0x27, 0x52, 0x4c, 0x36, 0x02, 0xe7, 0xa0, 0xc4, 0xc8, 0x9e,
    0xea, 0xbf, 0x8a, 0xd2, 0x40, 0xc7, 0x38, 0xb5, 0xa3, 0xf7, 0xf2, 0xce, 0xf9, 0x61, 0x15, 0xa1,
    0xe0, 0xae, 0x5d, 0xa4, 0x9b, 0x34, 0x1a, 0x55, 0xad, 0x93, 0x32, 0x30, 0xf5, 0x8c, 0xb1, 0xe3,
    0x1d, 0xf6, 0xe2, 0x2e, 0x82, 0x66, 0xca, 0x60, 0xc0, 0x29, 0x23, 0xab, 0x0d, 0x53, 0x4e, 0x6f,
    0xd5, 0xdb, 0x37, 0x45, 0xde, 0xfd, 0x8e, 0x2f, 0x03, 0xff, 0x6a, 0x72, 0x6d, 0x6c, 0x5b, 0x51,
    0x8d, 0x1b, 0xaf, 0x92, 0xbb, 0xdd, 0xbc, 0x7f, 0x11, 0xd9, 0x5c, 0x41, 0x1f, 0x10, 0x5a, 0xd8,
    0x0a, 0xc1, 0x31, 0x88, 0xa5, 0xcd, 0x7b, 0xbd, 0x2d, 0x74, 0xd0, 0x12, 0xb8, 0xe5, 0xb4, 0xb0,
    0x89, 0x69, 0x97, 0x4a, 0x0c, 0x96, 0x77, 0x7e, 0x65, 0xb9, 0xf1, 0x09, 0xc5, 0x6e, 0xc6, 0x84,
    0x18, 0xf0, 0x7d, 0xec, 0x3a, 0xdc, 0x4d, 0x20, 0x79, 0xee, 0x5f, 0x3e, 0xd7, 0xcb, 0x39, 0x48};

// 密钥扩展常数
static const u32 FK[4] __attribute__((section(".rodata"))) = {
    0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc};

// 固定参数 CK
static const u32 CK[32] __attribute__((section(".rodata"))) = {
    0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
    0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
    0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
    0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
    0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
    0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
    0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
    0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279};

static __always_inline u32 loopLeft(u32 a, short length)
{
    return (a << length) | (a >> (32 - length));
}

static __always_inline u32 functionB(u32 b)
{
    u8 a0 = (u8)(b >> 24);
    u8 a1 = (u8)(b >> 16);
    u8 a2 = (u8)(b >> 8);
    u8 a3 = (u8)(b);
    return ((u32)Sbox[a0] << 24) | ((u32)Sbox[a1] << 16) | ((u32)Sbox[a2] << 8) | Sbox[a3];
}

static __always_inline u32 functionL1(u32 a)
{
    return a ^ loopLeft(a, 2) ^ loopLeft(a, 10) ^ loopLeft(a, 18) ^ loopLeft(a, 24);
}

static __always_inline u32 functionL2(u32 a)
{
    return a ^ loopLeft(a, 13) ^ loopLeft(a, 23);
}

static __always_inline u32 functionT(u32 a, short mode)
{
    return mode == 1 ? functionL1(functionB(a)) : functionL2(functionB(a));
}

static __always_inline void extendFirst(u32 MK[4], u32 K[4])
{
#pragma unroll
    for (int i = 0; i < 4; i++)
    {
        K[i] = MK[i] ^ FK[i];
    }
}

static __always_inline void extendSecond(u32 RK[32], u32 K[4])
{
#pragma unroll
    for (int i = 0; i < 32; i++)
    {
        u32 temp = K[i % 4] ^ functionT(K[(i + 1) % 4] ^ K[(i + 2) % 4] ^ K[(i + 3) % 4] ^ CK[i], 2);
        K[(i + 4) % 4] = temp;
        RK[i] = temp;
    }
}

static __always_inline void getRK(u32 MK[4], u32 K[4], u32 RK[32])
{
    extendFirst(MK, K);
    extendSecond(RK, K);
}

static __always_inline void iterate32(u32 X[8], u32 RK[32])
{
#pragma unroll
    for (int i = 0; i < 32; i++)
    {
        u32 temp = X[i % 4] ^ functionT(X[(i + 1) % 4] ^ X[(i + 2) % 4] ^ X[(i + 3) % 4] ^ RK[i], 1);
        X[(i + 4) % 4] = temp;
    }
}

static __always_inline void reverse(u32 X[8], u32 Y[4])
{
#pragma unroll
    for (int i = 0; i < 4; i++)
    {
        Y[i] = X[3 - i];
    }
}

static __always_inline void encryptSM4(u32 X[4], u32 RK[32], u32 Y[4])
{
    iterate32(X, RK);
    reverse(X, Y);
}

static __always_inline u16 ip_checksum(struct iphdr *ip)
{
    u32 sum = 0;
    u16 *ptr = (u16 *)ip;
    ip->check = 0; // 必须先将校验和字段清零

#pragma unroll
    for (int i = 0; i < sizeof(struct iphdr) / 2; i++)
    {
        sum += ptr[i];
    }

    // 处理进位
    sum = (sum & 0xffff) + (sum >> 16);
    sum += (sum >> 16);
    return ~sum;
}

static __always_inline u16 udp_checksum(struct iphdr *ip, struct udphdr *udp, void *data_end)
{
    if ((void *)(ip + 1) > data_end || (void *)(udp + 1) > data_end)
        return 0;

    udp->check = 0;

    u16 udp_len_be = udp->len;
    u16 udp_len = bpf_ntohs(udp_len_be);

    if (udp_len < sizeof(*udp))
        return 0;

    u16 payload_len = udp_len - sizeof(*udp);
    u8 *payload_start = (u8 *)(udp + 1);
    if ((void *)(payload_start + payload_len) > data_end)
        return 0;

    u32 csum = 0;

    csum += (ip->saddr >> 16) & 0xffff;
    csum += ip->saddr & 0xffff;
    csum += (ip->daddr >> 16) & 0xffff;
    csum += ip->daddr & 0xffff;
    csum += (u16)ip->protocol << 8;

    // UDP长度
    csum += udp_len_be;

    csum += udp->source;
    csum += udp->dest;
    csum += udp_len_be;

    u16 count = 0;
    u32 offset = 0;

#pragma unroll 4
    for (int i = 0; i < 64 && offset < payload_len - 1; i += 2)
    {
        void *pos = (void *)(payload_start + offset);
        if (pos + 2 > data_end)
            break;

        u16 v = *(u16 *)pos;
        csum += v;
        offset += 2;
        count += 2;
    }

    if (count < payload_len)
    {
        void *pos = (void *)(payload_start + count);
        if (pos + 1 <= data_end)
        {
            u8 v = *(u8 *)pos;
            csum += (u16)v << 8;
        }
    }

    while (csum >> 16)
        csum = (csum & 0xffff) + (csum >> 16);

    return ~csum;
}

SEC("xdp")
int xdp_sm4_encrypt(struct xdp_md *ctx)
{
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS

                   struct iphdr *
                   ip = (void *)eth + sizeof(*eth);
    if ((void *)(ip + 1) > data_end)
        return XDP_PASS;
    if (ip->protocol != IPPROTO_UDP)
        return XDP_PASS;

    struct udphdr *udp = (void *)ip + (ip->ihl * 4);
    if ((void *)(udp + 1) > data_end)
        return XDP_PASS;

    u8 *payload = (u8 *)udp + sizeof(*udp);
    u32 payload_len = bpf_ntohs(udp->len) - sizeof(*udp);
    // bpf_printk("payload_length: %d", payload_len);
    if (ip->saddr != bpf_htonl(0xc0a83a81) || udp->source != bpf_htons(1235))
    {
        // bpf_printk("Dropped packet from %x:%d", ip->saddr, bpf_ntohs(udp->source));
        return XDP_PASS;
    }

    int pad = (16 - (payload_len % 16)) % 16;
    if (pad)
    {
        if (bpf_xdp_adjust_tail(ctx, pad))
            return XDP_PASS;
        // 调整后需要重新获取 data_end 和 udp、payload
        data = (void *)(unsigned long)ctx->data;
        data_end = (void *)(unsigned long)ctx->data_end;
        eth = data;
        if ((void *)(eth + 1) > data_end)
            return XDP_PASS;
        ip = (void *)eth + sizeof(*eth);
        if ((void *)(ip + 1) > data_end)
            return XDP_PASS;
        udp = (void *)ip + (ip->ihl * 4);
        if ((void *)(udp + 1) > data_end)
            return XDP_PASS;
        payload = (u8 *)udp + sizeof(*udp);
        payload_len = payload_len + pad;
        udp->len = bpf_htons(sizeof(*udp) + payload_len);
        ip->tot_len = bpf_htons((ip->ihl * 4) + sizeof(*udp) + payload_len);
    }

    // bpf_printk("payload_length: %d", payload_len);
    if (udp->dest != bpf_htons(1233))
    {
        return XDP_DROP;
    }

    u32 MK[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    u32 K[4] = {0};
    u32 RK[32] = {0};
    getRK(MK, K, RK);

    int block_num = payload_len / 16;

#pragma unroll
    for (int i = 0; i < 100; i++)
    {
        if (i >= block_num)
            break;

        u8 block_buf[16] = {0};
        if ((void *)(payload + i * 16 + 16) > data_end)
            break;
        __builtin_memcpy(block_buf, payload + i * 16, 16);
        u32 block[4] = {0};

#pragma unroll
        for (int j = 0; j < 4; j++)
        {
            __builtin_memcpy(&block[j], block_buf + j * 4, 4);
            block[j] = bpf_ntohl(block[j]);
        }

        u32 encrypted[4] = {0};
        encryptSM4(block, RK, encrypted);
        // bpf_printk("encrypted: block[%d]: 0x%02x", encrypted);

#pragma unroll
        for (int j = 0; j < 4; j++)
        {
            u32 tmp = bpf_htonl(encrypted[j]);
            __builtin_memcpy(payload + i * 16 + j * 4, &tmp, 4);
        }
    }
    // 重新计算校验和

    ip->check = 0;
    ip->check = ip_checksum(ip);
    udp->check = 0;
    udp->check = udp_checksum(ip, udp, data_end);

    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
