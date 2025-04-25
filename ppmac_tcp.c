#include "ppmac_tcp.h"

void InitSocket(char *host, int port) {
    struct sockaddr_in serverAddr;
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
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &serverAddr.sin_addr) <= 0) {
        Die("Invalid address/ Address not supported");
    }

    // Bind the server socket
    if (bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        Die("Failed to bind the server socket");
    }

    // Listen on the server socket
    if (listen(serverSock, MAXPENDING) < 0) {
        Die("Failed to listen on server socket");
    }
}

int AcceptClient() {
    int clientSock;
    struct sockaddr_in clientAddr;
    socklen_t clientLen;

    // Run until cancelled
    while (1) {
        clientLen = sizeof(clientAddr);
        // Wait for client connection
        if ((clientSock = accept(serverSock, (struct sockaddr *) &clientAddr, &clientLen)) < 0) {
            Die("Failed to accept client connection");
        }
        else {
            return clientSock;
        }
    }
}

int HandleClient(int clientSock, char * buffer, size_t data_size) {
    static size_t bytes_received;

    #ifndef LOCAL_HOST
    InitLibrary();  // Required for accessing Power PMAC library
    #ifdef DEBUG
    double exec_time = GetCPUClock();
    #endif  // DEBUG
    #endif  // LOCAL_HOST

    // Receive message from client
    bytes_received = recv(clientSock, buffer, data_size*sizeof(double), 0);
    if (bytes_received <= 0) {
        if (bytes_received < 0) {
            perror("recv");
        }
        return -1;
    }

    // Check for shutdown command
    if (strncmp(buffer, SHUTDOWN_CMD, bytes_received) == 0) {
        printf("Shutdown command received\n");
        close(clientSock);
        close(serverSock);
        #ifndef LOCAL_HOST
        CloseLibrary();
        #endif
        exit(EXIT_SUCCESS);
        return 1;
    }

    // Check if the received bytes match the expected size
    if (bytes_received != data_size) {
        printf("Warning: Expected %zd bytes, but received %zd bytes\n", data_size, bytes_received);
    }

    #ifdef DEBUG // Does this still make sense here?
    printf("%f\n", GetCPUClock()-exec_time);
    #endif
    return 0;
}

void CloseSocket(int sock) {
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) != 0 || error != 0) {
        close(sock);
    }
}

void Die(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void kill_handler(int sig) {
  close(serverSock);

  #ifndef LOCAL_HOST
  CloseLibrary();
  #endif

  exit(EXIT_SUCCESS);
}

void test_print_data(double *dest, size_t data_count) {
    int i;
    for (i = 0; i < data_count; i++) {
        printf("%p: %3.4f \t| ", dest, *dest);
        dest++;
    }
    printf("\n");
}

int main() {
    #ifdef LOCAL_HOST
    pushm = (void *)malloc(sizeof(pushm));  // HACK
    pshm = (void *)malloc(sizeof(pshm));    // HACK

    #else
    struct sched_param param;
    int done = 0;
    struct timespec sleeptime = {0};
    sleeptime.tv_nsec = NANO_5MSEC;	// #defines NANO_1MSEC, NANO_5MSEC & NANO_10MSEC are defined

    #ifndef RUN_AS_RT_APP
    //-----------------------------
    // Runs as a NON RT Linux APP
    //-----------------------------
    param.__sched_priority = 0;
    pthread_setschedparam(pthread_self(),  SCHED_OTHER, &param);
    #else
    //---------------------------------------------------------------
    // Runs as a RT Linux APP with the same scheduling policy as the
    // Background script PLCs
    // To run at a recommended lower priority use BACKGROUND_RT_PRIORITY - 10
    // To run at a higher priority use BACKGROUND_RT_PRIORITY + 1
    //---------------------------------------------------------------------
    param.__sched_priority =  BACKGROUND_RT_PRIORITY - 10;
    pthread_setschedparam(pthread_self(),  SCHED_FIFO, &param);
    #endif // RUN_AS_RT_APP

    InitLibrary();  // Required for accessing Power PMAC library
    #endif // LOCAL_HOST

    // global int serverSock due to kill_handler
    char *host = "127.0.0.1";
    int port = 8080;
    size_t data_count = 7;
    size_t data_size = data_count*sizeof(double);
    int clientSock;
    int socketStatus;
    double *dest = (double *) malloc(data_size);
    char buffer[BUFFSIZE];

    InitSocket(host, port);

    clientSock = AcceptClient();

    do {
        // TODO: Add busy-wait here until buffer is clear to be overwritten
        socketStatus = HandleClient(clientSock, buffer, data_size);
        memcpy(dest, buffer, data_size);
        test_print_data(dest, data_count);
    } while (socketStatus == 0);

    printf("Out of while\n");
    CloseSocket(clientSock);
    CloseSocket(serverSock);

    #ifdef LOCAL_HOST
    free(pushm);
    #else
    CloseLibrary();
    #endif
    return 0;
}

