#define PORT 8080
#define MAXPENDING 5
#define BUFFSIZE 48 // 6* sizeof(double)
#define SHUTDOWN_CMD "SHUTDOWN"

int serverSock;

void Die(char *message);
void kill_handler(int sig);
void HandleClient(int clientSock);
