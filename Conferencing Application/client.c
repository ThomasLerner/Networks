/*--------------------------------------------------------------------*/
/* conference client */

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

#define MAXMSGLEN 1024

extern char * recvdata(int sd);
extern int senddata(int sd, char *msg);

extern int connecttoserver(char *servhost, ushort servport);
/*--------------------------------------------------------------------*/

/*--------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
	int sock;

	/* check usage */
	if(argc != 3) {
		fprintf(stderr, "usage : %s <servhost> <servport>\n", argv[0]);
		exit(1);
	}

	/* connect to the server */
	sock = connecttoserver(argv[1], atoi(argv[2]));
	if(sock == -1)
		exit(1);
	
	/* no need to save fdset in temp variable since we always only have server and stdin */
	fd_set rfds;

	while(1) {
		/* init rfds */
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		FD_SET(fileno(stdin), &rfds);		
		
		/*
			use select() to watch for user inputs and messages from the server
		*/
		select(sock + 1, &rfds, NULL, NULL, NULL);
	
		if(FD_ISSET(sock, &rfds) /* message from server */) {
			char *msg;
			msg = recvdata(sock);
			if(!msg) {
				/* server died, exit */
				fprintf(stderr, "error: server died\n");
				exit(1);
			}

			/* print the message */
			printf(">>> %s", msg);
			free(msg);
		}

		if(FD_ISSET(fileno(stdin), &rfds) /* input from keyboard, could use 0 but this is slightly better */) {
			char msg[MAXMSGLEN];

			if (!fgets(msg, MAXMSGLEN, stdin))
				exit(0);
			senddata(sock, msg);
		}
	}
}
/*--------------------------------------------------------------------*/
