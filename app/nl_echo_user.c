#include <linux/netlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_PAYLOAD 1024

int main(void) {
  int sockfd;
  struct sockaddr_nl src = {0}, dst = {0};
  struct nlmsghdr *nlh = NULL;
  struct iovec iov;
  struct msghdr msg = {0};
  const char *hello = "hello from userspace";

  sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);

  if (sockfd < 0) {
    perror("socket");
    return 1;
  }

  src.nl_family = AF_NETLINK;
  src.nl_pid = getpid();
  if (bind(sockfd, (struct sockaddr *)&src, sizeof(src)) < 0) {
    perror("bind");
    close(sockfd);
    return 1;
  }

  dst.nl_family = AF_NETLINK;
  dst.nl_pid = 0; /* 0 = kernel */
  dst.nl_groups = 0;

  nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
  if (!nlh) {
    perror("malloc");
    close(sockfd);
    return 1;
  }
  memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
  nlh->nlmsg_len = NLMSG_LENGTH(strlen(hello) + 1);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;
  memcpy(NLMSG_DATA(nlh), hello, strlen(hello) + 1);

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;

  msg.msg_name = (void *)&dst;
  msg.msg_namelen = sizeof(dst);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  if (sendmsg(sockfd, &msg, 0) < 0) {
    perror("sendmsg");
    goto out;
  }

  /* 接收内核回包（阻塞等待） */
  memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
  iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);

  if (recvmsg(sockfd, &msg, 0) < 0) {
    perror("recvmsg");
    goto out;
  }

  printf("userspace got: '%s'\n", (char *)NLMSG_DATA((struct nlmsghdr *)nlh));

out:
  free(nlh);
  close(sockfd);
  return 0;
}
