#include "prodcon.h"

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

struct timeval  tv;
double time_in_mill;

void close_socket( char* msg, int sock )

{
	if ( msg != 0 ) {
		printf( "producer client: %s\n", msg );
	}
	close( sock );
	pthread_exit( NULL );
}


void fill_item( ITEM* item )

{
	item->size    = rand() % MAX_LETTERS;
	item->letters = malloc( item->size * sizeof( char ) );
	int i         = 0;
	while ( i < item->size ) {
		item->letters[ i ] = 'X';
		i                  = i + 1;
	};
	// item->size = 20;
	// item->letters = "Hello my friends!";
	// printf("sent item from producers %s\n", item->letters);
}


void* produce( void* csock )

{
	uint32_t sent_len;
	int      sock;
	int      rb;
	ITEM     item;
	char     msg[ 8 ];
	sock = *( int* ) csock;
	free(csock);
	fill_item( &item );
	write( sock, "PRODUCE\r\n", 9 );
	rb       = read( sock, msg, 4 );

	printf("server send %s\n", msg);
	msg[ 4 ] = '\0';
	if ( rb == 4 && strcmp( msg, "GO\r\n" ) == 0 ) {
		sent_len = htonl( item.size );
		write( sock, &sent_len, 4 );
		write( sock, item.letters, item.size );
		rb       = read( sock, msg, 6 );
		msg[ 6 ] = '\0';
		printf("must be done!!! %s and size of it is %d\n", msg, strlen(msg));
		if ( rb == 6 && strcmp( msg, "DONE\r\n" ) == 0 ) {
			close_socket( "item was sent to server.", sock );
			// close( sock );
			// pthread_exit( NULL );
		} else {
			printf("wrong message %s\n", msg);
			close_socket( "wrong message\n", sock );
			// close( sock );
			// pthread_exit( NULL );
		}
		// close( sock );
		// pthread_exit( NULL );
	}
	// close( sock );
	else{
		close_socket( "Server closed\n", sock );
		// pthread_exit( NULL );
	};
}


void main( int argc, char** argv )

{
	int   *csock;
	int   status;
	int   clients_num;
	int   started_threads;
	char* service;
	char* host;
	// int i = 0;
	host = "localhost";
	if ( argc == 3 ) {
		service     = argv[ 1 ];
		clients_num = atoi( argv[ 2 ] );
	} else {
		if ( argc != 4 ) {
			fprintf( stderr, "usage: producers [host] port num\n" );
			exit( -1 );
		}
		host        = argv[ 1 ];
		service     = argv[ 2 ];
		clients_num = atoi( argv[ 3 ] );
		printf("produce 4\n");
	}
	int arr[clients_num];
	int *ptr;
	ptr = arr;
	printf( "The count is %d\n", clients_num );
	pthread_t threads[ clients_num ];
	started_threads = 0;
	while ( started_threads < clients_num ) {
		csock = malloc(sizeof(int));
		*csock = connectsock( host, service, "tcp" );
		// arr[started_threads] = csock;
		if ( csock == 0 ){
			printf("couldnt connect to server\n");
			break;
		}
		status = pthread_create( &threads[ started_threads ], NULL,
								 produce, (void *)csock );
		printf("status %d\n", status);
		if ( status != 0 ) {
			fprintf( stderr, "pthread_create error %d.\n", status );
			exit( -1 );
		}
		printf("P count %d\n", started_threads);
		started_threads = started_threads + 1;
	}
	fprintf( stderr, "Sent all items.\n" );
	exit( -1 );
}
