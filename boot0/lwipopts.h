/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt
 *
 */
#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

/* Prevent having to link sys_arch.c (we don't test the API layers in unit tests) */
#define NO_SYS                          1
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                (32*1024)
#define MEMP_NUM_PBUF           32

#define LWIP_RAW                        0
#define LWIP_NETCONN                    0
#define LWIP_SOCKET                     0
#define LWIP_DHCP                       0
#define LWIP_ICMP                       1
#define LWIP_UDP                        1
#define LWIP_TCP                        1
#define LWIP_IPV4                       1
#define LWIP_IPV6                       0
#define ETH_PAD_SIZE                    0
#define LWIP_IP_ACCEPT_UDP_PORT(p)      ((p) == PP_NTOHS(67))

#define TCP_MSS                         (3400 /*mtu*/ - 20 /*iphdr*/ - 20 /*tcphhr*/)
#define TCP_SND_BUF                     (4 * TCP_MSS)
#define TCP_WND                         (4 * TCP_MSS)

#define ETHARP_SUPPORT_STATIC_ENTRIES   1
//#define LWIP_DEBUG  LWIP_DBG_ON
//#define HTTPD_DEBUG LWIP_DBG_ON
//#define HTTPD_POLL_INTERVAL             1
#define LWIP_HTTPD_SUPPORT_11_KEEPALIVE 0
#define LWIP_HTTPD_CGI                  0
#define LWIP_HTTPD_SSI                  1
#define LWIP_HTTPD_SSI_INCLUDE_TAG      0
#define LWIP_HTTPD_SUPPORT_POST         1  
#define LWIP_HTTPD_POST_MANUAL_WND      1
#define LWIP_HTTPD_CUSTOM_FILES         1
#define LWIP_HTTPD_DYNAMIC_FILE_READ    1
#define LWIP_HTTPD_DYNAMIC_HEADERS      1  
#define LWIP_HTTPD_SSI_RAW              1
#define LWIP_HTTPD_MAX_TAG_INSERT_LEN   512

#define LWIP_SINGLE_NETIF               1

#define PBUF_POOL_SIZE                  24
#define PBUF_POOL_BUFSIZE               3000

#define LWIP_MULTICAST_PING             1
#define LWIP_BROADCAST_PING             1
#define LWIP_IPV6_MLD                   0
#define LWIP_IPV6_SEND_ROUTER_SOLICIT   0

#endif /* __LWIPOPTS_H__ */
