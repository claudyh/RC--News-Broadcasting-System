//Projeto RC
//Cláudia Torres e Guilherme Rodrigues

//  gcc -pthread Final_Server.c -o news_server
// ./news_server 9000 1234 Config.txt


//INCLUDES -------------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>

//Shared memory
#include <sys/shm.h>

//Semaphores
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>


//CONSTS ---------------------------------------------------------------------------------------
#define BUFLEN 1024
#define MAX_USERS 100
#define MAX_TOPICS 100
#define MAX_NEWS 50


//STRUCTS --------------------------------------------------------------------------------------
typedef struct {
    int id;
    char title[256];
    //char news[MAX_NEWS][BUFLEN];
    struct sockaddr_in addr;
    char address[256];
    int port;
    int socket_fd;
} Topic;

typedef struct {
    char username[BUFLEN];
    char password[BUFLEN];
    char type[BUFLEN];
    char subscriptions[100][BUFLEN];
    int num_Subscriptions;
} User;

typedef struct {
    Topic *topics;
	User *users;
	int n_users;
	int n_topics;
} SharedMemory;

//Memória partilhada
int shmid, var_port = 9001; //id
SharedMemory *shm; //apontador

//Semáforos
sem_t *sem_users;
sem_t *sem_topics;

int saiu = 0;

//FUNCTIONS ------------------------------------------------------------------------------------

//Verifications
void erro(char *s) {
    perror(s);
    exit(1);
}

char* incrementMulticastAddress(const char* address) {
    char* newAddress = malloc(strlen(address) + 1);
    strcpy(newAddress, address);

    char* token = strtok(newAddress, ".");
    int octets[4];
    int i = 0;

    while (token != NULL && i < 4) {
        octets[i] = atoi(token);
        i++;
        token = strtok(NULL, ".");
    }

    if (i == 4) {
    if (octets[0] == 239 && octets[1] == 0 && octets[2] == 0 && octets[3] < 255) {
            octets[3]++;
    } else {
            printf("Invalid multicast address range.\n");
            free(newAddress);
            return NULL;
        }
    } else {
        printf("Invalid IP address format.\n");
        free(newAddress);
        return NULL;
    }

    sprintf(newAddress, "%d.%d.%d.%d", octets[0], octets[1], octets[2], octets[3]);
    return newAddress;
}



int verification_num(char parameter[]){
    int check=0, m=0;
    while(m<strlen(parameter)-1){
        if(isdigit(parameter[m])==0){
            check++;
            m++;
        }else{
            m++;
        }
    }
    if(check!=0){
        return 1;
    }
    else{
        return 0;
    }
}


int verification_alnum(char parameter[]){
    int check=0, m=0;
    while(m<strlen(parameter)){
        if(isalnum(parameter[m])==0){
            check++;
            m++;
        }else{
            m++;
        }
    }
    if(check!=0){
        return 1;
    }
    else{
        return 0;
    }
}


int verification_file(char parameter[]){
    char extension[4];
    char verify[] = "txt";
    int len = strlen(parameter);

    for(int i = 0; i < 3; i++){
        extension[i] = parameter[len - 3 + i];
    }
    extension[3] = '\0';

    if (strcmp(extension, verify)==0){
        return 0;
    }
    else{
        return 1;
    }

}

