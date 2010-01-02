#include <sys/types.h>

#include <string.h>


#include "base.h"
#include "inet_ntop_cache.h"
#include "sys-socket.h"

const char *inet_ntop_cache_get_ip(server * srv, sock_addr * addr)
{
#ifdef HAVE_IPV6
	size_t ndx = 0, i;
	/*
	 * 首先在缓冲区srv -> inet_ntop_cache中查找，看看是否有已经转换好的地址。
	 * 如果找到了，则直接返回即可。
	 * 否则，将地址进行转换并存入缓冲区中。
	 */
	for (i = 0; i < INET_NTOP_CACHE_MAX; i++)
	{
		if (srv->inet_ntop_cache[i].ts != 0)
		{
			if (srv->inet_ntop_cache[i].family == AF_INET6 &&
				0 == memcmp(srv->inet_ntop_cache[i].addr.ipv6.s6_addr, addr->ipv6.sin6_addr.s6_addr, 16))
			{
				/*
				 * IPv6 found in cache 
				 */
				break;
			} 
			else if (srv->inet_ntop_cache[i].family == AF_INET &&
					   srv->inet_ntop_cache[i].addr.ipv4.s_addr == addr->ipv4.sin_addr.s_addr)
			{
				/*
				 * IPv4 found in cache 
				 */
				break;

			}
		}
	}

	if (i == INET_NTOP_CACHE_MAX)
	{
		/*
		 * not found in cache 
		 */

		i = ndx;
		/*
		 * 		const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt);
		 * This function converts the network address structure src in af address family into a character string, 
		 * which is copied to a character buffer dst, which is cnt bytes long.
		 * 也就是将网络字节序的二进制地址转换成人可以识别的文本形式。
		 *
		 * inet_ntop() extends the inet_ntoa() function to support mutiple address families.
		 */
		inet_ntop(addr->plain.sa_family,
				  addr->plain.sa_family == AF_INET6 ? (const void *) &(addr->ipv6.sin6_addr) : (const void *) &(addr->ipv4.sin_addr),
				  srv->inet_ntop_cache[i].b2, INET6_ADDRSTRLEN);

		//存入缓冲区。
		//二进制的地址和文本形式的地址都缓存。方便查找。
		srv->inet_ntop_cache[i].ts = srv->cur_ts; 	//存储的时间。
		srv->inet_ntop_cache[i].family = addr->plain.sa_family;

		if (srv->inet_ntop_cache[i].family == AF_INET)
		{
			srv->inet_ntop_cache[i].addr.ipv4.s_addr =
				addr->ipv4.sin_addr.s_addr;
		} 
		else if (srv->inet_ntop_cache[i].family == AF_INET6)
		{
			memcpy(srv->inet_ntop_cache[i].addr.ipv6.s6_addr,
				   addr->ipv6.sin6_addr.s6_addr, 16);
		}
	}

	return srv->inet_ntop_cache[i].b2;
#else
	UNUSED(srv);
	return inet_ntoa(addr->ipv4.sin_addr);
#endif
}
