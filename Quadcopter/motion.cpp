#define throttle 3 
#define roll 1
#define pitch 2
#define yaw 4

#define throttleZero 1000  
#define pitchZero 1465
#define rollZero 1472
#define yawZero 1483

#define throttleMax 100
#define pitchMax 170
#define rollMax 170
#define yawMax 170

#include <iostream>
#include <stdio.h>
#include <cstring>
#include <string>
#include "PracticalSocket.h" // For UDPSocket and SocketException
#include <cstdlib>           // For atoi()

#define BUF_LEN 65540 // Larger than maximum UDP packet size

using namespace std;

void react(string cmd){

	cout<<cmd<<endl;
}

int main(int argc, char* argv[]){

 	
   if (argc != 2) { // Test for correct number of parameters
        cerr << "Usage: " << argv[0] << " <Server Port>" << endl;
        exit(1);
    }

    string sourceAddress; // Address of datagram source
    unsigned short sourcePort; // Port of datagram source

    unsigned short servPort = atoi(argv[1]); // First arg:  local port
    char msg[100];
    try {
        UDPSocket sock(servPort);

        while (1) {
            
            sock.recvFrom( &msg[0], 100, sourceAddress, sourcePort);
       		
       		react(msg);
        }
    } catch (SocketException & e) {
        cerr << e.what() << endl;
        exit(1);
    }

 	return 0;

}
