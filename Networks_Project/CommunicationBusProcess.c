#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define bool int
#define true 1
#define false 0

//File pointer to open and close the output log file.
FILE *file_pointer;
//variable port_no is used to store the port number given by the user.
int port_no;
int sock_fd;

//an array of stations that are connected.
bool connectedStations_ar[10];
bool stations_coll_ar[10];

//exits if there is any error
void error(const char *message) 
{
	perror(message);
	exit(1);
}

//Is_there_a_Collision function checks if there is a collision
int Is_there_a_Collision(int station) 
{
	int collision_occured = 0;
	for (int i = 0; i < 10; i++) 
	{
        //if the current station is not equal to the array index
		if ((station != i))
        {
           //if the station with index number is already connected to the CommunicationBusProcess
           if(connectedStations_ar[i] == 1) 
		   {
				collision_occured = 1;
				stations_coll_ar[i] = 1;
           }
		}
	}
	return collision_occured;
}

//main function
int main(int argc, char * argv[]) 
{
    if (argc != 2) 
	{
		printf("\n Please enter : ./CommunicationBusProc <server port number>");
		exit(0);
	}
	
	port_no = atoi(argv[1]);
	file_pointer = fopen("Communication_Bus_Output.txt" , "w");
    
    if(file_pointer == NULL)
	{
		perror("Error in opening output file");
		return(-1);
    }

    //Initializing the connectedStations_ar and stations_coll_ar arrays. 
    for (int i = 0; i < 10; i++)
	{
		connectedStations_ar[i] = false;
		stations_coll_ar[i] = false;
	}
	
	int new_Sockfd;
	socklen_t client_length;
	struct sockaddr_in serv_addr, cli_addr;
	int n = 0;
	
	//Creating an internet stream TCP socket.
	sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if (sock_fd < 0)
		error("ERROR opening socket");
	
	int yes = 1;
	
	//setting the SO_REUSEADDR socket option before calling bind function.
	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
	{
		perror("setsockopt");
		exit(1);
	}
	
	//bzero function sets the entire structure to zero.
	bzero((char*) (&serv_addr), sizeof(serv_addr));
	
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	//port_no is the port number which user gives as input.
	serv_addr.sin_port = htons(port_no);
	
	if (bind(sock_fd, (struct sockaddr*) (&serv_addr), sizeof(serv_addr)) < 0)
	{
		error("ERROR on binding");
	}
	
	//the socket is converted into a listening socket with a queue of 10 where the incoming connections will be accepted by the kernel.
	listen(sock_fd, 10);

	client_length = sizeof(cli_addr);
	
	fd_set rset;
	
	//Initializes the file descriptor set to contain no file decriptors.
	FD_ZERO(&rset);
	
	while (true) 
	{    
		FD_SET(sock_fd, &rset);
	
		//waiting for connection from any station
		int nready = select(sock_fd + 1, &rset, NULL, NULL, NULL);
		
		if (FD_ISSET(sock_fd, &rset)) 
		{
			new_Sockfd = accept(sock_fd, (struct sockaddr*) (&cli_addr), &client_length);
			if (new_Sockfd < 0)
			{
				error("ERROR on accept");
			}
			
		   	printf("\nServer established connection with the client %s : %d",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port) );
			
			fflush(stdout);
			
			int childpid;
			
			if ((childpid = fork()) == 0) 
			{	
				//spawn a child process to handle the station frame part
				char buffer1[151];
				bzero(buffer1, 151);
				
				//Read from station process
				n = read(new_Sockfd, buffer1, 150);

				if (n < 0)
					error("ERROR reading from socket");

				int fromStation, toStation, frameid, partno;
				
				//frame id, part no, station from, station to are stored in the buffer1 array.
				sscanf(buffer1, "%d %d %d %d", &frameid, &partno, &fromStation,&toStation);
				
				//Log received message to bus log
				fprintf(file_pointer,"Receive part %d  of frame %d from Station %d , to Station %d \r\n", partno,frameid,fromStation,toStation);

				//Set the flag to indicate connected station
				if (partno == 1)
					connectedStations_ar[fromStation - 1] = true;

				char reply[10] = "success";
				//checks for collision 

				int collision = 0;

				if(Is_there_a_Collision(fromStation-1) == 1)
				{
					collision = 1;
                }
                if(stations_coll_ar[fromStation - 1] == 1)
				{
					collision = 1;
                }
				if (collision == 1) 
				{
					//prepare to send collision message back to station and reset flags
					strcpy(reply, "collision");
					fprintf(file_pointer,"Inform station  %d  a collision \r\n", fromStation);
				}
				else 
				{
					if (partno == 2) 
					{
						fprintf(file_pointer,"Transfer part 1 of frame   %d  from Station %d , to station %d\r\n",frameid, fromStation,toStation);
					
						//check whether there is a collision after sending first part to destination
						if(Is_there_a_Collision(fromStation-1) == 1)
				        {
							collision = 1;
                        }
                        if(stations_coll_ar[fromStation - 1] == 1)
				        {
							collision = 1;
                        }
						if (collision == 1) 
						{
							//prepare to send collision message back to station and reset flags
							strcpy(reply, "collision");
							
							fprintf(file_pointer,"Inform station  %d  a collision \r\n", fromStation);
						}
						else 
						{
							//prepare for success reply and reset flags
							fprintf(file_pointer,"Transfer part 2 of frame   %d  from Station %d , to station %d\r\n",frameid, fromStation,toStation);
						}
     				}
				}
				connectedStations_ar[fromStation - 1] = false;
				stations_coll_ar[fromStation - 1] = false;
				
				//send the reply back to station
				write(new_Sockfd, reply, sizeof(reply));
				exit(0);
			}
			//parent closes connected socket
			close(new_Sockfd);	
		}
	}
	close(sock_fd);
    fclose(file_pointer);

	return 0;
}
