/*--------------------------------------------------------------------*/
/* conference server */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h>
#include <time.h> 
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

extern char * recvdata(int sd);
extern int senddata(int sd, char *msg);

extern int startserver();
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
/* main function*/
int main(int argc, char *argv[]) {
	int serversock; 
	
	fd_set liveskset; /* set of live client sockets */
	int liveskmax; /* maximum socket */

	/* check usage */
	if(argc != 1) {
		fprintf(stderr, "usage : %s\n", argv[0]);
		exit(1);
	}

	/* get ready to receive requests */
	serversock = startserver();
	if(serversock == -1) {
		exit(1);
	}
	/* set max socket */
	liveskmax = serversock;
	
	/*
		TODO:
		init the live client set 
	*/
	FD_ZERO(&liveskset);
	FD_SET(serversock, &liveskset);

	/* receive and process requests */
	while(1) {
		bool stop = false; /* bool to break loop after liveskset has been used and reset */
		int itsock; /* loop variable */
		fd_set temp = liveskset; /* temp storage of client sockets */
		/* select() halts process until input is recieved */
		select(liveskmax + 1, &liveskset, NULL, NULL, NULL);
		
		/* process messages from clients */
		for(itsock=3; itsock <= liveskmax; itsock++) {
			/* skip the listen socket */
			if(itsock == serversock) continue;
			
			if(FD_ISSET(itsock, &liveskset) /* message from client */ ) {
				char * clienthost; /* host name of the client */
				ushort clientport; /* port number of the client */
				liveskset = temp; /* reset liveskset */
				stop = true; /* indicate that liveskset has been reset */
	
				/*
					obtain client's host name and port
					using getpeername() and gethostbyaddr()
				*/
				struct sockaddr_in cli_addr;
				bzero((char *) &cli_addr, sizeof(cli_addr));
				socklen_t foo = sizeof(cli_addr);
				getpeername(itsock, (struct sockaddr *) &cli_addr, &foo);
				struct hostent* clienthostent = gethostbyaddr(&cli_addr, sizeof(cli_addr), AF_INET);
				clienthost = clienthostent->h_name;
				clientport = ntohs(cli_addr.sin_port);
				
				/* read the message */
				char * msg = recvdata(itsock);
				if(!msg) {
					/* disconnect from client */
					*clienthost = 1;
					printf("admin: disconnect from '%s(%hu)'\n",
						clienthost, clientport);

					/*
						remove this client from 'liveskset'	
					*/
					FD_CLR(itsock, &liveskset);
					close(itsock);
				}
				else {
					/*
						send the message to other clients
					*/
					for(int sendsock = 3; sendsock <= liveskmax; sendsock++) {
						/* make sure that the client is actually connected first! */
						if(sendsock != serversock && sendsock != itsock && FD_ISSET(sendsock, &liveskset))
							senddata(sendsock, msg);
					}
					/* print the message */
					printf("%s(%hu): %s", clienthost, clientport, msg);
					free(msg);
				}
				break;
			}
		}
		if(FD_ISSET(serversock, &liveskset) && stop == false /* connect request from a new client, make sure that liveskset hasn't been reset first */ ) {
			liveskset = temp;
			/*
				accept a new connection request
			*/
			struct sockaddr_in cli_addr;
			bzero((char *) &cli_addr, sizeof(cli_addr));
			socklen_t foo = sizeof(cli_addr);
			int newsockfd = accept(serversock, (struct sockaddr *) &cli_addr, &foo);
			if(newsockfd >= 0 /* if accept is fine */) {
				char * clienthost; /* host name of the client */
				ushort clientport; /* port number of the client */

				/*
					get client's host name and port using gethostbyaddr() 
				*/
				struct hostent* clienthostent = gethostbyaddr(&cli_addr, sizeof(cli_addr), AF_INET);
				clienthost = clienthostent->h_name;
				clientport = ntohs(cli_addr.sin_port);
				
				printf("admin: connect from '%s' at '%hu'\n",
					clienthost, clientport);

				/*
					add this client to 'liveskset'
				*/
				FD_SET(newsockfd, &liveskset);
				if(newsockfd > liveskmax) liveskmax = newsockfd;
			}
			else {
				perror("accept");
				exit(0);
			}
		}
	}
}
/*--------------------------------------------------------------------*/
