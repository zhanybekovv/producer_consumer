#include "prodcon.h"
#include <stdbool.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


int             count, prod_count, con_count, client_count;
ITEM**          buffer;
pthread_mutex_t pcmutex, mtmutex;
sem_t           full, empty;




void* serve( void* sock )

{
	bool    is_free;
	int rb;
	char    buf[ 1024 ];
	int ssock = *( int* ) sock;
	free(sock);
	rb = read( ssock, buf, 1024 );

	if ( rb < 0 ) {
		// printf("cant read value%d\n", rb);
		// printf("cant read buffer value%s\n", buf);
		close_socket( "can't read from sock", ssock, 2 );
	} else {
		// printf("i'm in serv_produce\n");
		buf[ rb ] = '\0';
		if ( strcmp( buf, "PRODUCE\r\n" ) == 0 ) {
			pthread_mutex_lock( &mtmutex );
			is_free = prod_count < MAX_PROD;
			if ( is_free ) {
				prod_count = prod_count + 1;
			}
			pthread_mutex_unlock( &mtmutex );
			if ( is_free ) {
				produce( ssock );
				pthread_exit( NULL );
			}
			close_socket( "REJECTED\n", ssock, 2 );
		}
		if(strcmp( buf, "CONSUME\r\n" ) == 0 ){
			pthread_mutex_lock( &mtmutex );
			is_free = con_count < MAX_CON;
			if ( is_free ) {
				con_count = con_count + 1;
			}
			pthread_mutex_unlock( &mtmutex );
			if ( is_free ) {
				consume(  ssock );
				pthread_exit( NULL );
			}
			close_socket( "REJECTED\n", ssock, 2 );
		} 
		else {
				printf("error command %s and buf size %d\n", buf, strlen(buf));
				close_socket( "it is not produce or consume", ssock, 2 );
			}
		}
	}

void consume( int sock )

{
	ITEM*    sent_item;
	sem_wait( &full );
	pthread_mutex_lock( &pcmutex );
	sent_item             = buffer[ count - 1 ];
	buffer[ count - 1 ] = NULL;
	count                = count - 1;
	printf( "C Count %d.\n", count );
	pthread_mutex_unlock( &pcmutex );
	sem_post( &empty );
	int item_size = htonl( sent_item->size );
	write( sock, &item_size, 4 );
	// printf("consume in server side %s\n", sent_item->letters);
	write( sock, sent_item->letters, sent_item->size );
	free( sent_item->letters );
	free( sent_item );
	close_socket( "Successfully sent the item to consumer.\n", sock, 1 );
}

void produce( int sock )

{
	int received_len;
	int      str_index;
	int      rb;
	ITEM*    received_item;

	str_index     = 0;
	received_item = ( uint32_t* ) malloc( 0x10 );
	// sem_wait( &empty );
	// pthread_mutex_lock( &pcmutex );
	// printf("in produce\n");
	write( sock, "GO\r\n", 4 );
	read( sock, &received_len, 4 );
	received_len        = ntohl( received_len );
	received_item->size = received_len;
	// printf( "The len is %d\n", received_len );
	received_item->letters = malloc( ( received_len + 1 ) * sizeof( char ) );
	read(sock, ( void* ) ( received_item->letters), received_len);
	// printf("recieved item %s\n", received_item->letters);
	sem_wait( &empty );
	pthread_mutex_lock( &pcmutex );
	buffer[ count ] = received_item;
	count            = count + 1;
	printf( "C Count %d.\n", count );
	received_item->letters[ received_item->size ] = '\0';
	pthread_mutex_unlock( &pcmutex );
	sem_post( &full );
	write( sock, "DONE\r\n", 6 );

	close_socket( "item was recieved by server with success", sock, 0 );
	// }
	// pthread_mutex_unlock( &pcmutex );
	// sem_post( &full );
}


void close_socket( char *msg, int sock, int type )

{
	
	printf( "pcserver: %s\n", msg );
	// sem_wait( &empty );
	pthread_mutex_lock( &mtmutex );
	client_count = client_count - 1;
	if ( type == 0 ) {
		prod_count = prod_count - 1;
		printf("prod count is %d\n", prod_count);
	}
	if ( type == 1 ) {
		con_count = con_count - 1;
		printf("cons count is %d\n", con_count);
	}else
	{
		printf("error occured\n");
	};
	
	pthread_mutex_unlock( &mtmutex );
	// sem_post(&full);
	close( sock );
	pthread_exit( NULL );
}

int main( int argc, char** argv )

{
	char*              err;
	socklen_t          alen;
	int                rport;
	int                maxitems;
	int                msock;
	int                *ssock;
	
	char*              service;
	struct sockaddr_in fsin;
	rport    = 0;
	maxitems = 0;
	
	if ( argc != 3 ) {
			fprintf( stderr, "usage: pcserver [port] maxitems\n" );
			exit( -1 );
		}
		// printf("%s\n", argv[1]);
		// printf("%s\n", argv[2]);
		service  = argv[ 1 ];
		maxitems = atoi( argv[ 2 ] );
	if ( 0 < maxitems ) {
		buffer = malloc( maxitems * sizeof( ITEM* ) );
		pthread_mutex_init( &pcmutex, 0 );
		pthread_mutex_init( &mtmutex, 0 );
		sem_init( &full, 0, 0 );
		sem_init( &empty, 0, maxitems );
		count       = 0;
		client_count = 0;
		con_count    = 0;
		prod_count   = 0;
		// int *ptr;
		// int arr[maxitems];
		// ptr = arr;
		msock        = passivesock( service, "tcp", QLEN, &rport );
		if ( rport != 0 ) {
			fprintf( stderr, "pcserver: port is %d\n", rport );
			fflush( stdout );
		}
		while ( 1 ) {
			pthread_t thread;
			alen  = sizeof( fsin );
			ssock = malloc(sizeof(int));
			*ssock = accept( msock, &fsin, &alen );
			// arr[client_count] = ssock;
			
			if ( ssock < 0 )
				break;
			puts( "A client has arrived." );
			fflush( stdout );
			pthread_mutex_lock( &mtmutex );
			if ( client_count < MAX_CLIENTS ) {
				
				pthread_create( &thread, NULL, serve, (void *)ssock);
				// printf("arr of sock %d\n", *ptr);
				// ptr++;
				client_count = client_count + 1;
				
			} else {
				close( ssock );
				fprintf( stderr, "REJECTED\n" );
			}
			pthread_mutex_unlock( &mtmutex );
			
		}
		err = strerror( errno );
		fprintf( stderr, "accept: %s\n", err );
		pthread_exit( NULL );
	}
	else {
		fprintf( stderr, "pcserver: maxitems must be > 0\n" );
	};
	exit( -1 );
}



