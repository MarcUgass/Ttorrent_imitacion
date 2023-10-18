// Trivial Torrent

// TODO: some includes here

#include "logger.h"
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdlib.h>
#include "file_io.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>



// TODO: hey!? what is this?

/** 
 * This is the magic number (already stored in network byte order).
 * See https://en.wikipedia.org/wiki/Magic_number_(programming)#In_protocols
 */
static const uint32_t MAGIC_NUMBER = 0xde1c3232; // = htonl(0x32321cde);

static const uint8_t MSG_REQUEST = 0;
static const uint8_t MSG_RESPONSE_OK = 1;
static const uint8_t MSG_RESPONSE_NA = 2;

enum { RAW_MESSAGE_SIZE = 13 };
int client (char **argv);
int server (char **argv);

/**
 * Main function.
 */
int main(int argc, char **argv) {

	set_log_level(LOG_DEBUG);

	log_printf(LOG_INFO, "Trivial Torrent (build %s %s) by %s", __DATE__, __TIME__, "Marc Ugas(1636442) and Oriol Puertas(1636385)");

	// ==========================================================================
	// Parse command line
	// ==========================================================================

	// TODO: some magical lines of code here that call other functions and do various stuff.

	// The following statements most certainly will need to be deleted at some point...
	(void) argc; //quantitat de dades d'entrada
	(void) argv; //1 o 3 dades de entrada
	(void) MAGIC_NUMBER;
	(void) MSG_REQUEST;
	(void) MSG_RESPONSE_NA;
	(void) MSG_RESPONSE_OK;
	
	if(argc == 2) {
		int rclient = client(argv);
		if (rclient == -1 ) {
			perror("Error, el client torrent no s'ha executat correctament.");
		}
	} else if(argc == 4) {
		int rserver = server(argv);
		if (rserver == -1) {
			perror("Error, el servidor torrent no s'ha executat correctament.");
		}
	} else {
		perror("Error, l'entrada d'arguments no es vàlida.");
	}
	exit(0);
}

int server(char **argv) {
	
	uint16_t PORT = 0;
	int a = 0;
	struct torrent_t torrent;
	assert(strlen(argv[2]) == 4);
	assert(strlen(argv[3]) > 9);

	while (argv[2][a] != '\0') { // argv[2] PORT, 8080, esta en char ho passem a int
		PORT = (uint16_t) (PORT * 10 + (argv[2][a] - '0')); //multipliquem per 10, n * 10^i
		a++;
	}

	char file[strlen(argv[3])-8]; //fem el mateix que el client
	strncpy(file, argv[3], strlen(argv[3])-9); 
	file[strlen(argv[3])-9] = '\0';

	int c = create_torrent_from_metainfo_file(argv[3], (struct torrent_t*) &torrent, (const char *) file);
	if (c == -1) {
		perror("Error, la creació del torrent no es possible");
		return -1;
	}
	
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	
	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(PORT); //port d'entrada
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY); //apunta a qualsevol adreça

	//Fem el bind i el listen per poder poder escoltaara les peticions del client
	if ( bind(sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) == -1) {
		perror("Error, no s'ha pogut executar correctament el bind");
		return -1;
	}
	
	if ( listen(sock, SOMAXCONN) == -1) { //S'utilitza SOMAXCONN ja que és un valor molt gran, per determinar el vaalor màxim de connexions del socket
		perror("Error, no s'ha pogut executar correctament el listen");
		return -1;
	}
	
	while (1) {
		socklen_t socklen = sizeof(sockaddr);
		int sock2 = accept(sock, (struct sockaddr *) &sockaddr, &socklen);
		
		int child = 0;
		if ((child = fork()) == 0) {  //Creem el procés fill
			
			int c1 = close(sock); //tanquem el socket principal, per no tenir dos oberts a la vegada
			if (c1 == -1) {
				perror("Error, no es pot tancar el socket principal");
				continue;
			}
			
			int error = 0;
			uint8_t payload[13];
			ssize_t r = 0;
			while ((r = recv(sock2, payload, RAW_MESSAGE_SIZE, MSG_WAITALL)) > 0) { 
				if (r == -1) {
					perror("Error, el servidor no ha rebut les dades correctament.");
					error = 1;
				}
								
				struct block_t block;
				uint64_t number = 0;
				for (int i = 5; i < RAW_MESSAGE_SIZE; i++) { //Omplim la variable number de 64 bytes amb 8 valors de 8 bytes(payload)
					number |= (number << 8);
					number |= payload[i];
				}
				int lb = load_block((struct torrent_t *) &torrent, number, (struct block_t *) &block);
				if (lb == -1) {
					perror("Error,no s'ha pogut carregar el bloc correctament.");
					error = 1;
				}
				
				if (error == 0) { //Com que no s'ha produit cap error, s'ha trobat el bloc
					
					uint8_t data[block.size+13];
					for (uint64_t i = 0; i < block.size+13; i++) {
						if (i < 13) {
						
							if (i == 4) {
								data[i] = MSG_RESPONSE_OK; 
							} else {
								data[i] = payload[i]; 
							}
							
						} else {
							data[i] = block.data[i-13]; //El bloc no conté les dades identificadores
						}
					}
					
					ssize_t s = send(sock2, (uint8_t *) data, sizeof(data), 0); 
					if (s == -1) {
						perror("Error, no s'ha pogut enviar el missatge correctament");
						continue;
					}
				} else { //Com que el bloc ha donat error, retornarem el payload buit confome no s'ha rebut el bloc 
					
					payload[4] = MSG_RESPONSE_NA;
					ssize_t s = send(sock2, (uint8_t *) payload, sizeof(payload), 0); 
					if (s == -1) {
						perror("Error, no s'ha pogut enviar el missatge correctament");
						continue;
					}
				}
			}
			int c2 = close(sock2);
			if (c2 == -1) {
				perror("Error, no s'ha pogut tancar el socket secundari");
				continue;
			}
		} else { 
			//Procés Pare
			waitpid(-1, &child, 0); //Prova fill espera
			int c2 = close(sock2);
			if (c2 == -1) {
				perror("Error, no s'ha pogut tancar el socket secundari");
				continue;
			}
		}
	}
	int d = destroy_torrent((struct torrent_t*) &torrent);
    if (d == -1) {
    	perror("Error, no s'ha pogut alliberar correctament la informació del torrent");
    	return -1;
    }
	int c1 = close(sock);
	if (c1 == -1) {
		perror("Error, no s'ha pogut tancar el socket principal");
		return -1;
	}
	return 0;
}

