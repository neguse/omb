#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
extern "C" {
#include <ae.h>
#include <anet.h>
}
#include <cstdio>
#include <unistd.h>
#include <cstring>

static const int TIMER_INTERVAL_MS = 5000;

char err[ANET_ERR_LEN];

void socketProc(aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
	printf("socketProc\n");
	char buf[128];
	int readed = anetRead(fd, buf, 1);
	if (readed <= 0) {
		printf("closed\n");
		close(fd);
		aeDeleteFileEvent(eventLoop, fd, mask);
		return;
	}
	buf[readed] = '\0';
	printf("received %s\n", buf);
}

void listenProc(aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
	printf("listenProc\n");
	char ip[128];
	int port = 0;
	int sock = anetTcpAccept(err, fd, ip, sizeof(ip), &port);
	if (sock == ANET_ERR) {
		printf("failed to accept : %s\n", err);
		return;
	}
	printf("connected from %s %d\n", ip, port);
	if (aeCreateFileEvent(eventLoop, sock, AE_READABLE, socketProc, NULL) == AE_ERR) {
		printf("failed to create file event\n");
		return;
	}
}

int timeProc(aeEventLoop *eventLoop, long long id, void *clientData) {
	printf("elapsed!\n");
	return TIMER_INTERVAL_MS;
}

int main(void) {
	int setsize = 10;
	aeEventLoop *eventLoop = aeCreateEventLoop(setsize);
	void* clientdata = NULL;
	aeCreateTimeEvent(eventLoop, TIMER_INTERVAL_MS, timeProc, clientdata, NULL);

	char* bindaddr = "127.0.0.1";
	int listensock = anetTcpServer(err, 3000, bindaddr, 128);
	if (listensock == ANET_ERR) {
		printf("failed to create server : %s", err);
		return -1;
	}
	aeCreateFileEvent(eventLoop, listensock, AE_READABLE, listenProc, NULL);

	aeMain(eventLoop);
	printf("exit\n");
	return 0;
}

