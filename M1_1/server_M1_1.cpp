/*

 * Single Process , Multiple Thread , Solution with busy waiting
 * 
 * 
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
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER; 	

int  thread_no = 0 ;
int MAX_THREAD = 100;

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
			pthread_mutex_lock(&file_mutex);
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
			pthread_mutex_unlock(&file_mutex);			
		}
		else if( token1[0].compare("PUT") == 0 )
		{   // Extract line to append in a file			
			pthread_mutex_lock(&file_mutex);
			ofstream opFile;
			opFile.open("airport.csv",std::ofstream::out | std::ofstream::app);
			opFile <<token1[1]+" "+token1[2] <<endl;
			opFile.close();				
			pthread_mutex_unlock(&file_mutex);
			
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
					 	
	int portno =atoi(argv[2]) ;		
	int sockfd,newsockfd;	
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
	
return 0;
}

