#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <net/sock.h>

#define MODNAME           "nl_echo"
#define NL_MC_GRP         1                  // 多播组 id（自定义 1）

static struct sock *nl_sk;

static void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    int portid;
    char *payload;
    const char *reply = "pong from kernel";

    if (!skb)
        return;

    nlh = nlmsg_hdr(skb);
    portid = nlh->nlmsg_pid;        // 发送者的 portid（用户态是 getpid()）
    payload = (char *)nlmsg_data(nlh);

    pr_info(MODNAME ": recv from pid=%d: '%s'\n", portid, payload);

    // 回一个 echo（单播回应给发送进程）
    {
        struct sk_buff *skb_out;
        int msg_size = strlen(reply) + 1;
        int ret;

        skb_out = nlmsg_new(msg_size, GFP_KERNEL);
        if (!skb_out) {
            pr_err(MODNAME ": nlmsg_new failed\n");
            return;
        }

        nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
        if (!nlh) {
            pr_err(MODNAME ": nlmsg_put failed\n");
            kfree_skb(skb_out);
            return;
        }
        memcpy(nlmsg_data(nlh), reply, msg_size);

        ret = nlmsg_unicast(nl_sk, skb_out, portid);
        if (ret < 0)
            pr_err(MODNAME ": nlmsg_unicast err=%d\n", ret);
    }
}

static int __init nl_echo_init(void){
    struct netlink_kernel_cfg cfg = {
        /* call back */
        .input  = nl_recv_msg,
        /* 多播组 位图 */
        .groups = BIT(NL_MC_GRP),
    };

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);
    /* init_net 默认网络命名空间 
     * NETLINK_USERSOCK netlink 协议，linux netlink 最多支持 32 种协议 */

    if (!nl_sk) {
        pr_err(MODNAME ": netlink_kernel_create failed.\n");
        return -ENOMEM;
    }

    // 模块加载时，演示广播一条多播消息（订阅该组的用户态能收到）
    {
        const char *hello = "hello, multicast from kernel";
        struct sk_buff *skb;
        struct nlmsghdr *nlh;
        int msg_size = strlen(hello) + 1;

        skb = nlmsg_new(msg_size, GFP_KERNEL);
        if (skb) {
            nlh = nlmsg_put(skb, 0, 0, NLMSG_DONE, msg_size, 0);
            if (nlh) {
                memcpy(nlmsg_data(nlh), hello, msg_size);

                /* 内核发多播：端口 id=0 表示内核 */
                netlink_broadcast(nl_sk, skb, 0 /*portid*/, NL_MC_GRP, GFP_KERNEL);
            } else {
                kfree_skb(skb);
            }
        }
    }
    return 0;
}

static void __exit nl_echo_exit(void){
    if (nl_sk)
        netlink_kernel_release(nl_sk);
    pr_info(MODNAME ": rmmod\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jackson");
MODULE_DESCRIPTION("simple netlink module (echo + multicast)");
module_init(nl_echo_init);
module_exit(nl_echo_exit);
