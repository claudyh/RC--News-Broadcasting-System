//Projeto RC
//Cláudia Torres e Guilherme Rodrigues
 
 //gcc -pthread Final_Client.c -o news_client
 //./news_client 127.0.0.1 9000

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE    1024

typedef struct {
  struct ip_mreq mreq;
  struct sockaddr_in addr;
  socklen_t slen;
  int socket_fd;
}multicast_group;

struct ip_mreq mreq;
struct sockaddr_in addr;
struct hostent *hostPtr;
int fd, mc_fd, port, n_socket = 0;
multicast_group mc_group[100];
pthread_t multicast_thread;
int max_fd = 0;

void erro(char *msg);

void subscribeTopic(char *mc_address, int port_socket){
  //printf("EU ESTOU A CRIAR UM SOCKET\n");
  if ((mc_group[n_socket].socket_fd = socket(AF_INET, SOCK_DGRAM,0)) == -1){
    perror("socket");
  }
  //printf("Socket porta: %d\n", port_socket);

int inutil = 1;
if (setsockopt(mc_group[n_socket].socket_fd, SOL_SOCKET, SO_REUSEADDR, &inutil, sizeof(inutil)) < 0) {
    perror("setsockopt");
    exit(1);
}
  //printf("REUSEADDR SETUP\n");

  memset(&addr, 0, sizeof(addr));
  mc_group[n_socket].addr.sin_family = AF_INET;
  mc_group[n_socket].addr.sin_addr.s_addr = htonl(INADDR_ANY);
  mc_group[n_socket].addr.sin_port = htons(port_socket);

  //printf("adress: %s\n", mc_address);
   
  if (bind(mc_group[n_socket].socket_fd, (struct sockaddr *)&mc_group[n_socket].addr, sizeof(mc_group[n_socket].addr)) < 0)
  { 
    perror("multicast bind");
    exit(1);
  }
  //printf("BIND DONE\n");

  memset(&mc_group[n_socket].mreq, 0, sizeof(mc_group[n_socket].mreq));
  mc_group[n_socket].mreq.imr_multiaddr.s_addr = inet_addr(mc_address);
  mc_group[n_socket].mreq.imr_interface.s_addr = INADDR_ANY;
  
  if (setsockopt(mc_group[n_socket].socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mc_group[n_socket].mreq, sizeof(mc_group[n_socket].mreq)) < 0) {
    perror("setsockopt");
    exit(1);
  }
  
  if (mc_group[n_socket].socket_fd> max_fd){
  	max_fd = mc_group[n_socket].socket_fd;
  }
  
  n_socket++;
}

void *listen_multicast()
{
  printf("ENTROUUUUU\n");
  int n;
  char buffer[1024];
  fd_set fds;
  struct timeval tv;
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  socklen_t slen = sizeof(struct sockaddr_in);
  while (1)
  {
    memset(buffer, 0, sizeof(buffer));
    FD_ZERO(&fds);
    for (int i = 0; i < n_socket; i++){
      FD_SET(mc_group[i].socket_fd, &fds);
    }
    select(max_fd + 1, &fds, NULL, NULL, &tv);
    for (int i = 0; i < n_socket; i++)
    {
      if (FD_ISSET(mc_group[i].socket_fd, &fds))
      {
      	printf("meh\n");
        if ((n = recvfrom(mc_group[i].socket_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&mc_group[i].addr, &slen)) < 0)
        {
          perror("multicast recvfrom");
        }
        printf("Received multicast message: %s\n", buffer);
      }
    }
  }
  pthread_exit(NULL);
}

