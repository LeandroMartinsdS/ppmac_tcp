/*For more information see notes.txt in the Documentation folder */
#include <gplib.h>
#include <stdlib.h>
#include <errno.h>

#define _PPScriptMode_		// for enum mode, replace this with #define _EnumMode_
#include "../../Include/pp_proj.h"
#include "pp_tcp.h"


void Die(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void kill_handler(int sig) {
  close(serverSock);
  CloseLibrary();
  exit(EXIT_SUCCESS);
}

void HandleClient(int clientSock) {
    char buffer[BUFFSIZE];
    char dest[10];
    int received = -1;
    InitLibrary();  // Required for accessing Power PMAC library

    // Receive message from client
    if ((received = recv(clientSock, buffer, BUFFSIZE, 0)) < 0) {
        Die("Failed to receive initial bytes from client");
    }

    // Send received message back to client
    while (received > 0) {
        // HACK
        pshm->P[100]=atof(buffer);
        *src = "USHM";
        // WriteBuffer(buffer,)
        // Check for more data
        if ((received = recv(clientSock, buffer, BUFFSIZE, 0)) < 0) {
            Die("Failed to receive additional bytes from client");
        }
    }
    printf("%s\n", *dest);
    close(clientSock);
}

int main() {
    //int serverSock, clientSock;
    // global int serverSock due to kill_handler
    int clientSock;
    struct sockaddr_in serverAddr, clientAddr;
    unsigned int clientLen;

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

    // Run until cancelled
    while (1) {
        clientLen = sizeof(clientAddr);
        // Wait for client connection
        if ((clientSock = accept(serverSock, (struct sockaddr *) &clientAddr, &clientLen)) < 0) {
            Die("Failed to accept client connection");
        }

        printf("Client connected: %s\n", inet_ntoa(clientAddr.sin_addr));
        HandleClient(clientSock);
    }

    close(serverSock);
    CloseLibrary();  // Required for accessing Power PMAC library
    return 0;
}

