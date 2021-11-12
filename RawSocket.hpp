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

const uint16_t MAXLINE = 1500;

class RawSocket{
    private:
        int rawSocket;
        char datagram[MAXLINE];
        uint16_t datagramSize = 0;
        uint16_t datalen = 0;

        struct ip ip_header;                // IP Header
        struct udphdr udp_header;           // UDP Header

        struct in_addr src_ip;              // Source IP
        struct in_addr dst_ip;              // Destination IP

        struct sockaddr_in destaddr;        // sendto() destination 

        __int32_t broadcastOption = 0;
        __int8_t shouldConstruct = 0;

    public:
        RawSocket(){
            // Constructing the raw socket
            rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
            if(rawSocket == -1) {
                perror("ERROR: failed to create raw socket\n");
                exit(-1);
            }
            
            memset(&destaddr, 0, sizeof(destaddr));
            memset(&ip_header, 0, sizeof(ip_header));
            memset(&udp_header, 0, sizeof(udp_header));

            // IP Header
            ip_header.ip_hl = sizeof(struct ip) / 4; // Header length is size of header in 32bit words, always 5.
            ip_header.ip_v = 4;						 // IPv4
            ip_header.ip_tos = 0; 					 // Type of service, See RFC for explanation.
            
            ip_header.ip_id = 0; 					 // Can be incremented each time by setting datagram[4] to an unsigned short.
            ip_header.ip_off = 0;					 // Fragment offset, see RFC for explanation.
            ip_header.ip_ttl = IPDEFTTL;			 // Time to live, default 60.
            ip_header.ip_p = IPPROTO_UDP;			 // Using UDP protocol.
            ip_header.ip_sum = 0;				     // Checksum, set by kernel.

            // UDP Header
            udp_header.uh_sum = 0; 

            // sendto() destination 
            destaddr.sin_family = AF_INET;
        }

        void setBroadcastOption(__int32_t option){
            broadcastOption = option;
            if (setsockopt(rawSocket, SOL_SOCKET, SO_BROADCAST, &broadcastOption, sizeof(broadcastOption)) == -1){
                perror("ERROR: failed set the broadcast option to raw socket\n");
            }
        }

        __int32_t getBroadcastOption(){
            return broadcastOption;
        }

        void setDestination(char *destAddr, uint16_t destPort){
            shouldConstruct = 1;
            
            // IP Header
            dst_ip.s_addr = inet_addr(destAddr);
            ip_header.ip_dst = dst_ip;

            // UDP Header
            udp_header.uh_dport = htons(destPort); 

            // sendto() destination 
            destaddr.sin_port = htons(destPort);
            destaddr.sin_addr.s_addr = inet_addr(destAddr);

        }

        void setSource(char *sourceAddr, uint16_t sourcePort){
            shouldConstruct = 1;

            // IP Header
            src_ip.s_addr = inet_addr(sourceAddr);
            ip_header.ip_src = src_ip;

            // UDP Header
            udp_header.uh_sport = htons(sourcePort);  
        }

        void setData(char *data, uint16_t len){
            shouldConstruct = 1;

            // Copying at the end of datagram
            datalen = len;
            memcpy(datagram + MAXLINE - datalen, data, datalen);
        }

        void constructDatagram(){

            // IP Header
            ip_header.ip_len = htons(sizeof(struct ip) + sizeof(struct udphdr) + datalen);

            // UDP Header
            udp_header.uh_ulen = htons(sizeof(struct udphdr) + datalen); 

            // Construct datagram
            datagramSize = sizeof(struct ip) + sizeof(struct udphdr) + datalen;
            memcpy(datagram, &ip_header, sizeof(struct ip));
            memcpy(datagram + sizeof(struct ip), &udp_header, sizeof(struct udphdr));
            memcpy(datagram + sizeof(struct ip)+sizeof(struct udphdr), datagram + MAXLINE - datalen, datalen);

            shouldConstruct = 0;
        }

        void sendDatagram(){
            if (shouldConstruct == 1)
                constructDatagram();

            if (sendto(rawSocket, datagram, datagramSize, 0,(struct sockaddr*)&destaddr, sizeof(destaddr)) == -1)
                perror("ERROR: failed to send the datagram using the raw socket\n");
        }
    
};
