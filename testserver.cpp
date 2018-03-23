#include <winSock2.h>
#pragma comment (lib, "Ws2_32.lib")
//-lws2_32
using namespace std;
#define MAXLINE 1024
#define ListenNum 10
#define DEBUG
#define BUFSIZE 1024

HANDLE hWaittid;
HANDLE hNumMutex;
HANDLE hexception;
volatile int threadNum = 0;

SOCKET open_listenfd(char *port);
void SIGINT_handler(int sig);
unsigned int thread(LPVOID varp);
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <process.h>
#include <signal.h>
unsigned int deal_request(SOCKET connfd);

int main(int argc, char **argv){
	WSADATA ws;
	if(WSAStartup(MAKEWORD(2, 2), &ws) != 0){
		printf("WSAStartup failed!\n");
		exit(0);
	}

	if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }

    /*ctr-c to terminate the server*/
    signal(SIGINT, SIGINT_handler);

    SOCKADDR_STORAGE clientaddr;
    int clientlen = sizeof(SOCKADDR_STORAGE);
    SOCKET listenfd = open_listenfd(argv[1]);
    if(listenfd == INVALID_SOCKET){
    	printf("open_listenfd failed!\n");
    	exit(0);
    }

    /*wait for all threads to return before main threads quit*/
    hWaittid = CreateSemaphore(NULL, 1, 1, NULL);
	hNumMutex = CreateMutex(NULL, FALSE, NULL);
	hexception = CreateEvent(NULL, TRUE, TRUE, NULL);

    for(int i = 0; 1; i++){
    	SOCKET connfd = accept(listenfd, (SOCKADDR *)&clientaddr, &clientlen);
    	/*main thread keep blocked while exception handler thread runs*/
    	WaitForSingleObject(hexception, INFINITE);

    	if(connfd == INVALID_SOCKET){
    		printf("accept return invalid socket\n");
    		SIGINT_handler(SIGINT);
    	}
    	HANDLE hThread;
    	unsigned threadID;

		/*create thread*/
    	hThread = (HANDLE) _beginthreadex(
			NULL, 0, thread, (LPVOID)connfd, 0, &threadID);
		CloseHandle(hThread); 
    }

    printf("program control should never reach here!\n");
    exit(0);
}

unsigned int thread(LPVOID varp){
	/*increase threadNum and relock semaphore in case 
    	hWaittid becomes 0 before all threads return*/
	WaitForSingleObject(hNumMutex, INFINITE);
	if(threadNum == 0)
		WaitForSingleObject(hWaittid, INFINITE);
	threadNum ++;
	ReleaseMutex(hNumMutex);

	
	SOCKET connfd = (SOCKET)varp;
	/*deal with client request*/
	deal_request(connfd);
	closesocket(connfd);

	/*deal with threadNum*/
	WaitForSingleObject(hNumMutex, INFINITE);
	threadNum --;
	if(threadNum == 0)
		ReleaseSemaphore(hWaittid, 1, NULL);
	ReleaseMutex(hNumMutex);
	return 0;
}

void SIGINT_handler(int sig){
	WaitForSingleObject(hexception, INFINITE);
	ResetEvent(hexception);
	printf("SIGINT Caught\n");
	/*Use Semaphore to wait for all threads to return*/
	WaitForSingleObject(hWaittid, INFINITE);
	WSACleanup();
	printf("^C\n");
	fflush(stdout);
	CloseHandle(hWaittid);
	CloseHandle(hNumMutex);
	CloseHandle(hexception);
	exit(0);
}

SOCKET open_listenfd(char *port){
	struct addrinfo hints, *listp, *p;
	SOCKET listenfd;
	int optval = 1024;

	/*Get listen socketfd*/
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;
	getaddrinfo(NULL, port, &hints, &listp);

	/*walk the list*/
	for(p = listp; p; p = p->ai_next){
		if((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
			 == INVALID_SOCKET){
			printf("socket failed\n");
			continue;
		}
		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
			(const char *)&optval, sizeof(int));

		if(bind(listenfd, p->ai_addr,p->ai_addrlen) == 0)/*success*/
			break;
#ifdef DEBUG
		printf("bind failed\n");
		if(listenfd = INVALID_SOCKET)
			printf("invalid listenfd\n");
		printf("2error code:%u\n", WSAGetLastError());
		printf("p->ai_addr:%s\n", p->ai_addr->sa_data);
#endif
		closesocket(listenfd);
	}

	/*clean up*/
	freeaddrinfo(listp);
	if(!p){
		printf("No addressworked list\n");
		return INVALID_SOCKET;
	}

	if(listen(listenfd, ListenNum) == SOCKET_ERROR){
		closesocket(listenfd);
		printf("listen error\n");
		return INVALID_SOCKET;
	}

	return listenfd;
}

unsigned int deal_request(SOCKET connfd){
	char request_buf[BUFSIZE];
	int rec = recv(connfd, request_buf, sizeof(request_buf), 0);
	printf("%s\n", request_buf);
	char buf[BUFSIZE], body[BUFSIZE];

    /* Build the HTTP response body */
    sprintf(body,
     "<html>\n<title>Tiny</title>\n<body>\n<h1>Hello Web Server!</h1>\n</body>\n</html>\n");

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.1 200 OK\n");
    send(connfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html; charset=UTF-8\n\n");
	send(connfd, buf, strlen(buf), 0);
    send(connfd, body, strlen(body), 0);
	return NULL;
}