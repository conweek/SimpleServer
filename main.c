#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>

int process_get_req(char** request) {

    char* token;
    
    // There is a better way to do this
    // TODO: fix this
    // We just want the 2nd element, no need for the for loop
    for (int i = 0; i < 2; i++) {

        if (!(token = strtok_r(*request, " ", request))) {
            fprintf(stderr, "[!] Couldn't identify what the GET request wants?\r\n");
            fflush(stderr);
            return 1;
        }

        // If we're at the 2nd element
        if (i == 1) {
            printf("%s\r\n", token);
            fflush(stdout);
        }

    }

    return 0;

}

// 0 on success
int process_header(char** header) 
{

    // TODO: Fix this
    // Shouldn't need to back the backup pointer here
    char* tempHeader = strdup(*header);
    char* tempHeaderBackup = tempHeader;

    if (!tempHeader) {
        perror("[!] Couldn't allocate temporary header for manipulation\r\n");
        free(tempHeader);
        return 1;
    }

    char* line;
    unsigned int count = 0;
    char* substr;
    while ((line = strtok_r(tempHeader, "\n", &tempHeader))) {
   
        // Check we're on the first line of header
        if (count == 0) {
            

            // TODO: Add POST check
            // Checks whether this is a valid HTTP GET or POST request
            if (!(substr = strstr(line, "GET"))) {
                fprintf(stderr,"[!] Couldn't find GET or POST!\r\n");
                free(tempHeader);
                return 2;
            } else {
                printf("This is a GET Request!!\r\n");
                
                if (process_get_req(&substr)) {
                    fprintf(stderr, "[!] Error in GET request processing\r\n");
                    free(tempHeader);
                    return 2;

                }
            }
        
        }

        count++;

    }
    free(tempHeaderBackup);
    return 0;
}

// TODO: Implement client handling here
// this will handle the page requests
// and the POSTs back :)
void* handle_client(void* params) 
{
    int connectionFd = *((int*)params);
    
    // Temporary header read buffer
    // TODO: probably make this less than 16Kb
    char headerBuf[16000];
    char* wholeMessage;
    uint8_t readCount = 0;
    uint8_t bytesRead = 0;

    printf("[!] New connection thread started!\r\n");
    fflush(stdout);
    
    // While we're not EOF or an error
    while ((bytesRead = read(connectionFd, headerBuf, 16000)) > 0) {
        
        if (readCount > 0) {
            // Reallocates the size to store the whole header
            wholeMessage = (char*)realloc(wholeMessage, (16000 * sizeof(int)) * readCount);
            
            // Check the pointer was allocated
            if (!wholeMessage) {
                fprintf(stderr, "[!] Connection error!\r\n\tCouldn't reallocate space in header storage!\r\n");
                fflush(stderr);
            }

        } else {
            wholeMessage = (char*)malloc(16000 * sizeof(char));
            memset(wholeMessage, 0, 16000);

            // Check the pointer was allocated
            if (!wholeMessage) {
                fprintf(stderr, "[!] Connection error:\r\n\tCouldn't allocate space for the header\r\n");
                fflush(stderr);
            }
            
            printf("[!] Allocated header storage successfully!\r\n");


            if (!memcpy(wholeMessage, headerBuf, sizeof(headerBuf))) {
                perror("[!] Failed to memcpy\r\n");
            }
        }

        readCount++;

        if(process_header(&wholeMessage)) {
        
            if (wholeMessage) {
                free(wholeMessage);
            }
        
            close(connectionFd);
            return NULL;
        }
    }
    
    if (bytesRead < 0) {
        fprintf(stderr, "[!] Error in reading the connection header!\r\n");
        return NULL;
    }
   
    // Connection aborted 
    printf("[!] No more to read!\r\n");
    fflush(stdout);
        
    return NULL;
}

int main(int argc, char** argv)
{

    // Create the socket file descriptor
    int socketFd;
    
    // Create the socket addr struct
    struct sockaddr_in socketAddr;
    socketAddr.sin_family = AF_INET;
    socketAddr.sin_port = htons(8080);
    socketAddr.sin_addr.s_addr = INADDR_ANY;

    
    // Create socket
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[!] Couldn't create socket!\r\n");
       
        // TODO: Fix magic number
        exit(-10);
    }

    // Bind the address to the socket
    if (bind(socketFd, (struct sockaddr*)&socketAddr, sizeof(struct sockaddr_in)) < 0) {
        perror("[!] Failed to bind socket!\r\n");
        exit(-11);
    }

    // Start listening
    // TODO: implemention connection throttling
    if (listen(socketFd, 10) < 0) {
        perror("[!] Failed to start listening!\r\n");
        exit(-12);

    }
    
    printf("[!] Server started!\r\n");
    fflush(stdout);

    while (1) {

        int connectionFd;
        
        // Tries the accept connections from clients
        if ((connectionFd = accept(socketFd, NULL, NULL)) < 0) {
            perror("[!] Couldn't accept an incoming connection!\r\n");
            exit(-13);
        }

        // Creates a new thread if a client joins
        pthread_t clientThread;
        pthread_create(&clientThread, NULL, handle_client, &connectionFd);
        pthread_join(clientThread, NULL); 
        printf("[!] Connection ended\r\n");    
    
    }
}
