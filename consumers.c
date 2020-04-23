#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <prodcon.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


void close_socket( char* msg, int sock )

{
	if ( msg != 0 ) {
		fprintf( stderr, "consumer client: %s\n", msg );
	}
	close( sock );
	pthread_exit( NULL );
}


void output_item( ITEM* consumed)

{
	ITEM      *item;
	int       fd;
	pthread_t thread;
	char      fname[ 40 ];
	item = consumed;
	// printf("item %s\n", item->letters);
	thread = pthread_self();
	sprintf( fname, "%ld.txt", thread );
	// printf( "TID %ld size %d\n", thread, item->size);
	// printf("consumed %s\n", *consumed);
	fd = open( fname, O_CREAT | O_RDWR, 0777 );
	if ( fd == -1 ) {
		fprintf( stderr, "consumer: cannot open %s.\n", fname );
	} else {
		fprintf( stderr, "consumer: wrote file %s.\n", fname );
		write( fd, item->letters, item->size );
		close( fd );
	}
}


void* consume( void* csock )

{
	int   received_len;
	int   str_index;
	int   sock;
	int   rb;
	ITEM  item;
	char* received_str;

	str_index = 0;
	sock      = *( int* ) csock;
	free(csock);
	write(sock, "CONSUME\r\n", 9 );
	rb = read( sock, &received_len, 4 );
	if ( rb == 4 ) {
		received_len = ntohl( received_len );
		item.size    = received_len;
		received_str = malloc( ( received_len + 1 ) * sizeof( char ) );
		read(sock, ( void* ) (received_str), received_len);
		received_str[ item.size ] = '\0';
		item.letters = received_str;
		// printf("consume item %s\n", item.letters);
		output_item( &item );
		free( received_str );
		close_socket( "item was consumed.\n", sock );
	} else {
		close_socket( "server closed unexpectedly\n", sock );
	}
}


void main( int argc, char** argv )

{
	int   *csock;
	int   status;
	// void* threads;
	int   clients_num;
	int   started_threads;
	char* service;
	char* host;

	host = "localhost";
	if ( argc == 3 ) {
		service     = argv[ 1 ];
		clients_num = atoi( argv[ 2 ] );
	} else {
		if ( argc != 4 ) {
			fprintf( stderr, "usage: consumers [host] port num\n" );
			exit( -1 );
		}
		host        = argv[ 1 ];
		service     = argv[ 2 ];
		clients_num = atoi( argv[ 3 ] );
	}
	fprintf( stderr, "The count is %d\n", clients_num );
	pthread_t threads[ clients_num ];

	started_threads = 0;
	
	while ( started_threads < clients_num ) {
		csock = malloc(sizeof(int));
		*csock = connectsock( host, service, "tcp" );
		if ( csock == 0 )
			break;
		status = pthread_create( &threads[started_threads], NULL, consume, ( void* ) csock );
		if ( status != 0 ) {
			fprintf( stderr, "pthread_create error %d.\n", status );
			exit( -1 );
		}
		started_threads = started_threads + 1;
	}
	fprintf( stderr, "Have consumed every item.\n" );
	exit( -1 );
}
