// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823700477

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

int verbose = 0;

void print_bytes(unsigned char *bytes, int byteslen);

int main(int argc, char *argv[]) {
	// Variable Declerations
	int addressFamily;
    socklen_t addressLength;

    struct sockaddr_in ipv4RemoteAddress;
    struct sockaddr_in6 ipv6RemoteAddress;
    struct sockaddr *remoteAddress; 

    struct sockaddr_in ipv4LocalAddress;
    struct sockaddr_in6 ipv6LocalAddress;

	// User Input
	char *server = argv[1];
    char *port = argv[2];
    int level = atoi(argv[3]);
    int seed = atoi(argv[4]);

	int requestSize = 8;
	unsigned char request[requestSize]; 

	// Initialize req to 0
	bzero(request, requestSize);

	// Byte 0 = 0
	request[0] = 0;
	
	// Byte 1 = level
	request[1] = level;
	
	// Byte 2-5 = USERID
	unsigned int formattedUserID = htonl(USERID);
	memcpy(&request[2], &formattedUserID, 4);

	// Byte 6-7 = seed
	unsigned short formattedSeed = htons(seed);
	memcpy(&request[6], &formattedSeed, 2);

	// Set up addrinfo structure
    struct addrinfo hints, *result, *rp;
	int socketFD;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_DGRAM; // Use UDP

	// Get address information of the server 
	if ((getaddrinfo(server, port, &hints, &result)) < 0) {
		fprintf(stderr, "Error getting address information");
		return 2;
	}	

	// Search for suitable address and establish a connection
	addressFamily = result->ai_family; 
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		// Create a UDP socket
		socketFD = socket(addressFamily, result->ai_socktype, result->ai_protocol);

		if (socketFD == -1) continue;
		else break;
	}
	
	// No address was found
	if (rp == NULL) {
		fprintf(stderr, "No address was found.");
		exit(EXIT_FAILURE);
	}
 
	// Check if the address family is IPv4 or IPv6
	addressFamily = rp->ai_family; 
	if (addressFamily == AF_INET) {
		// Copy the server's address information to remote_addr_in structure
		memcpy(&ipv4RemoteAddress, rp->ai_addr, rp->ai_addrlen);
		remoteAddress = (struct sockaddr *)&ipv4RemoteAddress;
		addressLength = sizeof(ipv4RemoteAddress); 

		// Get local address information using getsockname function
		getsockname(socketFD, (struct sockaddr *)&ipv4LocalAddress, &addressLength);
	} else {
		// Copy the server's address information to remote_addr_in6 structure
		memcpy(&ipv6RemoteAddress, rp->ai_addr, rp->ai_addrlen);
		remoteAddress = (struct sockaddr *)&ipv6RemoteAddress;
		addressLength = sizeof(ipv6RemoteAddress); 

		// Get local address information using getsockname function
		getsockname(socketFD, (struct sockaddr *)&ipv6LocalAddress, &addressLength);
	}   

	// Send the request using the created UDP socket 
	if (sendto(socketFD, request, 8, 0, remoteAddress, addressLength) < 0) {
		fprintf(stderr, "Error sending request using UDP socket.");
		return -1;
	}

	unsigned char treasure[1024];
	int treasureLength = 0;

	while (1) {
		// Receive response from the server
		unsigned char responseBuffer[256]; 
		ssize_t bytes_received = recvfrom(socketFD, responseBuffer, sizeof(responseBuffer), 0, remoteAddress, &addressLength);
		if (bytes_received < 0) {
			fprintf(stderr, "Error reading from UDP socket.");
			return -1;
		} 

		// Byte 0: length corresponding to the value  
		unsigned char chunklen = responseBuffer[0];

		if (chunklen == 0) { break; }

		// Byte 1->n: chunk of treasure 
		char chunk[chunklen+1];
		memcpy(chunk, &responseBuffer[1], chunklen);
		chunk[chunklen] = '\0';

		// Byte n+1: This is the op-code (directions to follow to get the next chunk) 
		unsigned char opcode = responseBuffer[chunklen + 1];

		// Bytes n+2 - n+3: Op Param
		unsigned short opparam;
		memcpy(&opparam, &responseBuffer[chunklen+2], 2);
		opparam = ntohs(opparam); // "network to host short", convert the byte order from network order to host order

		// Bytes n+4 - n+7: Nonce
		unsigned int nonce; 
		memcpy(&nonce, &responseBuffer[chunklen+4], 4);
		nonce = ntohl(nonce); // "network to host long", convert the byte order from network order to host order
		// nonce += 1;

		// Populate treasure
		memcpy(treasure + treasureLength, &responseBuffer[1], chunklen);
		treasureLength += chunklen;
		treasure[treasureLength] = '\0';

		if (opcode == 4) { 
			unsigned short newPort = opparam;
            char newPortString[12];
            
			bzero(newPortString, 12);
			sprintf(newPortString, "%hu", newPort);

			// Clean Up
            freeaddrinfo(result);
			close(socketFD); 

			// Set address family
			addressFamily = (addressFamily == AF_INET) ? AF_INET6 : AF_INET;

			// Sets up hints for configuring a UDP socket
			memset(&hints, 0, sizeof(hints));
            hints.ai_family = addressFamily;
            hints.ai_socktype = SOCK_DGRAM;

			// Get address information of the server 
			if ((getaddrinfo(server, newPortString, &hints, &result)) < 0) {
				fprintf(stderr, "Error getting address information");
				return 2; 
			} 

			// Search for suitable address and establish a connection
            for (rp = result; rp != NULL; rp = rp->ai_next) {
				// Create a UDP socket
                socketFD = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
                if (socketFD == -1) continue;
                else break;
            }
            
			// No address was found
			if (rp == NULL) {
				fprintf(stderr, "No address was found.");
				exit(EXIT_FAILURE);
			}

			// Check if the address family is IPv4 or IPv6
			addressFamily = rp->ai_family; 
			if (addressFamily == AF_INET) {
				// Copy the server's address information to remote_addr_in structure
				memcpy(&ipv4RemoteAddress, rp->ai_addr, rp->ai_addrlen);
				remoteAddress = (struct sockaddr *)&ipv4RemoteAddress;
				addressLength = sizeof(ipv4RemoteAddress); 

				// Get local address information using getsockname function
				getsockname(socketFD, (struct sockaddr *)&ipv4LocalAddress, &addressLength);
			} else {
				// Copy the server's address information to remote_addr_in6 structure
				memcpy(&ipv6RemoteAddress, rp->ai_addr, rp->ai_addrlen);
				remoteAddress = (struct sockaddr *)&ipv6RemoteAddress;
				addressLength = sizeof(ipv6RemoteAddress); 

				// Get local address information using getsockname function
				getsockname(socketFD, (struct sockaddr *)&ipv6LocalAddress, &addressLength);
			}    
            
            nonce = nonce + 1;
		} else if (opcode == 3) {
		  	unsigned short messageCount = opparam; 
            unsigned int totalPort = 0;   

			// Loop through each message
            for (int i = 0; i < messageCount; i++) {
                struct sockaddr_in ipv4RemoteAddress;
                struct sockaddr_in6 ipv6RemoteAddress;

				// Get address length based on IP version
                socklen_t remoteAddressLength;
                if (addressFamily == AF_INET) {
                    remoteAddressLength = sizeof(ipv4RemoteAddress);
                } else {
                    remoteAddressLength = sizeof(ipv6RemoteAddress);
                }

				// Receive message and get port
                if (addressFamily == AF_INET) {
                    recvfrom(socketFD, NULL, 0, 0,  (struct sockaddr *)&ipv4RemoteAddress, &remoteAddressLength);
                } else {
                    recvfrom(socketFD, NULL, 0, 0, (struct sockaddr *)&ipv6RemoteAddress, &remoteAddressLength);
               	} 

				// Get port from address structure
                if (addressFamily == AF_INET) {
					totalPort += ntohs(ipv4RemoteAddress.sin_port);
                } else {
					totalPort += ntohs(ipv6RemoteAddress.sin6_port);
                }
            }

            nonce = totalPort + 1;      
		} else if (opcode == 2) {  
			// Close existing socket
			close(socketFD);  

			// Create new socket
			socketFD = socket(addressFamily, SOCK_DGRAM, 0);
			if (socketFD < 0) {
				return 1; 
			}

			struct sockaddr_in ipv4LocalAddress;
			struct sockaddr_in6 ipv6LocalAddress;

			// Set up local address based on IP version  
			if (addressFamily == AF_INET) {
				ipv4LocalAddress.sin_family = AF_INET;
				ipv4LocalAddress.sin_port = htons(opparam);
				ipv4LocalAddress.sin_addr.s_addr = 0;
			} else {
				ipv6LocalAddress.sin6_family = AF_INET6;
				ipv6LocalAddress.sin6_port = htons(opparam);
				bzero(ipv6LocalAddress.sin6_addr.s6_addr, 16); 
			}

			// Bind socket 
			if (bind(socketFD, (struct sockaddr *)&ipv4LocalAddress, sizeof(ipv4LocalAddress)) < 0) {
				perror("bind()");
				return 1;
			} 

            nonce = nonce + 1;
		} else if (opcode == 1) {
			// Set port in remote address structure 
 			if (addressFamily == AF_INET) {
				// For IPv4, set sin_port field 
                ipv4RemoteAddress.sin_port = htons(opparam);
            } else { 
				// For IPv6, set sin6_port field
                ipv6RemoteAddress.sin6_port = htons(opparam);
            }
			
            nonce = nonce + 1;
		} else if (opcode == 0) {
			nonce = nonce + 1;
		} else {
            fprintf(stderr, "Bad opcode");
            return -1;
        }

		nonce = htonl(nonce);  
		unsigned char request2[4];
		memcpy(request2, &nonce, 4);
		
		// Send the request using the created UDP socket 
		if (sendto(socketFD, request2, 4, 0, remoteAddress, addressLength) < 0) {
			fprintf(stderr, "Error sending request using UDP socket.");
			return -1;
		}
	} 

	printf("%s\n", treasure);

	// Clean Up
    freeaddrinfo(result);
    close(socketFD);
} 

void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
}