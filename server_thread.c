/**
 * @file server_proc.c
 * @brief A very simple threaded concurrent HTTP server. 
 *
 * @author Dr. Jonathan Misurda
 * @author Joshua Hellauer
 * @date 2024-11-04
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

FILE* stats_thread_txt;

enum { NS_PER_SECOND = 1000000000 };


/**
 * @brief subtract two timespecs.
 *
 * Credit goes to user `chux` on stackoverflow.
 * https://stackoverflow.com/questions/68804469/subtract-two-timespec-objects-find-difference-in-time-or-duration
 *
 * td = t1 - t2
 *
 * @param t1 The first timespec.
 * @param t2 The second timesepc.
 * @param td Timespec struct to hold the result.
 */
void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td)
{
    td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
    td->tv_sec  = t2.tv_sec - t1.tv_sec;
    if (td->tv_sec > 0 && td->tv_nsec < 0)
    {
        td->tv_nsec += NS_PER_SECOND;
        td->tv_sec--;
    }
    else if (td->tv_sec < 0 && td->tv_nsec > 0)
    {
        td->tv_nsec -= NS_PER_SECOND;
        td->tv_sec++;
    }
}


/**
 * @brief Worker thread for client response-handling.
 *
 * @param args The client socket descriptor value. 
 *
 */
void* handle_client_connection(void* args) {
	//At this point a client has connected. The remainder of the
	//loop is handling the client's GET request and producing
	//our response.
	int connfd = (int)args;
	struct timespec start, finish, delta;
	char buffer[1024];
	char filename[1024];
	FILE *f;

	memset(buffer,  0, sizeof(buffer));
	memset(filename, 0,  sizeof(buffer));

	//In HTTP, the client speaks first. So we recv their message
	//into our buffer.
	int amt = recv(connfd, buffer, sizeof(buffer), 0);
	fprintf(stderr, "%s", buffer);

	//We only can handle HTTP GET requests for files served
	//from the current working directory, which becomes the website root
	if(sscanf(buffer, "GET /%s", filename)<1) {
		fprintf(stderr, "Bad HTTP request\n");
		close(connfd);
		pthread_exit(NULL);
	}

	//If the HTTP request is bigger than our buffer can hold, we need to call
	//recv() until we have no more data to read, otherwise it will be
	//there waiting for us on the next call to recv(). So we'll just
	//read it and discard it. GET should be the first 3 bytes, and we'll
	//assume paths that are smaller than about 1000 characters.
	if(amt == sizeof(buffer))
	{
		//if recv returns as much as we asked for, there may be more data
		while(recv(connfd, buffer, sizeof(buffer), 0) == sizeof(buffer))
			/* discard */;
	}

	//if we don't open for binary mode, line ending conversion may occur.
	//this will make a liar our of our file size.
	f = fopen(filename, "rb");
	
	if(f == NULL)
	{
		//Assume that failure to open the file means it doesn't exist
		strcpy(buffer, "HTTP/1.1 404 Not Found\n\n");
		send(connfd, buffer, strlen(buffer), 0);
	}
	else
	{
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

		int filesize;
		char response[1024];

		strcpy(response, "HTTP/1.1 200 OK\n");
		send(connfd, response, strlen(response), 0);

		time_t now;
		time(&now);
		//How convenient that the HTTP Date header field is exactly
		//in the format of the asctime() library function.
		//
		//asctime adds a newline for some dumb reason.
		sprintf(response, "Date: %s", asctime(gmtime(&now)));
		send(connfd, response, strlen(response), 0);

		//Get the file size via the stat system call
		struct stat file_stats;
		fstat(fileno(f), &file_stats);
		sprintf(response, "Content-Length: %ld\n", file_stats.st_size);
		send(connfd, response, strlen(response), 0);

		//Tell the client we won't reuse this connection for other files
		strcpy(response, "Connection: close\n");
		send(connfd, response, strlen(response), 0);

		//Send our MIME type and a blank line
		strcpy(response, "Content-Type: text/html\n\n");
		send(connfd, response, strlen(response), 0);

		fprintf(stderr, "File: %s\n", filename);

		int bytes_read = 0;
		do
		{
			//read response amount of data at a time.
			//Note that sizeof() in C can only tell us the number of
			//elements in an array that is declared in scope. If you
			//move the declaration elsewhere, it will degrade into
			//the sizeof a pointer instead.
			bytes_read = fread(response, 1, sizeof(response), f);

			//if we read anything, send it
			if(bytes_read > 0)
			{
				int sent = send(connfd, response, bytes_read, 0);
				//It's possible that send wasn't able to send all of
				//our response in one call. It will return how much it
				//actually sent. Keep calling send until all of it is
				//sent.
				while(sent < bytes_read)
				{
					sent += send(connfd, response+sent, bytes_read-sent, 0);
				}
				filesize += sent;
			}
		} while(bytes_read > 0 && bytes_read == sizeof(response));
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &finish);
		sub_timespec(start, finish, &delta);
		{
			pthread_mutex_lock(&mutex);
			fprintf(stats_thread_txt, "%s\t%d\t%d.%.9ld\n", filename, filesize, (int)delta.tv_sec, delta.tv_nsec);
			fflush(stats_thread_txt);
			pthread_mutex_unlock(&mutex);
		}
		fprintf(stderr, "Just logged %s\t%d\t%d.%.9ld\n", filename, filesize, (int)delta.tv_sec, delta.tv_nsec);		
		
		fclose(f);
	}
	shutdown(connfd, SHUT_RDWR);
	close(connfd);
	pthread_exit(NULL);
}

int main()
{
	// open the log file
	stats_thread_txt = fopen("stats_thread.txt", "a");
	if(stats_thread_txt == NULL) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	//Sockets represent potential connections
	//We make an internet socket
	int sfd = socket(PF_INET, SOCK_STREAM, 0);
	if(-1 == sfd)
	{
		perror("Cannot create socket\n");
		exit(EXIT_FAILURE);
	}

	// Avoid "Bind: Address Already in Use" failures
	int yes = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	//We will configure it to use this machine's IP, or
	//for us, localhost (127.0.0.1)
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	//Web servers always listen on port 80
	addr.sin_port = htons(80);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//So we bind our socket to port 80
	if(-1 == bind(sfd, (struct sockaddr *)&addr, sizeof(addr)))
	{
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	//And set it up as a listening socket with a backlog of 10 pending connections
	if(-1 == listen(sfd, 10))
	{
		perror("Listen failed");
		exit(EXIT_FAILURE);
	}

	//A server's gotta serve...
	for(;;)
	{
		//accept() blocks until a client connects. When it returns,
		//we have a client that we can do client stuff with.
		pthread_t tid;
		int connfd = accept(sfd, NULL, NULL);
		if(connfd < 0)
		{
			perror("Accept failed");
			exit(EXIT_FAILURE);
		}
		pthread_create(&tid, NULL, handle_client_connection, (void *)connfd);
	}

	//clean up
	fclose(stats_thread_txt);
	close(sfd);
	return 0;
}
