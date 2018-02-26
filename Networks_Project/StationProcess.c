#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <math.h>

#define bool int
#define true 1
#define false 0

int sock_fd;
struct sockaddr_in serv_addr;
int portno;
int time_slot_length = 100000;
int station_numb;
FILE *input_file;
FILE *outfile;

//exits when there is any error occured.
void error(const char *message)
{
	perror(message);
	exit(0);
}

//Sends a part of the frame to the Communication bus.
bool sendFrame(char *rdbuff, int part, int i) 
{
	int n;
	int frame_id, to_Station;
	char s1[10], s2[10], s3[10], s4[10];
    char reply_buffer[51];
    char write_buff[150];
    
	//reads the values.
	sscanf(rdbuff, "%s %d %s %s %s %d", s1, &frame_id, s2, s3, s4,&to_Station);
	
	//sets all the values in the array to zero in write_buff.
	bzero(write_buff, 150);
	
	//prepare the buffer to write to socket
	sprintf(write_buff, "%d %d %d %d", frame_id, part, station_numb, to_Station);
	
	//create socket descriptor sock_fd using socket function
	sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
	
	//outputs if there is an error in opening the socket.
	if (sock_fd < 0)
	{
		error("ERROR in opening socket");
    }
	
	//outputs the error if there is an error in connecting.
	if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		error("ERROR in connecting");
    }
	
	 //write to socket using socket descriptor sock_fd
	if (write(sock_fd, write_buff, strlen(write_buff)) < 0)
	{
		error("ERROR writing to socket");
    }
	
	//write log to file
	fprintf(outfile,"\nSend part  %d  of  %s \n", part,rdbuff);

	//sets zero to all values in reply_buffer array.
	bzero(reply_buffer, 51);
	
	//read from socket using socket descriptor sock_fd
	if (read(sock_fd, reply_buffer, 50) < 0)
	{
		error("ERROR reading from socket");
    }
	
	//check whether there is a collision at the bus
	if (strcmp(reply_buffer, "success") != 0) 
	{
		if (i == 16) 
		{
			error("Transmission failure after 16 attempts");
		}
		n = i > 10 ? 10 : i;
	    n = pow(2, n);
		
		//calculate the time slots to wait before the next attempt
		int time_slots = rand() % n;

        fprintf(outfile,"A collision informed, wait for %d time slots\r\n", time_slots);
		usleep(time_slots * time_slot_length);
		close(sock_fd);
		return false;
	}
	close(sock_fd);
	return true;
}

int main(int argc, char *argv[]) 
{    
	//check arguments to start the station process
	if (argc != 4) 
	{
		printf("\n Please enter in this format : ./StationProc <server name> <port> <station number>");
		exit(0);
	}
    
    char readstationip[101];
    
	//open output file and writes on it.
    outfile = fopen("Station_Process_Output.txt" , "w");
    
    if(outfile == NULL) 
	{
		perror("Error opening output file");
		return(-1);
    }
	
	//hostname store the host address.
	char * hostname = argv[1];
	
	//portno stores the port number given by the user.
	portno = atoi(argv[2]);
	
	//station_numb stores the number of the station entered by the user.
	station_numb = atoi(argv[3]);

	//open the input file based on the station number
    switch(station_numb)
	{
     case 1: 
           input_file  = fopen("station_process1.txt" , "r");    
           break;
     case 2:
           input_file  = fopen("station_process2.txt" , "r");    
           break;
     case 3:
           input_file  = fopen("station_process3.txt" , "r");    
           break;
     case 4:
           input_file  = fopen("station_process4.txt" , "r");    
           break;
     case 5:
           input_file  = fopen("station_process5.txt" , "r");    
           break;
     case 6:
           input_file  = fopen("station_process6.txt" , "r");    
           break;
     case 7:
           input_file  = fopen("station_process7.txt" , "r");    
           break;
     case 8:
           input_file  = fopen("station_process8.txt" , "r");    
           break;
     case 9:
           input_file  = fopen("station_process9.txt" , "r");    
           break;
     case 10:                      
           input_file  = fopen("station_process10.txt" , "r");    
           break;
    }
	
    //check if input file is opened without any errors.
    if(input_file == NULL) 
	{
		perror("Error opening input file");
		return(-1);
    }
	
	//sets all the values to zero.
	bzero((char *) &serv_addr, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	
	//read from input file
	while (true) 
	{
		bzero(readstationip, 101);
		if(feof(input_file))
			break;
		if(fgets(readstationip, 100,input_file)!=NULL)
		{	
			//send frame represented by the current line in the simulation file
			int i = 0;
			while (true) 
			{
				i++;
				//send first part of the frame
				if (!sendFrame(readstationip, 1, i))
					continue;
				//send second part of the frame
				if (sendFrame(readstationip, 2, i))
					break;
			}
		}
	}
	fclose(input_file);
	fclose(outfile);
	return 0;
}