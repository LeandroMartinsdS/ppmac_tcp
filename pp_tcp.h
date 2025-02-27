#define PORT 8080
#define MAXPENDING 5
#define BUFFSIZE 64

int serverSock;

void Die(char *message);
void kill_handler(int sig);
void HandleClient(int clientSock);