void createTopic(int id, char*name){

    shm->topics[shm->n_topics].id = id;
    strcpy(shm->topics[shm->n_topics].title, name);
    if(shm->n_topics == 0){
    strcpy(shm->topics[shm->n_topics].address, incrementMulticastAddress("239.0.0.0"));
    }
    else{
        strcpy(shm->topics[shm->n_topics].address, incrementMulticastAddress(shm->topics[shm->n_topics-1].address));
    }
    shm->topics[shm->n_topics].port = var_port;
    printf("PORTO: %d\n", shm->topics[shm->n_topics].port);
    var_port++;
    printf("VOU CRIAR UM SOCKET COM ESTA ADDRESS: %s\n", shm->topics[shm->n_topics].address);
    if ((shm->topics[shm->n_topics].socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&shm->topics[shm->n_topics].addr, 0, sizeof(shm->topics[shm->n_topics].addr));
    shm->topics[shm->n_topics].addr.sin_family = AF_INET;
    shm->topics[shm->n_topics].addr.sin_addr.s_addr = inet_addr(shm->topics[shm->n_topics].address);
    shm->topics[shm->n_topics].addr.sin_port = htons(shm->topics[shm->n_topics].port);

    int enable = 10;
    if (setsockopt(shm->topics[shm->n_topics].socket_fd, IPPROTO_IP, IP_MULTICAST_TTL, &enable, sizeof(enable)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    shm->n_topics++;
}



//Admin menu
void admin_menu(int socket, struct sockaddr_in cliente, socklen_t addrlen){
    
    char buffer_resposta[2050];
    int log=1;
    
    //Print menu
    if(sendto(socket, "\nADMIN MENU\n- - - - - -\nAvailable operations and commands:\n- Add an user: ADD_USER {username} {password} {administrador/leitor/jornalista}\n- Delete an user: DEL {username}\n- List all users: LIST\n- Leave this menu: QUIT\n- Leave this server: QUIT_SERVER\n", strlen("\nADMIN MENU\n- - - - - -\nAvailable operations and commands:\n- Add an user: ADD_USER {username} {password} {administrador/cliente/jornalista}\n- Delete an user: DEL {username}\n- List all users: LIST\n- Leave this menu: QUIT\n- Leave this server: QUIT_SERVER\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
    	erro("Erro no sendto");
    }
    
    //Actions
    while(log) {
		
		//insert command
        if(sendto(socket, "\nInsert command:", strlen("\nInsert command:"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        	erro("Erro no sendto");
    	}
    	
    	//receive command 
        if(recvfrom(socket, buffer_resposta, BUFLEN, 0, (struct sockaddr *) &cliente, &addrlen) == -1) {
        	erro("Erro no recvfrom");
    	}
    	
    	char string[BUFLEN];
    	char *input[4];
    	
    	strncpy(string, strtok(buffer_resposta, "\n"), BUFLEN);

        char * token = strtok(string, " ");

        int i = 0;
        while (token != NULL) {
            input[i++] = token;
            token = strtok(NULL, " ");
        }

        //add
        if (strcmp(input[0], "ADD_USER") == 0) {
        	int not_valid=0;
        
        	for(int i=0; i<4; i++){
        		if(input[i]==NULL){
        			not_valid=1;
        		}
        	}
        	
        	if(not_valid){
        		if(sendto(socket, "Atention: parameters are missing.\n", strlen("Atention: parameters are missing.\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        			erro("Erro no sendto");
    			} 
    			continue;
        	}
        	//sem_wait(sem_users);
        	int verify=0;
            int found_user = 0;
            
            for (int i = 0; i < shm->n_users; i++) {
                if (strcmp(shm->users[i].username, input[1]) == 0) {
                    found_user = 1;
                    break;
                }
            }
            //sem_post(sem_users);
            
            //se ja existir
            if (found_user) {
                if(sendto(socket, "Username already exists.\n", strlen("Username already exists.\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        			erro("Erro no sendto");
    			}   
    		//se nao existir: cria um user	 
            } else {
            	
            	verify+=verification_alnum(input[1]);
            	
            	if (verify!=0){
        			if(sendto(socket, "Username not valid.\n", strlen("Username not valid.\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        			erro("Erro no sendto");
    				}
        			continue;
    			}
            	
    			verify+=verification_alnum(input[2]);
    
    			if (verify!=0){
        			if(sendto(socket, "Password not valid.\n", strlen("Password not valid.\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        			erro("Erro no sendto");
    				}
        			continue;
    			}
    			
    			if( ! (strcmp(input[3], "administrador") == 0 || strcmp(input[3], "leitor") == 0 ||strcmp(input[3], "jornalista") == 0)){
    				if(sendto(socket, "Invalid type of user.\n", strlen("Invalid type of user.\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        			erro("Erro no sendto");
    				}
        			continue;
    			}
                
                //sem_wait(sem_users);
                
                strcpy(shm->users[shm->n_users].username, input[1]);
    			strcpy(shm->users[shm->n_users].password, input[2]);
    			strcpy(shm->users[shm->n_users].type, input[3]);
    
    			shm->n_users++;
    			
    			//sem_post(sem_users);

                if(sendto(socket, "\nUser added with sucess!\n", strlen("\nUser added with sucess!\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        			erro("Erro no sendto");
    			}
            }

        //delete
        } else if (strcmp(input[0], "DEL") == 0){
            int found_user = 0;
            int user_index;
            
            //sem_wait(sem_users);

            for (int i = 0; i < shm->n_users; i++) {
                if (strcmp(shm->users[i].username, input[1]) == 0) {
                    found_user = 1;
                    user_index= i;
                    break;
                }
            }

            if (found_user) {
                for (int i = user_index; i < shm->n_users - 1; i++) {
                    shm->users[i] = shm->users[i + 1];
                }
                
                if(sendto(socket, "\nUser deleted.\n", strlen("\nUser deleted.\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        			erro("Erro no sendto");
    			}
    			
    			shm->n_users--;
    			
    			//sem_post(sem_users);
    			
            } else{
                if(sendto(socket, "\nUser doesnt exist.\n", strlen("\nUser doesnt exist.\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        			erro("Erro no sendto");
    			}
            }          

        //list
        } else if (strcmp(input[0], "LIST") == 0){
            if(sendto(socket, "\nUser list:\n\n", strlen("\nUser list:\n\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        		erro("Erro no sendto");
    		}
    		
    		//sem_wait(sem_users);

            for (int i = 0; i < shm->n_users; i++) {
                sprintf(buffer_resposta, "%s\t%s\n", shm->users[i].username, shm->users[i].type);
                if(sendto(socket, buffer_resposta, strlen(buffer_resposta), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        			erro("Erro no sendto");
    			}
            }
            
            //sem_post(sem_users);

        //quit
        } else if (strcmp(input[0], "QUIT") == 0){
            //terminar o cliente
            log=0;
            saiu=0;

        //quit server
        } else if (strcmp(input[0], "QUIT_SERVER") == 0){
            saiu=1;
            
            if(sendto(socket, "\nServer closed. Connect again soon :)", strlen("\nServer closed. Connect again soon :)"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        		erro("Erro no sendto");
    		}
            
            return;
        } else{
            if(sendto(socket, "\nCommand not valid.\n", strlen("\nCommand not valid.\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        		erro("Erro no sendto");
    		}
        }
    }
}


//Login Menu UDP
void login_menu_udp(int socket, struct sockaddr_in cliente, socklen_t addrlen) {
    char buffer_resposta[BUFLEN];
    int found_user = 1;
    int logged_in = 0; // inicializa como não logado
    
    while(!logged_in){
		
		//enviar menu
    	if(sendto(socket, "\nLOGIN\nUsername:", strlen("\nLOGIN\nUsername:"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        	erro("Erro no sendto");
    	}
		
		//receber username
    	if(recvfrom(socket, buffer_resposta, BUFLEN, 0, (struct sockaddr *) &cliente, &addrlen) == -1) {
        	erro("Erro no recvfrom");
    	}
    	
    	User current_user;
    	strncpy(current_user.username, strtok(buffer_resposta, "\n"), BUFLEN);
    	
    	//verificar username
    	//sem_wait(sem_users);
    	
        for (int i = 0; i < shm->n_users; i++) {
            if (strcmp(shm->users[i].username, current_user.username) == 0) {
                
                //user found
                found_user = 0;
                
                //enter password message
                if(sendto(socket, "Password:", strlen("Password:"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        			erro("Erro no sendto");
    			}
                
                //recive password from user
                if(recvfrom(socket, buffer_resposta, BUFLEN, 0, (struct sockaddr *) &cliente, &addrlen) == -1) {
        			erro("Erro no recvfrom");
    			}
    			
    			strncpy(current_user.password, strtok(buffer_resposta, "\n"), BUFLEN);

                //verify password
                if (strcmp(shm->users[i].password, current_user.password) == 0) {
                    
                    //Correct password
                	if(sendto(socket, "\nWelcome!\n", strlen("\nWelcome!\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        				erro("Erro no sendto");
    				}
    				
    				//check if it is an admin
                    if(strcmp(shm->users[i].type, "administrador")==0){
                        //open admin menu
                        admin_menu(socket, cliente, addrlen);
                        
                        if(saiu == 1){
                        	return;
                        }
                        
                        break;
                    } else{
                    	if(sendto(socket, "\nUser does not have admin permission.\n", strlen("\nUser does not have admin permission.\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        					erro("Erro no sendto");
    					}
    					break;
                    }
                } else {
                
                    //Incorrect password
                    if(sendto(socket, "\nIncorrect password.\n", strlen("\nIncorrect password.\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        				erro("Erro no sendto");
    				}
                    break; 
                }
        	}
        }
        //sem_post(sem_users);
        
        if (found_user) { 
            //user not found
            if(sendto(socket, "\nUser not found.\n", strlen("\nUser not found.\n"), 0, (struct sockaddr*)&cliente, addrlen) == -1) {
        		erro("Erro no sendto");
    		}
        }
    }  	
}


//Client Menu TCP


void client_menu(int client_fd, User client, int user_index){
    char buffer_resposta[2050];
    int nread = 0;
    shm->users[user_index].num_Subscriptions = 0;
    
    char client_console[] = "\nCLIENT MENU\n- - - - - -\nAvailable operations and commands:\n- List Topics: LIST_TOPICS\n- Subsribe topic: SUBSCRIBE_TOPIC {Topic ID}\n- Log out: QUIT\n";
    char journalist_console[] = "\nCLIENT MENU\n- - - - - -\nAvailable operations and commands:\n- List Topics: LIST_TOPICS\n- Create Topic: CREATE_TOPIC {Topic ID} {Topic Title}\n- Subsribe topic: SUBSCRIBE_TOPIC {Topic ID}\n- Send News: SEND_NEWS {Topic ID} {News}\n- Log out: QUIT\n";
    int log=1;
    int verify;
    
    //Print menu
    if(strcmp(client.type, "jornalista")==0){
        if(write(client_fd, journalist_console, sizeof(journalist_console)) < 0){
            erro("Erro no write");
        }
        if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        	erro("Erro no meu read");
    	}
    }
    else if(strcmp(client.type, "leitor")==0){
        if(write(client_fd, client_console, sizeof(client_console)) < 0){
            erro("Erro no write");
        }
        if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        	erro("Erro no meu read");
    	}
    }
    
    //Actions
    while(log) {
		//insert command
        if(write(client_fd, "\n- - - - - -\nInsert command:", sizeof("\n- - - - - -\nInsert command:"))<0) {
        	erro("Erro no write");
    	}
    	//receive command 
        if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        	erro("Erro no meu read");
    	}
    	buffer_resposta[nread] = '\0';
    	
    	
    	char string[BUFLEN];
    	char *input[4];
    	
    	strncpy(string, strtok(buffer_resposta, "\n"), BUFLEN);

        char * token = strtok(string, " ");

        int i = 0;
        while (token != NULL) {
            input[i++] = token;
            token = strtok(NULL, " ");
        }
              

        //add
        if (strcmp(input[0], "CREATE_TOPIC") == 0) {
        	int not_valid=0;
        	
        
        	for(int i=0; i<3; i++){
        		if(input[i]==NULL){
        			not_valid=1;
        		}
        	}
        	
        	if(not_valid){
        		if(write(client_fd, "Atention: parameters are missing.\n", strlen("Atention: parameters are missing.\n")) < 0) {
        			erro("Erro no write");
    			}
    			if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			} 
    			continue;
        	}
        
        	int verify=0;
            int found_topic = 0;
            
            //sem_wait(sem_topics);
            
            for (int i = 0; i < shm->n_topics; i++) {
                if (shm->topics[i].id == atoi(input[1]) || strcmp(shm->topics[i].title, input[2]) == 0) {
                    found_topic = 1;
                    break;
                }
            }
            
            //sem_post(sem_topics);
            
            //se ja existir
            if (found_topic) {
                if(write(client_fd, "Topic already exists.\n", strlen("Topic already exists.\n")) < 0) {
        			erro("Erro no write");
    			}
    			if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			}    
    		//se nao existir: cria um topic 
            } 
            
            else {
            	
            	verify+=verification_num(input[1]);
            	
            	if (verify!=0){
        			if(write(client_fd, "ID not valid.\n", strlen("ID not valid.\n")) < 0) {
        			    erro("Erro no write");
    				}
    			if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			} 
        			continue;
    			}

                if(strcmp(client.type, "jornalista") != 0){
                    if(write(client_fd, "Client does not have permission for this command.\n", strlen("Client does not have permission for this command.\n")) < 0){
                        erro("Erro no write");
                    }
                if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			} 
                    continue;
                }   
                
                //sem_wait(sem_topics);
                createTopic(atoi(input[1]), input[2]);
    			//sem_post(sem_topics);

                if(write(client_fd, "Topic added with sucess!\n", strlen("Topic added with sucess!\n"))< 0) {
        			erro("Erro no sendto");
    			}
    			if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			} 
            }
        //delete
        }else if(strcmp(input[0], "SEND_NEWS") == 0){
            int not_valid=0;
        
        	for(int i=0; i<3; i++){
        		if(input[i]==NULL){
        			not_valid=1;
        		}
        	}
        	
        	if(not_valid){
        		if(write(client_fd, "Atention: parameters are missing.\n", strlen("Atention: parameters are missing.\n")) < 0) {
        			erro("Erro no write");
    			} 
                if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			} 
    			continue;
        	}

            if (verify!=0){
        		if(write(client_fd, "ID not valid.\n", strlen("ID not valid.\n")) < 0) {
        			erro("Erro no write");
    			}
                if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			} 
        		continue;
    		}

            if(strcmp(client.type, "jornalista") != 0){
                if(write(client_fd, "Client does not have permission for this command.\n", strlen("Client does not have permission for this command.\n")) < 0){
                    erro("Erro no write");
                }
                if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			} 
                continue;
            }



            int found_topic = 0;
            int topic_index;
            
            //sem_wait(sem_topics);

            for(int i = 0; i < shm->n_topics; i++) {
                if (shm->topics[i].id == atoi(input[1])) {
                    found_topic = 1;
                    topic_index = i;
                    break;
                }
            }

            if (found_topic){
                //strcpy(shm->topics[topic_index].news, input[2]);
                //MULTICAST SET
                //JUAN
                if(sendto(shm->topics[topic_index].socket_fd, input[2], sizeof(input[2]), 0, (struct sockaddr*)&shm->topics[topic_index].addr, sizeof(shm->topics[topic_index].addr)) == -1) {
        			erro("Erro no sendto");
    			}
                if(write(client_fd, "News sent!\n", strlen("News sent!\n")) < 0){
                    erro("Erro no write");
                }
                if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			} 
            }
            else{
                if(write(client_fd, "Topic not found!\n", strlen("Topic not found!\n")) < 0){
                    erro("Erro no write");
                }
                if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			} 
            }
            
            //sem_post(sem_topics);

        //list
        }else if (strcmp(input[0], "LIST_TOPICS") == 0){
        	char everytopic[BUFLEN];
        	sprintf(everytopic, "\nTopic list:\n\n");
        	
        	//sem_wait(sem_topics);
        	
            for (int i = 0; i < shm->n_topics; i++) {
                sprintf(buffer_resposta, "%d\t%s\n", shm->topics[i].id, shm->topics[i].title);
                strcat(everytopic, buffer_resposta);
            }
            
            //sem_post(sem_topics);
            
            //printf("\nEste é o buffer_resposta enviado: %s\n", everytopic);
            if(write(client_fd, everytopic, sizeof(everytopic)) <0) {
        			erro("Erro no write");
    			}
    		if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			} 

        //quit
        } else if (strcmp(input[0], "QUIT") == 0){
            if(write(client_fd, "\nSee you soon! :)", strlen("\nSee you soon! :)")) <0) {
        		erro("Erro no sendto");
    		}
    		
    		if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        		erro("Erro no meu read");
    		}
            exit(0);

        //--
        } else if (strcmp(input[0], "Comando") == 0){
            continue;
            
        } else if (strcmp(input[0], "SUBSCRIBE_TOPIC") == 0){
        	int not_valid=0;
        
        	for(int i=0; i<2; i++){
        		if(input[i]==NULL){
        			not_valid=1;
        		}
        	}
        	
        	if(not_valid){
        		if(write(client_fd, "Atention: parameters are missing.\n", strlen("Atention: parameters are missing.\n")) < 0) {
        			erro("Erro no write");
    			}
    			if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			} 
    			continue;
        	}
        
        	int verify=0;
            int found_topic = 0, topic_index;
            
            //sem_wait(sem_topics);
            
            for (int i = 0; i < shm->n_topics; i++) {
                if (shm->topics[i].id == atoi(input[1])) {
                    found_topic = 1;
                    topic_index = i;
                    break;
                }
            }
            
            //sem_post(sem_topics);
            
            //se nao existir
            if (!found_topic) {
                if(write(client_fd, "Topic doesnt exist.\n", strlen("Topic doesnt exist.\n")) < 0) {
        			erro("Erro no write");
    			}
    			if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			}    
    		//se existir: subscreve um topic 
            } 
            
            else {
                
                //sem_wait(sem_topics);
                
                strcpy(shm->users[user_index].subscriptions[shm->users[user_index].num_Subscriptions], input[1]);
    			shm->users[user_index].num_Subscriptions++;
                sprintf(buffer_resposta, "%s;%d",shm->topics[topic_index].address, shm->topics[topic_index].port);
    		    if(write(client_fd, buffer_resposta, sizeof(buffer_resposta))< 0){
        			erro("Erro no sendto");
    			}
    			
    			printf("AAAAAAAAAAAAAAAAAaaa %s\n", buffer_resposta);

    			//sem_post(sem_topics);
    			if((nread = read(client_fd, buffer_resposta, sizeof(buffer_resposta))) <= 0) {
        			erro("Erro no meu read");
    			}
                buffer_resposta[nread] = '\0';
    			printf("Resposta: %s\n", buffer_resposta);
    			//sem_post(sem_topics);
                
                if(write(client_fd, "Topic doesnt exist.\n", strlen("Topic doesnt exist.\n")) < 0) {
        			erro("Erro no write");
    			}
                
    			printf("enviou: topic sucess\n");
    			if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no meu read");
    			}
                printf("Resposta2: %s\n",buffer_resposta);
                continue;
            }
           

        }
        else{
            if(write(client_fd, "Command not valid.\n", strlen("Command not valid.\n")) <0) {
        		erro("Erro no sendto");
    		}
    		if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        		erro("Erro no meu read");
    		} 
        }
    }
}

//Login Menu TCP

void login_menu_tcp(int client_fd){
    char buffer_resposta[BUFLEN];
    int nread = 0;
    int found_user = 1;
    int logged_in = 0; // inicializa como não logado
    int user_index;
    
    while(!logged_in){
		
		//enviar menu
    	if(write(client_fd, "\nLOGIN\nUsername:", strlen("\nLOGIN\nUsername:")) < 0) {
        	erro("Erro no write");
    	}
		
		//receber username
    	if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        	erro("Erro no read");
    	}
    	buffer_resposta[nread] = '\0';
    	
    	User current_user;
    	strncpy(current_user.username, strtok(buffer_resposta, "\n"), BUFLEN);
    	
    	//verificar username
    	//sem_wait(sem_users);
    	
        for (int i = 0; i < shm->n_users; i++) {
        	
            if (strcmp(shm->users[i].username, current_user.username) == 0) {
            	
                //user found
                found_user = 0;
                user_index=i;
                //enter password message
                if((nread = write(client_fd, "Password:", sizeof("Password:")))<=0) {
        			erro("Erro no write");
    			}
                
                //recive password from user
                if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        			erro("Erro no read");
    			}
    			buffer_resposta[nread] = '\0';    			
    			strncpy(current_user.password, strtok(buffer_resposta, "\n"), BUFLEN);
                //verify password
                if (strcmp(shm->users[i].password, current_user.password) == 0) {
                    //Correct password
    				//check if it is an admin
                    if(strcmp(shm->users[i].type, "jornalista")==0 || strcmp(shm->users[i].type, "leitor") == 0){
                        //open client menu
                        if(write(client_fd, "\nWelcome!\n", strlen("\nWelcome!\n"))<=0) {
        				erro("Erro no write");
    					}
    				
                        client_menu(client_fd, shm->users[i], user_index);
                        continue;
                    } 
                    else{
                    	if(write(client_fd, "\nAdmins must use UDP connections.", strlen("\nAdmins must use UDP connections."))<0) {
        					erro("Erro no write");
    					}
    					
    					if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        					erro("Erro no read");
    					}
    					continue;
                    }

                } else {
                
                    //Incorrect password
                    if(write(client_fd, "\nIncorrect password.", strlen("\nIncorrect password."))<0) {
        				erro("Erro no write");
    				}
    				
    				if((nread = read(client_fd, buffer_resposta, BUFLEN)) <= 0) {
        				erro("Erro no read");
    				}
    					
                    continue; 
                }
        	}
        }
        
        //sem_post(sem_users);
        
        if (found_user) { 
            //user not found
            if(write(client_fd, "\nUser not found.\n", strlen("\nUser not found.\n") < 0)) {
        		erro("Erro no write");
    		}
        }
    }  	
}


//MAIN -----------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
	
	setpgrp();  
    
	//VERIFICAÇOES --------------
	
	//Numero argumentos
	if(argc!=4){
        printf("Wrong paramaters! Expected ./news_server {NEWS_PORT} {CONFIG_PORT} {Config File}\n");
        exit(0);
    }
    
    //Argumentos sao inteiros
    int verify=0;
    
    verify += verification_num(argv[1]);
    verify += verification_num(argv[2]);
    if(verify!=0){
        printf("Wrong parameters, the ports introduced are not numbers!\n");
        exit(0);
    }
	
	int news_port = atoi(argv[1]);
    int config_port = atoi(argv[2]);
    
    //Último argumento é um file
    verify = verification_file(argv[3]);
    
    if(verify!=0){
        printf("Wrong parameters! The file introduced is not the correct file type. Expected \".txt\"\n");
        exit(0);
    }
    
    
    //SHARED MEMORY -------------
    
    int SIZE_SHARED_MEMORY= (sizeof(SharedMemory)+sizeof(User)*MAX_USERS)+(sizeof(Topic)*MAX_TOPICS);
    
    //Criar
    shmid= shmget(IPC_PRIVATE, SIZE_SHARED_MEMORY, IPC_CREAT | 0666);
    //Anexar
    shm= (SharedMemory*) shmat(shmid, NULL, 0);
    
    //Verificação
    if (shmid == -1) {
    	perror("Error: creating shared memory.\n");
    	exit(0);
    }
	if (shm == (void*)-1) {
    	perror("Error: attaching shared memory.\n");
    	exit(0);
    }
    
    //Localizar ponteiros na memória
    shm->users = (User*) (((void*)shm) + sizeof(SharedMemory));
    shm->topics = (Topic*)((void*)shm->users + sizeof(User)*MAX_USERS);
    //Inicializar
    shm->n_users=0;
	shm->n_topics=0;
	//shm->users.num_subscriptions=0;
	
	
	//NAMED SEMAPHORES ----------
	
    sem_unlink("sem_users");
	sem_unlink("sem_topics");
    
    //Criar
    sem_users = sem_open("sem_users", O_CREAT | O_EXCL , 0666, 1);
    sem_topics = sem_open("sem_topics", O_CREAT | O_EXCL , 0666, 1);
    
    //Verificações:
	if (sem_users == SEM_FAILED) {
		fprintf(stderr, "Error: sem_open() failed for sem_users. errno:%d\n", errno);
		exit(0);
	}
	if (sem_topics == SEM_FAILED) {
		fprintf(stderr, "Error: sem_open() failed for sem_topics. errno:%d\n", errno);
		exit(0);
	}
	


	//LER CONFIG FILE -----------
	
	FILE *config_file;
	char *line = NULL;
	size_t len= 0;
	int counter_token=0, read;
	
	if ((config_file=fopen(argv[3],"r+"))==NULL) {
	    perror("Failed to open config file\n");
	    exit(1);
    }

    while((read=getline(&line, &len, config_file)) !=-1) {
		if(line[strlen(line)-1]=='\n')
			line[strlen(line)-1]='\0';

		counter_token=0;
    	char *input[3];
    	char * token = strtok(line, ";");
    	int iteration = 0;
    	
    	if (token == NULL){
        	printf("Error in config file: One of the lines is empty.\n");
        	exit(0);
    	}
    
    	if(verify!=0){
        	printf("Error in config file: One of the introduced parameters is not alpha numeric.\n");
        	exit(0);
    	}
    	
    	while(token!=NULL){
        	input[iteration++] = token;
        	token = strtok(NULL, ";");
    	}
    
    	verify+=verification_alnum(input[0]);
    	verify+=verification_alnum(input[1]);
    	verify+=verification_alnum(input[2]);
    	
    	//Verificar se é leitor..jornalista/..
    
    	if (verify!=0){
        	printf("Error in config file: One of the introduced parameters is not alpha numeric.\n");
        	exit(0); 
    	}
    	
    	strcpy(shm->users[shm->n_users].username, input[0]);
    	strcpy(shm->users[shm->n_users].password, input[1]);
    	strcpy(shm->users[shm->n_users].type, input[2]);
    	
    	shm->n_users++;
    	
    }
    
	
    //FORK
    char buf[BUFLEN];
    
    pid_t pid = fork();

    if(pid <0){
        erro("Erro no fork\n");
    } 
    else if(pid==0){
        //UDP - - - - - - - - - - - - - - - - 
    	struct sockaddr_in si_minha, si_outra;
    	int s, recv_len;
    	socklen_t slen = sizeof(si_outra);

    	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        	erro("Erro na criação do socket");
    	}

    	si_minha.sin_family = AF_INET;
    	si_minha.sin_port = htons(config_port);
   		si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    	if(bind(s, (struct sockaddr*)&si_minha, sizeof(si_minha)) == -1) {
    		erro("Erro no bind");
    	}
    
    	//Conection
    	for(int i=0; i<5; i++){
    		if((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_outra, &slen)) == -1) {
        		erro("Erro no recvfrom");
    		}

    		buf[recv_len] = '\0';
    		printf("Recebi uma mensagem do sistema com o endereço %s e o porto %d\n", inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port));
    		printf("Conteudo: %s\n", buf);
    		}
    		

    	while(1) {
    		login_menu_udp(s, si_outra, slen);
    		break;
    	}
    	
    	close(s); 
    }
    else{
        int fd, client;
  	    struct sockaddr_in addr, client_addr;
  	    int client_addr_size;

  	    bzero((void *) &addr, sizeof(addr));
  	    addr.sin_family = AF_INET;
  	    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	    addr.sin_port = htons(news_port);

  	    if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		    erro("na funcao socket");
  	    if ( bind(fd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
		    erro("na funcao bind");
  	    if(listen(fd, 5) < 0)
		    erro("na funcao listen");
  	    client_addr_size = sizeof(client_addr);
  	    while (1) {
    	    //clean finished child processes, avoiding zombies
    	    //must use WNOHANG or would block whenever a child process was working
    	    while(waitpid(-1,NULL,WNOHANG)>0);
    	    //wait for new connection
    	    client = accept(fd,(struct sockaddr *)&client_addr,(socklen_t *)&client_addr_size);
    	    if (client > 0) {
      		    if (fork() == 0) {
        		    close(fd);
        		    login_menu_tcp(client);
        		    exit(0);
      		    }
    		close(client);
    		}
    	}
    }
    
    //printf("saiu\n");
    killpg(getpgrp(), SIGTERM);
    
    return 0;
}
