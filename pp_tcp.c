/*For more information see notes.txt in the Documentation folder */
#include <gplib.h>
#include <stdlib.h>
#include <errno.h>

#define _PPScriptMode_		// for enum mode, replace this with #define _EnumMode_
#include "../../Include/pp_proj.h"
#include "pp_tcp.h"


inline void Die(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

inline void kill_handler(int sig) {
  close(serverSock);
  CloseLibrary();
  exit(EXIT_SUCCESS);
}

inline void process_data(double values[6]) {
    static int i;

    for (i = 0; i < 6; i++) {
        pshm->P[(100+i)] = values[i];
    }
}

void HandleClient(int clientSock) {
    static char buffer[BUFFSIZE];
    static ssize_t bytes_received;
    static double values[6];

    InitLibrary();  // Required for accessing Power PMAC library
    double exec_time = GetCPUClock();
    int i=0;
    while (1) {
        // Receive message from client
        bytes_received = recv(clientSock, buffer, BUFFSIZE, 0);
        if (bytes_received <= 0) {
            if (bytes_received < 0) {
                perror("recv");
            } 
//            else {
//                printf("Client disconnected\n");
//            }
            break;
        }

        // Check for shutdown command
        if (strncmp(buffer, SHUTDOWN_CMD, bytes_received) == 0) {
            printf("Shutdown command received\n");
            close(clientSock);
            close(serverSock);
            CloseLibrary();
            exit(EXIT_SUCCESS);
        }

        // Check if the received bytes match the expected size
        if (bytes_received != BUFFSIZE) {
            printf("Warning: Expected %d bytes, but received %zd bytes\n", BUFFSIZE, bytes_received);
            continue;
        }

        // Unpack data
        memcpy(values, buffer, 6 * sizeof(double));
        
        process_data(values);
        printf("%f\n", GetCPUClock()-exec_time);
    }

    close(clientSock);
    close(serverSock);
    CloseLibrary();
    exit(EXIT_SUCCESS);
}
int main() {
    //int serverSock, clientSock;
    // global int serverSock due to kill_handler
    int clientSock;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen;

    //------------------------------------------------
    // Reguired uncontrolled program terminations
    //-------------------------------------------------
    signal(SIGTERM,kill_handler);   // Process Termination (pkill)
    signal(SIGINT,kill_handler);    // Interrupt from keyboard (^C)
    signal(SIGHUP,kill_handler);    // Hangup of controlling window (Close of CMD window)
    signal(SIGKILL,kill_handler);   // Forced termination (ps -9)
    signal(SIGABRT,kill_handler);   // Abnormal termination (segmentation fault)
    
    // Create the TCP socket
    if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        Die("Failed to create socket");
    }

   // Set socket options
    int opt = 1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        Die("Failed to set socket options");
    }

    // Construct the server sockaddr_in structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    // Bind the server socket
    if (bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        Die("Failed to bind the server socket");
    }

    // Listen on the server socket
    if (listen(serverSock, MAXPENDING) < 0) {
        Die("Failed to listen on server socket");
    }

    InitLibrary();
    // Run until cancelled
    while (1) {
        clientLen = sizeof(clientAddr);
        // Wait for client connection
        if ((clientSock = accept(serverSock, (struct sockaddr *) &clientAddr, &clientLen)) < 0) {
            Die("Failed to accept client connection");
        }
        else {
           // Client connected
            HandleClient(clientSock);
            //break;
        }
    }

    close(serverSock);
    CloseLibrary();  // Required for accessing Power PMAC library
    return 0;
}

