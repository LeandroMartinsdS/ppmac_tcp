#include "pp_tcp.h"

inline void Die(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

inline void kill_handler(int sig) {
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

    for (i = 0; i < 6; i++) {
        *ptr = values[i];
        ptr++;

        // printf("%3.6f \t | ", *ptr);
        printf("%p: %3.4f \t| ", ptr, *ptr);
    }
    printf("\n");
}

void HandleClient(int clientSock) {
    static char buffer[BUFFSIZE];
    static size_t bytes_received;
    static double values[6];

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
        memcpy(values, buffer, 6 * sizeof(double));

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
int main() {
    //int serverSock, clientSock;
    // global int serverSock due to kill_handler
    int clientSock;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen;

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

    #ifdef DEBUG
    if (inet_pton(AF_INET, HOST, &serverAddr.sin_addr) <= 0) {
        Die("Invalid address/ Address not supported");
    }
    #endif

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
        else {
           // Client connected
            HandleClient(clientSock);
            //break;
        }
    }

    // close(serverSock);
    #ifdef DEBUG
    free(pushm);
    #else
    CloseLibrary();
    #endif
    return 0;
}

