
/* To compile: gcc -o word_guess.out hw2.c ../../unpv13e/lib/libunp.a */

#include "../../unpv13e/lib/unp.h"
/* #include "lib/unp.h"  use this header if .c is in folder unpv13e*/
#include <string.h>
#include <ctype.h>

/*---------------------------- Struct Declaration ---------------------------*/
typedef struct USER {
	int sd;
	char* id;
	char* lower_id;
} USER;

/*---------------------------------------------------------------------------*/

/*---------------------------- GLOBAL VAIRABLES -----------------------------*/

/* Support up to 5 clients */
#define MAX_CLIENT 5
#define MAX_LENGTH 1024

USER* user_pool;	/* stores info of active users */
int N = 0; 			/* represent # of active users */

int Dict_size = 0;

char* Secret;
int Secret_length;

char* Welcome = "Welcome to Guess the Word, please enter your username.\n";

/*---------------------------------------------------------------------------*/

/*-------------------------- Function Declaration ---------------------------------------*/
char** read_dict(char* filename, int longest);
void get_secret_word(char** dict);
char* to_lower(char* str);
int find_index(int sd);
int name_set(int sd);
int valid_username(char* new_user, int index);
void disconnect(int sd);
void get_uername(int sd);
int get_answer(int sd);
int get_correct_letter(char* word);
int get_correct_place(char* word);
int guessed_correctly(int correct_letter, int correct_place);
void send_all(int sd, char* guessed, int letter, int place);
void disconnect_all();
void send_all_and_disconnect(int sd);
/*----------------------------------------------------------------------------------------*/

int main( int argc, char **argv ) {
	/*-------------------------- CHECK COMMAND LINE ARGUMENTS --------------------------*/
	if ( argc != 5 ) {
		fprintf( stderr, "ERROR: Invalid argument(s)\n"
						 "USAGE: a.out [seed] [port] "
						 "[dictionary_file] [longest_word_length]\n" );
		return EXIT_FAILURE;
	}

	int seed = atoi( *(argv+1) );
	int port = atoi( *(argv+2) );
	char* file = (char*) calloc(256, sizeof(char));
	strcpy(file, *(argv+3));
	int longest = atoi( *(argv+4) );
	/*----------------------------------------------------------------------------------*/

	/*------------------------------- READ IN DICTIONARY -------------------------------*/

	char** dict = read_dict(file, longest);

	/*----------------------------------------------------------------------------------*/

	/*------------------------------- Select Secret Word -------------------------------*/

	srand(seed);

	Secret = malloc(MAX_LENGTH*sizeof(char));

	get_secret_word(dict);

	printf("seed: %d, port: %d, file name: %s, longest word length: %d\n", 
			seed, port, file, longest);
	printf("Word: %s, Word Length: %d\n", Secret, Secret_length);
	/*----------------------------------------------------------------------------------*/

	/*----------------------------------- TCP SETUP ------------------------------------*/

	int					sock, connfd, maxfdp1, sd, i;
	struct sockaddr_in	servaddr, cliaddr;
/*	char*				sendline = calloc(MAX_LENGTH, sizeof(char));
	char*				recvline = calloc(MAX_LENGTH, sizeof(char)); */
	int*				clisock;
	fd_set				rset;

	sock = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(port);

	Bind(sock, (SA *) &servaddr, sizeof(servaddr));

	Listen(sock, MAX_CLIENT);
	printf( "Listening for TCP connections on port: %d\n", port );

	clisock = (int*) calloc(MAX_CLIENT, sizeof(int));

	/*----------------------------------------------------------------------------------*/

	user_pool = (USER*) calloc( MAX_CLIENT, sizeof(USER) );

	int fromlen = sizeof( cliaddr );

	/*----------------------------- START RECEIVING REQUEST ----------------------------*/
	while ( 1 ) {
    	FD_ZERO( &rset );		/* clear the descriptor set */
	    FD_SET( sock, &rset );	/* add primary socket (that waits for connection) into the set */
	    maxfdp1 = sock;

	    for (i = 0; i < N; i++) {		/* add client sockets into the set */
	    	sd = user_pool[i].sd;
	    	if (sd > 0) FD_SET( sd, &rset ); 	/* valid socket */
	    	if (sd > maxfdp1) maxfdp1 = sd;		/* find highest file descriptor */
	    }
	    maxfdp1 += 1;

		Select(maxfdp1, &rset, NULL, NULL, NULL); /* BLOCKING call, will block on all rset */

		if ( FD_ISSET( sock, &rset ) ) {	/* if an incoming connection occurs */
			connfd = Accept( sock, (SA *) &cliaddr, &fromlen);
			if (N >= 5) {
				close(connfd);
				continue;
			}
			printf( "Rcvd incoming TCP connection from: %s, sd: %d\n", inet_ntoa( cliaddr.sin_addr ), connfd );
			Send(connfd, Welcome, strlen(Welcome), 0);
			user_pool[N].sd = connfd;
			N++;
		}

		int correct = 0;
		for ( i = 0; i < N; i++ ) { /* check all connected users' status */
			sd = user_pool[i].sd;
			if ( FD_ISSET( sd, &rset ) ) {
				if ( !name_set(sd) ) {
					get_uername(sd);
				} else {

					correct = get_answer(sd);
#if 0
					/* Below is just for echoing */
					int rc = Recv(sd, recvline, MAX_LENGTH, 0);
					if (rc == 0) {
						printf("closing sd %d\n", sd);
						disconnect(sd); /* modify user_pool */
						close(sd);
						continue;
					}
					recvline[rc] = '\0';
					Send(sd, recvline, strlen(recvline), 0);
#endif
				}
			}
		}

		if (correct) { /* reset Secret Word */
			get_secret_word(dict);
			printf("Reset secret word: %s, secret length: %d\n", Secret, Secret_length);
		}

	}

	for (i = 0; i != Dict_size; i++) {
		free(dict[i]);
	}
	free(dict);
	disconnect_all();
	free(user_pool);
	free(Secret);
	free(file);
	return EXIT_SUCCESS;
}

