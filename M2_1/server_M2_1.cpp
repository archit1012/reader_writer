/*

 * Multiple Process , Multiple Thread , Solution with named Semaphore (same as server_M1_2 but with named semaphore)
 * 
 * Remaining : Implementing concurrent write for different Key
 * 
 * 
 * 
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <csignal>
#include <cstring>
#include <typeinfo>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>

using namespace std;

//lock before writting to file and unlock after finish , All threads need to check before reading that file_mutex is unlocked
//pthread_mutex_t write_file_mutex = PTHREAD_MUTEX_INITIALIZER; 	
//pthread_mutex_t read_file_mutex = PTHREAD_MUTEX_INITIALIZER; 	
int  thread_no = 0 ;
int MAX_THREAD = 100;

int  writer_counter = 0;
int  reader_counter = 0;

int is_write_available = 0 ;
int is_read_available = 0;
sem_t mutex ; 	
sem_t resource ;

void *serveRequest (void *sock)	 {		
   long sock_fd = (long)sock;	
   int n,i;
   char buffer[256];
   
   while(1)
   {
		bzero(buffer,256);
		n = read(sock_fd,buffer,255);
		
		//buffer input "GET 6532"  OR "PUT 6534 NewValue"
		if (n < 0) {
			perror("ERROR reading from socket");
			exit(1);
		}

		//printf("Here is the message: %s\n",buffer);
		//Convert into to proper format
		string s = buffer;			
		s.erase( s.size() - 1 );			
		s= s+" ";
		string delimiter = " ";
		string token = s.substr(0, s.find(delimiter)); 
		
		size_t pos = 0;
		string token1[3];
		i = 0 ; 
		while ((pos = s.find(delimiter)) != std::string::npos) {
			token = s.substr(0, pos);			
			token1[i]=token;
			s.erase(0, pos + delimiter.length());
			i++;
		}

		if( token1[0].compare("GET") == 0 )
		{	//logic to search in file and send that to client 
						
			sem_wait(&mutex); //decreaments mutex by 1 if > 0 , if  0 before decrementing  thread will block
			//wait on mutex as we want to access writer_counter
			if (writer_counter > 0 || reader_counter == 0) // either we have writer or no reader is there give chance to writer
			{
				sem_post(&mutex);				// writer_counter access over				
				sem_wait(&resource)	; 	//wait for resource as counter > 0				
			}
			reader_counter++;
			sem_post(&mutex); // if writer = 0
						
			
			ifstream inFile("airport.csv");			
			if(!inFile) {
				cout << "Cannot open input file.\n";
				//return 1;
			}
			
			string a,b;
			string str_key = token1[1];			
			int flag_found = 0 ; 			
			while (inFile >> a >> b)
			{				
				if(str_key.compare(a) == 0 )	//If Match found send to client 
				{
					n = write(sock_fd,b.c_str(),b.length());
					flag_found = 1;
				}				
			}					
			if (flag_found == 0 ){				
				n = send(sock_fd,"Not Found",9, 0);
			}
			inFile.close();					
			
			sem_wait(&mutex);
			reader_counter--;
			if (reader_counter == 0)
				sem_post(&resource);
			sem_post(&mutex);		
			
		}
		else if( token1[0].compare("PUT") == 0 )
		{   // Extract line to append in a file			
			
			
			sem_wait(&mutex);
			writer_counter++;
			sem_post(&mutex);
			
			sem_wait(&resource);	
			
			ofstream opFile;
			opFile.open("airport.csv",std::ofstream::out | std::ofstream::app);
			opFile <<token1[1]+" "+token1[2] <<endl;
			opFile.close();				
			
			sem_wait(&mutex);
			writer_counter--;
			sem_post(&mutex);
			sem_post(&resource);
			
			n = write(sock_fd,"Entry Added",11);
		}
		else if (*buffer == '#' )
		{	   				
			n = send(sock_fd, "bye bye", 7, 0);	//n = write(sock,"bye bye",7);
			printf("Bye Bye : \n");
			close(sock_fd);
			thread_no--;
			return NULL;	
		}
		if (n < 0) {
			perror("ERROR writing to socket");
			exit(1);
		}
	}
	return NULL;	
}



int main(int argc , char *argv[]  )
{
	if(argc < 3  )
	{
		cout<<"Enter Valid Command"<<endl;
		exit(0);
	}	
		
	sem_init(&mutex, 1, 1);	//initializing named semaphore - as it is multi process we need shared memory, creating in parent will share acroos all process and its thread
	sem_init(&resource, 1, 1);
	
	int no_of_ports = atoi(argv[1]) ;	
	int port_arg = atoi(argv[2])  ;
	
	for ( int cnt = 0 ; cnt < no_of_ports ;  cnt++)
	{		 	
		int portno = port_arg + cnt ;		
		int pid = fork();		
		if(pid < 0)
		{
			printf("Error on Fork \n");
		}
		if (pid == 0) 
		{
			
			int sockfd;
			int newsockfd;	
			socklen_t clilen ;	
			struct sockaddr_in serv_addr, cli_addr;
			int  rc = 0;

			// call to socket() 
			sockfd = socket(AF_INET,SOCK_STREAM,0);

			if (sockfd < 0)	{
				printf("ERROR opening socket" );					exit(1);
			}

			//socket structure 
			bzero((char * ) &serv_addr , sizeof(serv_addr)	) ;				
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_addr.s_addr = INADDR_ANY;
			serv_addr.sin_port = htons(portno);

			//  bind  host address		
			cout<<" sockfd " <<sockfd<<"  portno :"<<portno<<endl;
			if( bind(sockfd , (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0 ) {
				close(sockfd);
				printf("ERROR on binding" );	    	exit(1);
			}
			
			//start listening for the clients
			listen(sockfd , 5);
			clilen = sizeof(cli_addr) ;
			printf("listening for clients request on port portno \n" );			
			pthread_t threads[MAX_THREAD];
			
			while(1){
				
				while (thread_no > MAX_THREAD ){
					sleep(3);
				}
				
				newsockfd = accept(sockfd , (struct sockaddr *) &cli_addr , &clilen) ;    		
				if (newsockfd < 0) {
					printf("ERROR on accept");	        exit(1);
				}
				//Create Thread to handle request saparately
				rc = pthread_create( &threads[thread_no] , NULL ,  serveRequest ,(void *)newsockfd ) ;
				if( rc ){
					printf("could not create thread");
					return 1;
				}
				else{
					thread_no++;
				}		
			}
			//exit(0);
		}

	}
	return 0;
}

