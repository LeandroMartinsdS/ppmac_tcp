// #define DEBUG
#define RUN_AS_RT_APP

// Socket settings
#define PORT 8080
#define MAXPENDING 5
#define BUFFSIZE 48 // 6* sizeof(double)
#define SHUTDOWN_CMD "SHUTDOWN"

// Constants
#define MASTER_ECT_BASE 19

#ifdef DEBUG
    # define MAX_P 65536
    struct SHM {
        double P[MAX_P];            // Global P variable Array
    };
    struct SHM  *pshm;              // Pointer to shared memory
    void        *pushm;             // Pointer to user shared memory

    #include <stdio.h>
    #include <stdlib.h>
    // #include <stdbool.h>
    #include <string.h>
    #include <stddef.h>
    //
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <signal.h>
    #include <unistd.h>

    #define HOST "127.0.0.1"

#else
    #include <gplib.h>
    #include <stdlib.h>
    #include <errno.h>

    #define _PPScriptMode_		// for enum mode, replace this with #define _EnumMode_
    #include "../../Include/pp_proj.h"

    #define HOST "172.23.59.7"

#endif



int serverSock;

void Die(char *message);
void kill_handler(int sig);
void HandleClient(int clientSock);
