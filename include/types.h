#ifndef __TYPES__H__
#define __TYPES__H__
#define _GNU_SOURCE

/* netinet*/
#include <netinet/in.h>
#if 0
#include <netinet/ip_icmp.h>	
#include <netinet/udp.h>	
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <net/ethernet.h>
#endif 
#include <arpa/inet.h>
#include <netdb.h>
/* linux*/
//#include <linux/if_packet.h>
//#include <linux/if_tun.h>
//#include <linux/if_ether.h>
//#include <linux/if.h>

/*sys*/
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <pthread.h>
#include <sched.h>
#include <limits.h>
#include <signal.h>
#include <syscall.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

#endif  //!__TYPES__H__