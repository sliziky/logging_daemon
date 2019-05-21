#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>


#define PATH "/dev/log"
#define POS 20

typedef struct Sentence {
    char* sentence;
    int occurence;
} Sentence;

typedef struct Dictionary {
    int size;
    int capacity;
    Sentence* sentences;
} Dictionary;


void insert_into_dict( char* message );
void init_dict();
bool is_dict_full();
void get_substr( char* str, char* sub , int start, int len ){
    memcpy( sub, &str[ start ], len );
    sub[ len ] = '\0';
}
void open_files( FILE* files[], char** file_names, int size );
void realloc_dict();
void replace_first( char* str, char old, char new );
void sig_handler( int );
void write_into_files( FILE* files[], char* message, int size );

Dictionary dictionary;

int main(int argc, char** argv) {
    init_dict();
    signal( SIGINT, sig_handler );
    umask( 0 );
    unlink( PATH );


    FILE* files[ argc - 1 ];
    int sock = socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( sock < 0 ) {
        perror( "opening stream socket" );
        exit( 1 );
    }

    struct sockaddr_un server;
    server.sun_family = AF_UNIX;
    strcpy( server.sun_path, PATH );

    if ( bind(sock, (struct sockaddr* ) &server, sizeof(struct sockaddr_un)) ) {
        perror( "binding stream socket" );
        exit( 1 );
    }

    listen( sock, 5 );

    int msgsock;
    struct sockaddr_un  client;
    open_files( files, argv, argc - 1 );
    char buffer[ 1024 ];

    for( ;; ) {
        socklen_t len = sizeof( client );
        msgsock = accept( sock, (struct sockaddr* )&client, &len );

        if ( msgsock == -1 ) {
            perror( "accept" );
        }
        else {
            read( msgsock, buffer, 1024 );
            replace_first( buffer, '\n', '\0' );
        }

        char* message = malloc( strlen(buffer) );
        int to_be_read = strlen( buffer ) - POS;
        get_substr( buffer, message, POS, to_be_read );
        write_into_files( files, message, argc );
        insert_into_dict(message);

        printf( "%s \n", buffer );
        memset( buffer, 0, sizeof(buffer) );
        free( message );
    }

}

void sig_handler( int sig_num) {
    // find the most occured sentence
    int max_occur = dictionary.sentences[ 0 ].occurence;
    int max_occur_index = 0;
    for ( int i = 0; i < dictionary.size; ++i ) {
        if ( dictionary.sentences[ i ].occurence > max_occur ) {
            max_occur_index = i;
            max_occur = dictionary.sentences[ i ].occurence;
        }
    }
    printf("\n%d --> %s\nEND\n", dictionary.sentences[ max_occur_index ].occurence, dictionary.sentences[ max_occur_index ].sentence);
    for ( int i = 0; i < dictionary.size; ++i ) {
        free( dictionary.sentences[ i ].sentence );
    }
    free( dictionary.sentences );
    exit( 1 );
}

void replace_first( char* str, char old, char new ) {
    // replaces first occurence of 'old' with 'new'
    int counter = 0;
    for ( char* copy = str; *copy != '\0'; copy++, counter++ ) {
        if ( *copy == old ) {
            break;
        }
    }
    str[ counter ] = new;
}

int message_pos( char *message ) {
    for ( int i = 0; i < dictionary.size; ++i ) {
        if ( strcmp(dictionary.sentences[ i ].sentence, message) == 0 ) {
            return i;
        }
    }
    return -1;
}

bool message_exists( char* message ) {
    for ( int i = 0; i < dictionary.size; ++i ) {
        if ( strcmp(dictionary.sentences[ i ].sentence,message) == 0 ) {
            return true;
        }
    }
    return false;
}

void insert_into_dict( char* message ) {
    if ( is_dict_full() ) {
        realloc_dict();
    }
    if ( !message_exists(message) ) {
        // add message into the dictionary
        dictionary.sentences[ dictionary.size ].sentence = malloc( strlen(message) + 1 );
        strcpy( dictionary.sentences[ dictionary.size ].sentence, message );
        dictionary.sentences[ dictionary.size ].occurence = 1;
        dictionary.size++;
    }
    else {
        dictionary.sentences[ message_pos(message) ].occurence++;
    }
}


bool is_dict_full() {
    return dictionary.size + 1 == dictionary.capacity;
}
void init_dict() {
    dictionary.size = 0;
    dictionary.capacity = 10;
    dictionary.sentences = malloc( sizeof(Sentence) * dictionary.capacity );
}

void realloc_dict() {
    dictionary.capacity *= 2;
    dictionary.sentences = (Sentence*) realloc( dictionary.sentences, dictionary.capacity * sizeof(Sentence) );
}

void write_into_files( FILE* files[], char* message, int sz ) {
    for ( int i = 0; i < sz; ++i ) {
        fprintf( files[i], "%s\n", message );
    }
}

void open_files( FILE* files[], char** file_names, int sz ) {
    for ( int i = 0; i < sz; ++i ) {
        if ( strcmp(file_names[ i ], "-f") != 0 ) {
            files[ i ] = fopen( file_names[ i + 1 ], "a+" );
        }
    }
}

