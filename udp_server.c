 /**************************
 *      server.c
 *          
 *      Short Desciption: This is the server.c program. The file
 *      	  takes the host number as an argument. It waits for 
 *                the client to send out a signal to connect, and
 *                once they are connected, the server receives the
 *                name of the output file. Once it opens the output file,
 *                it then begins to write the data that is being
 *                read from the input file on the client side and send
 *                an ACK that signals for the next packet to be sent.
 *                However, if the checksum is corrupt, then it will 
 *                not write the data in the output file and instead
 *                send an ACK that signals for the packet to be resent.
 *            
 * ***************************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

// packet structure
typedef struct {
        int seq_ack;
        int length;
        int checksum;
        char data[10];
} PACKET;

// calculate checksum, set checksum to 0 before calculating because
// 	that was the checksum before it was calculated on the client side
int do_checksum(PACKET packet, int packetSize) {
        packet.checksum = 0;
        int ch_sum;
	char *ptr = (char *)&packet;
        ch_sum = *ptr++;

        int i;
        for (i = 0; i < packetSize; i++) {
                ch_sum ^= *ptr;
                ch_sum++;
                ptr++;
        }
	printf("checksum: %d\n", ch_sum);
        return ch_sum;
}

// MAIN
int main (int argc, char *argv[])
{
	// initializes many objects
	int sock, nBytes, state;
	char buffer[10];
	struct sockaddr_in serverAddr, clientAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;
	int i;
	FILE *dest;

	// sends an error for incorrent inputs
    	if (argc != 2)
    	{
        	printf ("need the port number\n");
        	return 1;
    	}

	// init 
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons ((short)atoi (argv[1]));
	serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	memset ((char *)serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));  
	addr_size = sizeof (serverStorage);

	// create socket
	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf ("socket error\n");
		return 1;
	}

	// bind
	if (bind (sock, (struct sockaddr *)&serverAddr, sizeof (serverAddr)) != 0)
	{
		printf ("bind error\n");
		return 1;
	}

	// create packets and set state
	state = 0;
	PACKET rcv_packet;
	PACKET sig_packet;
		
	//receive output file name
	recvfrom(sock, &rcv_packet, 106, 0, (struct sockaddr *)&serverStorage, &addr_size);
	if (rcv_packet.checksum == do_checksum(rcv_packet, rcv_packet.length))
		dest = fopen(rcv_packet.data, "wb" );

	// receive  datagrams
	while (1)
	{	
		nBytes = recvfrom (sock, &rcv_packet, 106, 0, (struct sockaddr *)&serverStorage, &addr_size);
		// if there is 0 bytes send over, break from the loop
		if (nBytes == 0)
                        break;

		// if the checksum works out, then it writes the data, updates the state, and sends a good ACK
		if (rcv_packet.checksum == do_checksum(rcv_packet, rcv_packet.length)) 
		{
			sig_packet.seq_ack = (rcv_packet.seq_ack == 0) ? 0 : 1;
                        state = (rcv_packet.seq_ack == 0) ? 1 : 0;
			fwrite(rcv_packet.data, 1, rcv_packet.length, dest);
			sendto(sock, &sig_packet, sizeof(sig_packet), 0, (struct sockaddr *)&serverStorage, addr_size);
                }	
		// if the checksum doesn't match, then it sends a bad ACK. state remains the same
		else 
		{ 
                        sig_packet.seq_ack = (rcv_packet.seq_ack == 0) ? 1 : 0;
                        sendto(sock, &sig_packet, sizeof(sig_packet), 0, (struct sockaddr *)&serverStorage, addr_size);
                }
	}

	fclose(dest);
	return 0;
}
