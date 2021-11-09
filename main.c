#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

const unsigned char SERVER_MESSAGE[] = "Server~Open~1~";
const uint8_t FIRST_SPECIAL_BYTE = 4;
const uint8_t SECOND_SPECIAL_BYTE = 2;
const uint16_t DESTINATION_PORT = 47777;
const uint16_t SOURCE_PORT = 56947;

void signalCatch(int socket);

int main(int argc, char** argv) {
    // Ensure proper usage
	__int8_t sourceIpNo = 2;
    __int8_t destIpNo = 4;
    switch (argc){
        case 1:{
            printf("Usage: -s SourceIp -d DestinationIp\n");
            return 0;
        }

        case 5:{
            __int8_t verif = 0;
            for (__int8_t i=1; i<argc; i++){
                if (strcmp("-s", argv[i]) == 0){
                    sourceIpNo = i+1;
                    verif += 2;
                }
                if (strcmp("-d", argv[i]) == 0){
                    destIpNo = i+1;
                    verif += 3;
                }
            }

            if (((sourceIpNo - destIpNo) == 2 || (destIpNo - sourceIpNo) == 2) && verif == 5)
                break;
        }

        default:{
            printf("Incorrect usage\n");
            return 0;
        }

    }

	// Configuration
	char* dest_addr = argv[destIpNo];
	uint16_t dest_port = DESTINATION_PORT;
	char* src_addr = argv[sourceIpNo];
	uint16_t src_port = SOURCE_PORT;

	// uint16_t packet_size = 1500; // max is your wifi, eth or loopback device's MTU.

	// Create raw socket (requires root uid)
	int rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if(rawSocket == -1) {
		perror("ERROR: failed to create raw socket\n");
		return -1;
	}

    // Capture ctrl+c
    signal(SIGINT, signalCatch);

    // Calculate data length and create data buffer
    
    //packet_size - ( sizeof(struct ip) + sizeof(struct udphdr)); // sizeof() will always be 20 & 8 respectively.
	unsigned char data[2 + strlen(SERVER_MESSAGE)];
    data[0] = FIRST_SPECIAL_BYTE;
    data[1] = SECOND_SPECIAL_BYTE;
	memcpy(data + 2, SERVER_MESSAGE, strlen(SERVER_MESSAGE));
	uint16_t datalen = strlen(data);

	/*
    if(packet_size < sizeof(struct ip) + sizeof(struct udphdr) + 1) {
        fprintf(stderr, "ERROR: packet_size must be >= 29\n");
        return -1;
    }
	*/

    // IP header
	struct ip ip_header;
	ip_header.ip_hl = sizeof(struct ip) / 4; // Header length is size of header in 32bit words, always 5.
	ip_header.ip_v = 4;						 // IPv4
	ip_header.ip_tos = 0; 					 // Type of service, See RFC for explanation.
	ip_header.ip_len = htons(sizeof(struct ip) + sizeof(struct udphdr) + datalen);
	ip_header.ip_id = 0; 					 // Can be incremented each time by setting datagram[4] to an unsigned short.
	ip_header.ip_off = 0;					 // Fragment offset, see RFC for explanation.
	ip_header.ip_ttl = IPDEFTTL;			 // Time to live, default 60.
	ip_header.ip_p = IPPROTO_UDP;			 // Using UDP protocol.
	ip_header.ip_sum = 0;				     // Checksum, set by kernel.

	// Source IP
	struct in_addr src_ip;
	src_ip.s_addr = inet_addr(src_addr);
	ip_header.ip_src = src_ip;

	// Destination IP
	struct in_addr dst_ip;
	dst_ip.s_addr = inet_addr(dest_addr);
	ip_header.ip_dst = dst_ip;

    // UDP Header
	struct udphdr udp_header;
	udp_header.uh_sport = htons(src_port);                          // Source port.
	udp_header.uh_dport = htons(dest_port);                         // Destination port.
	udp_header.uh_ulen = htons(sizeof(struct udphdr) + datalen);    // Length of data + udp header length.
	udp_header.uh_sum = 0;                                          // udp checksum (not set by us or kernel).

    // Construct datagram
	int datagram_size = sizeof(struct ip) + sizeof(struct udphdr) + datalen;
	unsigned char datagram[datagram_size];
	memcpy(datagram, &ip_header, sizeof(struct ip));
	memcpy(datagram+sizeof(struct ip), &udp_header, sizeof(struct udphdr));
    memcpy(datagram+sizeof(struct ip)+sizeof(struct udphdr), data, datalen);


    // sendto() destination 
	struct sockaddr_in destaddr;
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(dest_port);
	destaddr.sin_addr.s_addr = inet_addr(dest_addr);

    // Send until SIGTERM with 1 second between datagrams
	printf("Sending packets...\n");
	for(;;) {
		sendto(rawSocket, datagram, datagram_size, 0,(struct sockaddr*)&destaddr, sizeof(destaddr));
        sleep(1);
	}
}

void signalCatch(int socket) {
    printf("\nClosing socket.\n");
    close(socket);
    exit(0);
}