int client(char **argv) {
	struct torrent_t torrent;
	char file[strlen(argv[1])-8];
	strncpy(file, argv[1], strlen(argv[1])-9); //copia l'argument menys els ultims 9 caracters que son el .ttorrent
	file[strlen(argv[1])-9] = '\0';

	if (create_torrent_from_metainfo_file(argv[1], &torrent, file)) {
      		perror("Error, la creació del torrent no es possible");
		return -1;
  	}

	for (uint64_t i = 0; i < torrent.peer_count; i++) {
		
		int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP); //creació del socket
		
		struct sockaddr_in sockaddr;
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_port = torrent.peers[i].peer_port;
		sockaddr.sin_addr.s_addr = 0; //Inicilitzem tot el contingut de les adreces
		
		for ( uint64_t j = 0; j < 4; j++) {// 4 es el tamany peer address
    		uint8_t address = torrent.peers[i].peer_address[j];
    		uint32_t aux = 0;
    
    		uint32_t octet = (uint32_t)address << (8 * j); //agafem l'octet per cada adreça
    		octet += (uint32_t)aux << (8 * (j + 1));
    		sockaddr.sin_addr.s_addr += octet;
    	}
		
		int ctl_connect = connect(sock, (struct sockaddr *) &sockaddr, sizeof(struct sockaddr));
		if (ctl_connect == -1) {
			close(sock);
			perror("Error, la connexió amb el servidor no es valida");
			continue; //salta a la següent iteració
		}
		
		for (uint64_t j = 0; j < torrent.block_count; j++) {
			
			if (torrent.block_map[j] == 0) {
				 
				uint8_t payload[RAW_MESSAGE_SIZE];
				
				for (uint64_t v = 0; v < 13; v++) {
    				if (v < 4) {
    					payload[v] = (uint8_t) (MAGIC_NUMBER >> 8 * (3 - v));
    				}
    				if(v == 4) {
    					payload[v] = MSG_REQUEST;
    				}
    				if (v >= 5 && v < 13) {
    					payload[v] = (uint8_t) (j >> 8 * (12 - v));
    				}
    			}
				
				ssize_t s = send(sock, (uint8_t *) payload, RAW_MESSAGE_SIZE, 0); //s'envia el payload al servidor
				if (s == -1) {
					perror("Error, No es pot enviar el missatge correctrament");
					continue;
				}
				
				struct block_t block;
				size_t block_size = MAX_BLOCK_SIZE + 13; //13 primers bytes que hem omplert per poder diferenciar cada bloc (magic_number...)
				uint8_t data[block_size];
				ssize_t r = 0;
				
				if (j != torrent.block_count - 1) {
					r = recv(sock, (uint8_t *)&data, block_size, MSG_WAITALL);
					if (r == -1) {
						perror("Error, no s'ha rebut les dades correctament");
						continue;
					}
				} else { //L'ultim bloc no ocupa el màxim de bytes, per això no s'utilitza MSG_WAITALL, ja que entre en un bucle infinit
					r = recv(sock, (uint8_t *)&data, block_size, 0);
					if (r == -1) {
						perror("Error, no s'ha rebut les dades correctament");
						continue;
					}
				}
				
				for (int x = 0; x < r; x++) {
					block.data[x] = data[x + 13];
				}
				block.size = (uint64_t) (r - 13); //no ens interessa guardar els primers 13 bytes, ja que són els indentificadors
				if (store_block ((struct torrent_t*) &torrent, j, (struct block_t*) &block) == -1) {
					perror("Error, no es pot emmagatzemar les dades del bloc");
					continue;
				}
			}
		}
		if (close(sock) == -1) {
			perror("Error, no es pot tancar correctament el socket");
			continue;
		}
	}
	int d = destroy_torrent((struct torrent_t*) &torrent);
    if (d == -1) {
    	perror("Error, no s'ha pogut alliberar correctament la informació del torrent");
    	return -1;
    }
	return 0;
}