/* read in the dictionary from filename and store words into an array of char arrays*/
char** read_dict(char* filename, int longest) {
	FILE *fp;
	int memsize = MAX_LENGTH*8;
	char tmp[MAX_LENGTH];
	char **strlines = (char**)calloc(memsize,sizeof(char*));

	if ((fp = fopen(filename,"r")) == NULL) {
		printf("OPEN FILE ERROR.\n");
	}

	while( fgets(tmp,MAX_LENGTH,fp) ) {
		if (Dict_size >= memsize) {
			memsize *= 2;
			strlines = (char**)realloc(strlines,memsize*sizeof(char*));
		}

		strlines[Dict_size] = (char*) calloc(longest+1, sizeof(char));

		strcpy(strlines[Dict_size], tmp);
		
		Dict_size++;
	}

	printf("dict size: %d\n", Dict_size);
	fclose(fp);

	return strlines;
}

/* get a new secre_word */
void get_secret_word(char** dict) {
	memset(Secret, 0, MAX_LENGTH);

  	int randnum = rand() % Dict_size; /* randomization */

  	int i;

  	for(i = 0; i < strlen(dict[randnum])-1; i++) { /* eliminate the \n */
    	Secret[i] = dict[randnum][i];
  	}

  	Secret_length = strlen(Secret);
}

int find_index(int sd) {
	int i;
	for ( i = 0; i < N; i++ ) {
		if (sd == user_pool[i].sd) {
			/* printf("index: %d\n", i); */
			return i;
		}
	}
	return -1;
}

/* return 1 if client has set a valid username; return 0 otherwise */
int name_set(int sd) {
	int index = find_index(sd);
	if (user_pool[index].id != NULL) {
		return 1;
	}
	return 0;
}

/* return lower case str */
char* to_lower(char* str) {
	char* ret = calloc(strlen(str)+1, sizeof(char));
	int i = 0;
	while (str[i]) {
		ret[i] = tolower(str[i]);
		i++;
	}
	return ret;
}

/* check if username is valid (i.e. NOT already exist), if so, add the username into sd's struct */
int valid_username(char* new_user, int index) {
	char* lower = to_lower(new_user);
	int i;
	for (i = 0; i < N; i++) {
		printf("comparing with sd %d\n", user_pool[i].sd);
		if ( user_pool[i].lower_id != NULL && strcmp(lower, user_pool[i].lower_id) == 0 ) {
			printf("Username already exists\n");
			free(lower); 	/* if username already exists */
			return 0;
		}
	}
	/* valid username verified */
	user_pool[index].lower_id = lower;
	user_pool[index].id = calloc(strlen(lower)+1, sizeof(char));
	strcpy(user_pool[index].id, new_user);
	printf("id: %s, lower_id: %s\n", user_pool[index].id, 
			user_pool[index].lower_id);
	return 1;
}

/* disconnect client at sd */
void disconnect(int sd) {
	int i;
	for (i = 0; i < N; i++) {
		if (user_pool[i].sd == sd) {
			/* remove the user */
			free(user_pool[i].id);
			free(user_pool[i].lower_id);
			close(user_pool[i].sd);
			/* copy the last user's info to current position 
				if the removed user is NOT the last one in the pool*/
			if (i < N-1) {
				printf("called move last\n");
				user_pool[i].id = user_pool[N-1].id;
				user_pool[i].lower_id = user_pool[N-1].lower_id;
				user_pool[i].sd = user_pool[N-1].sd;
			}
			user_pool[N-1].id = NULL;
			user_pool[N-1].lower_id = NULL;
			user_pool[N-1].sd = 0;
			N--;
			break;
		}
	}
	printf("disconnect %d, N: %d\n", sd, N);
}

