
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "common.hpp"

int createTCPServerSocket( unsigned short port ) {
	int server_fd;

	server_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( server_fd < 0 ) {
		fprintf( stderr, "Failed to create server socket [%s]\n", strerror(errno) );
	} else {
		int opt = 1;
		if ( setsockopt( server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int) ) < 0 ) {
			fprintf( stderr, "Failed to set server socket reuseaddr [%s]\n", strerror(errno) );
			close( server_fd );
			server_fd = -1;
		} else {
			struct sockaddr_in	sai;
			memset( &sai, 0, sizeof(sai) );
			sai.sin_family = AF_INET;
			sai.sin_port = htons( port );
			sai.sin_addr.s_addr = htonl( INADDR_ANY );
			if ( bind( server_fd, (const sockaddr*)&sai, sizeof(sai) ) < 0 ) {
				fprintf( stderr, "Failed to bind server socket [%s]\n", strerror(errno) );
				close( server_fd );
				server_fd = -1;
			} else if ( listen( server_fd, 1 ) < 0 ) {
				fprintf( stderr, "Server wont listen [%s]\n", strerror(errno) );
				close( server_fd );
				server_fd = -1;
			} else {
				/* cool */
			}
		}
	}
	return server_fd;
}

