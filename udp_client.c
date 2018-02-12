/*************************************************************
 *      client.c
 *      
 *      Short Description: The file accepts four arguments. 
 *      	Once it creates the connection with the server 
 *              socket, it sends the output filename over. 
 *              Then, it will begin to read the data from the 
 *              input file and send it over to the server.
 *              Using the functions sendto and recvfrom, we are
 *              allowed to send data (in packets) back and forth 
 *              one at a time. Once the client receives the ACK
 *              packet, the ACK will determine if the packet 
 *              needs to be resent, or if a packet of new info
 *              can be sent over.
 *
 *************************************************************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>

typedef struct{
        int seq_ack;
        int length;
        int checksum;
	char data[10];
} PACKET;

// calculate checksum when checksum is currently. variable r
//  produces a random number, and if it is one, then it returns
//  r as opposed to the right checksum (fake error)
int do_checksum(PACKET packet, int packetSize) {
	int r = (rand()%10);
	if (r<=1)
		return r;
	else
	{
		int ch_sum;
	        char *ptr = (char *)&packet;
        	ch_sum = *ptr++;

        	int i;
        	for (i = 0; i < packetSize; i++) {
                	ch_sum ^= *ptr;
                	ch_sum++;
                	ptr++;
       		}
        	return ch_sum;
	}
}

// MAIN
int main (int argc, char *argv[])
{
	// initializes many objects
	int sock, portNum, nBytes, state;
	char buffer[10];
	struct sockaddr_in serverAddr;
	socklen_t addr_size;
	FILE * in = fopen(argv[3],"rb");
	srand(time(NULL));

	// sends error for incorrect input
	if (argc < 3)
	{
		printf ("need the port number, ip address, in file, out file \n");
		return 1;
	}

	// configure address
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons (atoi (argv[1]));
	inet_pton (AF_INET, argv[2], &serverAddr.sin_addr.s_addr);
	memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof serverAddr;

	/*Create UDP socket*/
	sock = socket (PF_INET, SOCK_DGRAM, 0);

	// create packets and set state
	state = 0;
	PACKET send_packet;
	PACKET rcv_packet;
	int count = 0;

	// send file name to server
        send_packet.seq_ack = 0;
        strcpy(send_packet.data, argv[4]);
	send_packet.length = sizeof(send_packet);
	send_packet.checksum = do_checksum(send_packet, send_packet.length);	
	sendto(sock, &send_packet, send_packet.length, 0, (struct sockaddr *)&serverAddr, addr_size);	
	
	// reads the file and sends the data over
 	while (nBytes = fread(send_packet.data, 1, 10, in))
	{	
		// new packet content, set checksum to 0 for calc.
		send_packet.seq_ack = state;
                send_packet.length = nBytes;
        	send_packet.checksum = 0;	
	       
		int n = 1;
		while (n == 1)
		{
			// sends packet after updating checksum from 0 to a new number
			send_packet.checksum = do_checksum(send_packet, send_packet.length);
			sendto (sock, &send_packet, sizeof(send_packet), 0, (struct sockaddr *)&serverAddr, addr_size);
			
			// receive signal
			recvfrom (sock, &rcv_packet, sizeof(rcv_packet), 0, NULL, NULL);
				
			// if packet is ACK'ed, n is changed to break from current loop, and state is updated
			if (state == rcv_packet.seq_ack)
			{	
                        	state = (rcv_packet.seq_ack == 0) ? 1 : 0;
				n = 0;
                	}
			// otherwise it will resend
			else
			 	printf("It was corrupt. Resending.\n");
		}
			
	}
	// send empty signal to tell server to stop receiving
	sendto (sock, 0, 0, 0, (struct sockaddr *)&serverAddr, addr_size);
	return 0;
}
