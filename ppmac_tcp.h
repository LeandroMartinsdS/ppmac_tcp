#define DEBUG
#define RUN_AS_RT_APP

// Socket settings
#define MAXPENDING 5
#define VAR_NUM     6
#define BUFFSIZE VAR_NUM*sizeof(double)
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

#else
    #include <gplib.h>
    #include <stdlib.h>
    #include <errno.h>

    #define _PPScriptMode_		// for enum mode, replace this with #define _EnumMode_
    #include "../../Include/pp_proj.h"

#endif

int serverSock;

void InitSocket(char *host, int port);
void AcceptClient();
void HandleClient(int clientSock);
void CloseSocket(int sock);
void Die(char *message);
void kill_handler(int sig);
void test_process_data(double values[]);
