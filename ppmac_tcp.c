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

void AcceptClient() {
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
           // Client connected
            HandleClient(clientSock);
        }
    }
}

void HandleClient(int clientSock) {
    static char buffer[BUFFSIZE];
    static size_t bytes_received;
    static double values[VAR_NUM];

    #ifndef DEBUG
    InitLibrary();  // Required for accessing Power PMAC library
    double exec_time = GetCPUClock();
    #endif

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
            #ifndef DEBUG
            CloseLibrary();
            #endif
            exit(EXIT_SUCCESS);
        }

        // Check if the received bytes match the expected size
        if (bytes_received != BUFFSIZE) {
            printf("Warning: Expected %d bytes, but received %zd bytes\n", BUFFSIZE, bytes_received);
            continue;
        }

        // Unpack data
        memcpy(values, buffer, VAR_NUM * sizeof(double));

        test_process_data(values);

        #ifndef DEBUG
        printf("%f\n", GetCPUClock()-exec_time);
        #endif
    }

    close(clientSock);
    close(serverSock);
    #ifndef DEBUG
    CloseLibrary();
    #endif
    exit(EXIT_SUCCESS);
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

  #ifndef DEBUG
  CloseLibrary();
  #endif

  exit(EXIT_SUCCESS);
}

void test_process_data(double values[]) {
    int i;
    double *ptr;
    ptr = (double *) pushm + MASTER_ECT_BASE;

    for (i = 0; i < VAR_NUM; i++) {
        *ptr = values[i];
        // printf("%3.6f \t | ", *ptr);
        printf("%p: %3.4f \t| ", ptr, *ptr);
        ptr++;
    }
    printf("\n");
}

int main() {
    // global int serverSock due to kill_handler
    char *host = "127.0.0.1";
    int port = 8080;

    #ifdef DEBUG
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
    #endif // DEBUG
    InitSocket(host, port);
    AcceptClient();
    CloseSocket(serverSock);

    #ifdef DEBUG
    free(pushm);
    #else
    CloseLibrary();
    #endif
    return 0;
}

