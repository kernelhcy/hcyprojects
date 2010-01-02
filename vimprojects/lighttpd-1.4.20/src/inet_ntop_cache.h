#ifndef _INET_NTOP_CACHE_H_
#define _INET_NTOP_CACHE_H_

/**
 * 将网路字节序的二进制网络地址addr转换成人可以识别的文本形式。
 * 
 * 改函数首先在srv中的inet_ntop_cache中查找是否有转换好的缓存的数据，
 * 如果找到，则直接返回数据。
 * 如果没有，则调用inet_ntop() (在只有ipv4的情况下，调用inet_ntoa())函数转换地址，
 * 并将转换后的文本地址和二进制地址都存入到srv中的地址缓存inet_ntop_cache中，以备以后
 * 查询使用。
 *
 * 函数中涉及到ipv4和ipv6的区分处理。
 */
#include "base.h"
const char *inet_ntop_cache_get_ip(server * srv, sock_addr * addr);

#endif