/* recv from client and trying to set username */
void get_uername(int sd) {
	char* sendline = calloc(MAX_LENGTH, sizeof(char));
	char* recvline = calloc(MAX_LENGTH, sizeof(char));

	if (Recv(sd, recvline, 1024, 0) == 0) { /* client disconnected */
		printf("Client disconnected, closing sd %d\n", sd);
		free(sendline); free(recvline);
		close(sd); /* NO need to modify user_pool since the user has NOT yet been added */
		return;
	}
	recvline[strlen(recvline)-1] = '\0';

	int valid = valid_username( recvline, find_index(sd) );

	if ( !valid ) {
		snprintf(sendline, MAX_LENGTH, "Username %s is already taken, "
							"please enter a different username\n", recvline);
		Send(sd, sendline, strlen(sendline), 0);
	} else {
		snprintf(sendline, MAX_LENGTH, "Let's start playing, %s\n", recvline);
		Send(sd, sendline, strlen(sendline), 0);
		snprintf(sendline, MAX_LENGTH, "There are %d player(s) playing. The secret word "
										"is %ld letter(s).\n", N, strlen(Secret));
		Send(sd, sendline, strlen(sendline), 0);
	}
	free(sendline);
	free(recvline);
}

/* return 1 if the secret word was guessed correctly */
int get_answer(int sd) {
	char* sendline = calloc(MAX_LENGTH, sizeof(char));
	char* recvline = calloc(MAX_LENGTH, sizeof(char));

	if (Recv(sd, recvline, MAX_LENGTH, 0) == 0) { /* client disconnected */
		printf("Client disconnected, closing sd %d\n", sd);
		free(sendline); free(recvline);
		disconnect(sd); /* modify user_pool */
		close(sd);
		return 0;
	}
	recvline[strlen(recvline)-1] = '\0'; /* replace \n with \0 */

	if (strlen(recvline) != Secret_length) {
		snprintf(sendline, MAX_LENGTH, "Invalid guess length. "
					"The secret word is %d letter(s).\n", Secret_length);
		Send(sd, sendline, strlen(sendline), 0);
		free(sendline); free(recvline);
		return 0;
	}

	printf("%s\n", recvline);
	int n1 = get_correct_letter(recvline);
	int n2 = get_correct_place(recvline);
	printf("letter: %d, place: %d\n", n1, n2);
	int correct;
	if ( guessed_correctly(n1, n2) ) {
		/* all connected users should receive the message Z has correctly 
		guessed the word S, and then all users should be disconnected from the server. */
		send_all_and_disconnect(sd);
		correct = 1;
	} else {
		/*send a message to all connected users in the format: 
		Z guessed G: X letter(s) were correct and Y letter(s) were correctly placed.*/
		send_all(sd, recvline, n1, n2);
		correct = 0;
	}

	free(sendline); free(recvline);
	return correct;
}

void send_all(int sd, char* guessed, int letter, int place) {
	int who = find_index(sd);
	char* sendline = calloc(MAX_LENGTH, sizeof(char));
	snprintf(sendline, MAX_LENGTH, "%s guessed %s: %d letter(s) were correct "
				"and %d letter(s) were correctly placed.\n", user_pool[who].id, 
				guessed, letter, place);
	int i;
	for (i = 0; i != N; i++) {
		Send(user_pool[i].sd, sendline, strlen(sendline), 0);
	}
	free(sendline);
}

void disconnect_all() {
	int i;
	for (i = 0; i != N; i++) {
		free(user_pool[i].id);
		free(user_pool[i].lower_id);
		printf("closing %d\n", user_pool[i].sd);
		close(user_pool[i].sd);
		user_pool[i].id = NULL;
		user_pool[i].lower_id = NULL;
		user_pool[i].sd = 0;
	}
	N = 0;
}

void send_all_and_disconnect(int sd) {
	int winner = find_index(sd);

	char* sendline = calloc(MAX_LENGTH, sizeof(char));
	snprintf(sendline, MAX_LENGTH, "%s has correctly guessed the word %s\n", 
				user_pool[winner].id, Secret);
	int i;
	for (i = 0; i != N; i++) {
		Send(user_pool[i].sd, sendline, strlen(sendline), 0);
	}
	free(sendline);
	disconnect_all();
}

/* return the number of correct letters */
int get_correct_letter(char* word) {
	// Huanzhi's edit
	int i, j;
	int correct_letter = 0;
	int correct_place[Secret_length];
	for (i = 0; i < Secret_length; i++) {
		correct_place[i] = 0;
	} 
	char* low_secret = to_lower(Secret);
	char* low_word = to_lower(word);
	for (i = 0; i < Secret_length; i++) {
		for (j = 0; j < Secret_length; j++) {
			if ((low_word[i] == low_secret[j]) && (correct_place[j] == 0)) {
				correct_place[j] = 1;
				correct_letter++;
				break;
			}
		} 
	}
	free(low_secret); free(low_word);
	return correct_letter;
}
/* return the number of correct places */
int get_correct_place(char* word) {
	// Huanzhi's edit
	int i;
	int correct_place = 0;
	char* low_secret = to_lower(Secret);
	char* low_word = to_lower(word);
	for (i = 0; i < Secret_length; i++) {
		if (low_secret[i] == word[i]) {
			correct_place++;
		} 
	}
	free(low_secret); free(low_word);
	return correct_place;
}

/* return 1 if all letters matches and all letters place in the correct place */
int guessed_correctly(int correct_letter, int correct_place) {
	return (correct_letter == Secret_length ) && 
			(correct_place == Secret_length );
}