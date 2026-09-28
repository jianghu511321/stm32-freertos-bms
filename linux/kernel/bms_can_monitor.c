#include <linux/can.h>
#include <linux/can/core.h>
#include <linux/can/skb.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/net.h>
#include <linux/seq_file.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <net/net_namespace.h>

#define BMS_CAN_BASE_ID 0x300U
#define BMS_CAN_ID_COUNT 8U
#define BMS_RING_CAPACITY 128U

struct bms_record {
    u64 timestamp_ns;
    canid_t can_id;
    u8 len;
    u8 data[CAN_MAX_DLEN];
};

static struct bms_record ring[BMS_RING_CAPACITY];
static unsigned int ring_head;
static unsigned int ring_count;
static DEFINE_SPINLOCK(ring_lock);
static atomic64_t received_frames;
static atomic64_t overwritten_frames;
static struct dentry *debugfs_root;

static void bms_can_receive(struct sk_buff *skb, void *data)
{
    const struct can_frame *frame;
    unsigned long flags;
    struct bms_record *record;

    (void)data;
    if (!can_is_can_skb(skb) || skb->len < CAN_MTU) {
        return;
    }
    frame = (const struct can_frame *)skb->data;

    spin_lock_irqsave(&ring_lock, flags);
    record = &ring[ring_head];
    record->timestamp_ns = ktime_get_ns();
    record->can_id = frame->can_id;
    record->len = frame->len;
    memcpy(record->data, frame->data, CAN_MAX_DLEN);
    ring_head = (ring_head + 1U) % BMS_RING_CAPACITY;
    if (ring_count < BMS_RING_CAPACITY) {
        ++ring_count;
    } else {
        atomic64_inc(&overwritten_frames);
    }
    spin_unlock_irqrestore(&ring_lock, flags);
    atomic64_inc(&received_frames);
}

static int frames_show(struct seq_file *m, void *unused)
{
    struct bms_record *snapshot;
    unsigned long flags;
    unsigned int count;
    unsigned int oldest;
    unsigned int i;

    (void)unused;
    snapshot = kcalloc(BMS_RING_CAPACITY, sizeof(*snapshot), GFP_KERNEL);
    if (!snapshot) {
        return -ENOMEM;
    }

    spin_lock_irqsave(&ring_lock, flags);
    count = ring_count;
    oldest = (ring_head + BMS_RING_CAPACITY - ring_count) % BMS_RING_CAPACITY;
    for (i = 0U; i < count; ++i) {
        snapshot[i] = ring[(oldest + i) % BMS_RING_CAPACITY];
    }
    spin_unlock_irqrestore(&ring_lock, flags);

    for (i = 0U; i < count; ++i) {
        unsigned int byte;
        seq_printf(m, "%llu %03X [%u]",
                   (unsigned long long)snapshot[i].timestamp_ns,
                   snapshot[i].can_id & CAN_SFF_MASK,
                   (unsigned int)snapshot[i].len);
        for (byte = 0U; byte < snapshot[i].len; ++byte) {
            seq_printf(m, " %02X", snapshot[i].data[byte]);
        }
        seq_putc(m, '\n');
    }
    kfree(snapshot);
    return 0;
}
DEFINE_SHOW_ATTRIBUTE(frames);

static int stats_show(struct seq_file *m, void *unused)
{
    unsigned long flags;
    unsigned int buffered;

    (void)unused;
    spin_lock_irqsave(&ring_lock, flags);
    buffered = ring_count;
    spin_unlock_irqrestore(&ring_lock, flags);

    seq_printf(m, "received=%lld\n", atomic64_read(&received_frames));
    seq_printf(m, "overwritten=%lld\n", atomic64_read(&overwritten_frames));
    seq_printf(m, "buffered=%u\n", buffered);
    seq_printf(m, "capacity=%u\n", BMS_RING_CAPACITY);
    return 0;
}
DEFINE_SHOW_ATTRIBUTE(stats);

static int __init bms_can_monitor_init(void)
{
    unsigned int i;
    int error;

    atomic64_set(&received_frames, 0);
    atomic64_set(&overwritten_frames, 0);
    debugfs_root = debugfs_create_dir("bms_can_monitor", NULL);
    if (IS_ERR_OR_NULL(debugfs_root)) {
        return debugfs_root ? PTR_ERR(debugfs_root) : -ENOMEM;
    }
    debugfs_create_file("frames", 0444, debugfs_root, NULL, &frames_fops);
    debugfs_create_file("stats", 0444, debugfs_root, NULL, &stats_fops);

    for (i = 0U; i < BMS_CAN_ID_COUNT; ++i) {
        error = can_rx_register(&init_net, NULL, BMS_CAN_BASE_ID + i,
                                CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG,
                                bms_can_receive, NULL, "bms_can_monitor", NULL);
        if (error) {
            while (i > 0U) {
                --i;
                can_rx_unregister(&init_net, NULL, BMS_CAN_BASE_ID + i,
                                  CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG,
                                  bms_can_receive, NULL);
            }
            debugfs_remove_recursive(debugfs_root);
            return error;
        }
    }
    pr_info("bms_can_monitor: monitoring CAN IDs 0x300-0x307\n");
    return 0;
}

static void __exit bms_can_monitor_exit(void)
{
    unsigned int i;
    for (i = 0U; i < BMS_CAN_ID_COUNT; ++i) {
        can_rx_unregister(&init_net, NULL, BMS_CAN_BASE_ID + i,
                          CAN_SFF_MASK | CAN_EFF_FLAG | CAN_RTR_FLAG,
                          bms_can_receive, NULL);
    }
    debugfs_remove_recursive(debugfs_root);
    pr_info("bms_can_monitor: unloaded\n");
}

module_init(bms_can_monitor_init);
module_exit(bms_can_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BMS FreeRTOS Demo contributors");
MODULE_DESCRIPTION("Buffers STM32 BMS SocketCAN telemetry in debugfs");
