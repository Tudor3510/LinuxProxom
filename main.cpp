#include "RawSocket.hpp"

const char SERVER_MESSAGE[] = "\x04\x02Server~Open~1~";
const uint16_t DESTINATION_PORT = 47777;
const uint16_t SOURCE_PORT = 56947;

const __int32_t BROADCAST_OPTION = 1;

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

    signal(SIGINT, signalCatch);


    RawSocket rawSocket;
    rawSocket.setBroadcastOption(BROADCAST_OPTION);
    rawSocket.setSource(argv[sourceIpNo], SOURCE_PORT);
    rawSocket.setDestination(argv[destIpNo], DESTINATION_PORT);
    rawSocket.setData((char*)SERVER_MESSAGE, strlen(SERVER_MESSAGE));

	printf("Sending packets...\n");
	for(;;) {
        rawSocket.sendDatagram();
	}
	
return 0;
}

void signalCatch(int socket) {
    printf("\nClosing socket.\n");
    close(socket);
    exit(0);
}
