
/* map used to count packets; key is Ip protocol, value is pkt count */
// 定义一个名为rxcnt的bpf_map_def结构体，并使用SEC("maps")进行标记
struct bpf_map_def SEC("maps") rxcnt = {  
    // 类型为BPF_MAP_TYPE_PERCPU_ARRAY，即每个CPU都有一个数组
    type = BPF_MAP_TYPE_PERCPU_ARRAY,
    // 键的大小为u32类型
    key_size = sizeof(u32),
    // 值的大小为long类型
    value_size = sizeof(long),
    // 最大条目数为256
    max_entries = 256
};

/* swaps MAC addresses using direct packet data access */
// 静态函数，用于交换源MAC地址和目标MAC地址
static void swap_src_dst_mac(void *data) {
    // 将data转换为unsigned short类型的指针
    unsigned short *p = data;
    unsigned short dst[3];
    dst[0] = p[0];
    dst[1] = p[1];
    dst[2] = p[2];
    p[0] = p[3];
    p[1] = p[4];
    p[2] = p[5];
    p[3] = dst[0];
    p[4] = dst[1];
    p[5] = dst[2];
}

// 静态函数，用于解析IPv4数据包
static int parse_ipv4(void *data, u64 nh_off, void *data_end) {
    // 将data指针加上nh_off偏移量，得到iph指针，指向IP头部
    struct iphdr *iph = data + nh_off;
    // 如果iph指针加上1大于data_end指针，说明数据包不完整，返回0
    if (iph + 1 > data_end) return 0;
    // 返回IP头部中的协议字段
    return iph->protocol;
}

SEC("xdp1") /* marks main eBPF program entry point */
int xdp_progl(struct xdp_md *ctx) {
    // 获取数据包的结束位置
    void *data_end = (void *)(long)ctx->data_end;
    // 获取数据包的起始位置
    void *data = (void *)(long)ctx->data;
    // 获取以太网头部
    struct ethhdr *eth = data;
    // 初始化返回值为XDP_DROP，表示丢弃数据包
    int rc = XDP_DROP;
    // 定义一个长整型指针
    long *value;
    // 定义一个16位无符号整数，用于存储协议类型
    u16 h_proto;
    // 定义一个64位无符号整数，用于存储下一个协议头的偏移量
    u64 nh_off;
    // 定义一个32位无符号整数，用于存储IP协议类型
    u32 ipproto;

    // 获取数据包的结束地址
    nh_off = sizeof(*eth);
    // 检查数据包是否合法
    if (data + nh_off > data_end) return rc;
    // 获取以太网协议类型
    h_proto = eth->h_proto;

    /* check VLAN tag: could be repeated to support double-tagged VLAN */
    // 如果协议类型是802.1Q或802.1AD
    if (h_proto == htons(ETH_P_8021Q) || h_proto == htons(ETH_P_8021AD)) {
        // 定义一个vlan_hdr结构体指针vhdr
        struct vlan_hdr *vhdr;
        // 将vhdr指向data+nh_off的位置
        vhdr = data + nh_off;
        // nh_off增加vlan_hdr结构体的大小
        nh_off += sizeof(struct vlan_hdr);
        // 如果data+nh_off大于data_end，返回rc
        if (data + nh_off > data_end) return rc;
        // h_proto等于vhdr中的h_vlan_encapsulated_proto
        h_proto = vhdr->h_vlan_encapsulated_proto;
    }

    // 如果以太网协议类型为IP协议
    if (h_proto == htons(ETH_P_IP))
        // 解析IPv4协议
        ipproto = parse_ipv4(data, nh_off, data_end);
    // 如果以太网协议类型为IPv6协议
    else if (h_proto == htons(ETH_P_IPV6))
        // 解析IPv6协议
        ipproto = parse_ipv6(data, nh_off, data_end);
    // 否则
    else
        // 协议类型为0
        ipproto = 0;

    /* lookup map element for ip protocol, used for packet counter */
    // 查找ip协议的映射元素，用于数据包计数
    value = bpf_map_lookup_elem(&rxcnt, &ipproto);
    if (value) *value += 1;

    /* swap MAC addrs for UDP packets, transmit out this interface */
    // 如果是UDP数据包，交换源和目的MAC地址，从该接口发送
    if (ipproto == IPPROTO_UDP) {
        swap_src_dst_mac(data);
        rc = XDP_TX;
    }
    return rc;
}