void menu(int fd){
	char buffer[BUF_SIZE];
	int nread = 0;
	
	//read menu
	if((nread = read(fd, buffer, sizeof(buffer))) <= 0){
		perror("read");
	}
	buffer[nread] = '\0';
	printf("%s", buffer);
	
	if(write(fd, "Recebido\n", sizeof("Recebido\n")) < 0){
    	perror("write");
    }
	
	
	while(1){
		if((nread = read(fd, buffer, BUF_SIZE)) <= 0){
			perror("read");
		}
		buffer[nread] = '\0';
		
		printf("%s\n", buffer);
		scanf(" %[^\n]", buffer);
		//printf("%s\n", buffer);
		
		//send user command
		if(write(fd, buffer, sizeof(buffer)) < 0){
    		perror("write");
    	}
    char* token = strtok(buffer," ");
    
    if(strcmp(token, "SUBSCRIBE_TOPIC") == 0){
      char temp[1024];
      memset(&temp, 0, sizeof(temp));
      
      if((nread = read(fd, temp, sizeof(temp))) <= 0){
		    perror("read");
		  }
	  //printf("%s\n", temp);
      token = strtok(temp, ";");
      char *input[4];
      int i = 0;
      while (token != NULL) {
        input[i++] = token;
        token = strtok(NULL, ";");
      }
      subscribeTopic(input[0], atoi(input[1]));
      if(write(fd, "Subscrito\n", sizeof("Subscrito\n")) < 0){
    		perror("write");
      }
      
      //printf("\n- - - - - -\nInsert command:");
    }
    	
    	//memset(buffer, 0, sizeof(buffer));
    	//read response
    	if((nread = read(fd, buffer, sizeof(buffer))) <= 0){
			  perror("read");
		}
		buffer[nread] = '\0';
		printf("%s\n", buffer);
		//fflush(stdin);
		
		if(strcmp("\nSee you soon! :)", buffer) == 0){
			if(write(fd, "Comando Recebido\n", sizeof("Comando Recebido\n")) < 0){
    			perror("write");
    		}
			exit(0);
		}
		
		if(write(fd, "Comando Recebido\n", sizeof("Comando Recebido\n")) < 0){
    		perror("write");
    	}
	
	}
}

void login(int fd){

  printf("Welcome to the best News Server!\n");
  
  int logger = 1, nread = 0;
  char buffer[BUF_SIZE];
  //read login
  if((nread = read(fd, buffer, sizeof(buffer))) <= 0){
      perror("read");
  }
  while(logger){
  
    //LOGIN CYCLE
    buffer[nread] = '\0';
    printf("%s", buffer);
      
      //Username question
      if(strcmp("\nLOGIN\nUsername:", buffer) == 0){
        scanf("%s", buffer);
        
        //Username Insertion - for server
        if(write(fd, buffer, sizeof(buffer)) < 0){
          perror("write");
        }
        
        //Read password question
        if((nread = read(fd, buffer, sizeof(buffer))) <= 0){
          perror("read");
        }
        else{
          buffer[nread] = '\0';
          
          if(strcmp("Password:", buffer) == 0){
          	printf("%s", buffer);
            scanf("%s", buffer);
            
            
            //Password insertion - for server
            if(write(fd, buffer, sizeof(buffer)) < 0){
              perror("write");
            }
            else{
              //Check if password is correct
              if((nread = read(fd, buffer, sizeof(buffer))) <= 0){
                perror("write");
              }
              else{
              	buffer[nread] = '\0';
                printf("%s\n", buffer);   
              	
                if(strcmp("\nWelcome!\n", buffer)==0){
                  //Password successful, Login successful, redirection to menu
                  menu(fd);
                  
                }else{
                	strcpy(buffer, "ok\n");
                	
                	if(write(fd, buffer, sizeof(buffer)) < 0){
              			perror("write");
            		}
                	
                	if((nread = read(fd, buffer, sizeof(buffer))) <= 0){
          				perror("read");
        			}
                	
                	continue;
                }
              }
            }
          }
          else{
            //User does not exist, restarting login process
            printf("\nUser not found.\n");
            continue;
          }
        }
      }
      else{
        printf("DEBUGGER - %s", buffer);//!DEBUG
      }
  }
}


int main(int argc, char *argv[]) {
  char endServer[100];
  struct hostent *hostPtr;

  if (argc != 3) {
    printf("client <host> <port>\n");
    exit(-1);
  }
  
  int result;

  // Create and start the listen_multicast thread
  result = pthread_create(&multicast_thread, NULL, listen_multicast, NULL);
  if (result != 0) {
  	printf("Failed to create multicast listening thread\n");
  	return 1;
  }
  
  port = atoi(argv[2]);
  strcpy(endServer, argv[1]);
  if ((hostPtr = gethostbyname(endServer)) == 0)
    erro("Não consegui obter endereço");

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
  addr.sin_port = htons((short) atoi(argv[2]));

  if ((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
      erro("socket");
  if (connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0)
      erro("Connect");

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons((short) atoi(argv[2]));

  if ((mc_fd = socket(AF_INET, SOCK_DGRAM,0)) == -1){
    perror("socket");
  }

  if(bind(mc_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    perror("connect");

  login(fd);
  
  // Wait for the listen_multicast thread to finish
  result = pthread_join(multicast_thread, NULL);
  if (result != 0) {
    printf("Failed to join multicast listening thread\n");
    return 1;
  }
}

void erro(char *msg) {
  printf("Erro: %s\n", msg);
    exit(-1);
}
