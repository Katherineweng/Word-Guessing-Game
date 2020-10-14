/* 
	Author: Katherine Weng
			Song Xu
			Huanzhi Zhu
	CSCI-4220, HW2 Client
*/

/* To compile: gcc -o lab3_client.out lab3_client.c lib/libunp.a
#include "lib/unp.h"
*/

/* Change the #include for submission */
#include "../../unpv13e/lib/unp.h"
/* #include "lib/unp.h"  use this header if .c is in folder unpv13e*/
#include <string.h>
/* */

/*---------------------------- GLOBAL VAIRABLES -----------------------------*/
#define MAX_LENGTH 1024
/*---------------------------------------------------------------------------*/

int main( int argc, char **argv ) {

	setbuf(stdout, NULL);

	int					sockfd, port, maxfdp1, i;
	struct sockaddr_in	servaddr;
	char*				sendline = calloc(MAX_LENGTH, sizeof(char));
	char*				recvline = calloc(MAX_LENGTH, sizeof(char));
	fd_set				rset;

	if ( argc != 2 ) {
		fprintf( stderr, "ERROR: Invalid argument(s)\n"
						 "USAGE: a.out [port]\n" );
		return EXIT_FAILURE;
	}

	port = atoi( *(argv+1) );

	printf("port: %d\n", port);

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	/* connect to 127.0.0.1 on that port */
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	FD_ZERO(&rset);
	for ( ; ; ) {
		FD_SET(fileno(stdin), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(stdin), sockfd) + 1;
		Select(maxfdp1, &rset, NULL, NULL, NULL);

		/* check stdin */
		if ( FD_ISSET(fileno(stdin), &rset) ) {
			if (Fgets(sendline, MAX_LENGTH, stdin) == NULL) {
				perror("fgets() failed!");
			}
			if (strcmp(sendline, "close\n") == 0) {
				close(sockfd);
				printf("closing sd\n");
				break;
			}
			Send(sockfd, sendline, strlen(sendline), 0);
		}

		/* check server for data */
		if (FD_ISSET(sockfd, &rset)) {  /* socket is readable */
			/* 0 indicates end of file */
			if (Recv(sockfd, recvline, MAX_LENGTH, 0) == 0) {
				printf("Server on %d closed\n", port);
				close(sockfd);
				break;
			} else {
				printf("%s", recvline);
				memset(recvline, 0, MAX_LENGTH);
			}
		}

	}

	return EXIT_SUCCESS;
}