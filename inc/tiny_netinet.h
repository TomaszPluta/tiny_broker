/*
 * tine_netinet.h
 *
 *  Created on: 23.06.2018
 *      Author: tomek
 */

#ifndef TINY_NETINET_H_
#define TINY_NETINET_H_

#ifdef TINY_NETINET_H_AVAIABLE
	#include <netinet/in.h>
#else
	#include <stdint.h>

	typedef uint32_t in_addr_t;
	struct in_addr
	  {
		in_addr_t s_addr;
	  };

	struct sockaddr_in {
		unsigned char	sin_len;
		unsigned char	sin_family;
		unsigned short	sin_port;
		struct	in_addr sin_addr;
		char	sin_zero[8];
	};

	struct sockaddr {
	   unsigned short   sa_family;
	   char             sa_data[14];
	};

#endif /* TINY_NETINET_H_AVAIABLE */

#endif /* TINY_NETINET_H_ */
