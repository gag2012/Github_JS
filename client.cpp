#include "winsock2.h"
#include "ws2tcpip.h"
#include "stdio.h"
#define SERVER_PORT 24680  // server port number
#define BUF_SIZE 4096 // block transfer size  
#define QUEUE_SIZE 10
#define IPAddress "165.246.38.151" // server IP address

void download(int s, int n, char buf[]);
void upload(int s, int n, char buf[]);

int main()
{
	WORD		        wVersionRequested;
	WSADATA		wsaData;
	SOCKADDR_IN          target; //Socket address information
	SOCKET		        s;
	int			err;
	int			bytesSent;
	char		        buf[100];

	//--- INITIALIZATION -----------------------------------
	wVersionRequested = MAKEWORD(1, 1);
	err = WSAStartup(wVersionRequested, &wsaData);

	if (err != 0) {
		printf("WSAStartup error %ld", WSAGetLastError());
		WSACleanup();
		return false;
	}
	//------------------------------------------------------

	//---- Build address structure to bind to socket.--------  
	target.sin_family = AF_INET; // address family Internet
	target.sin_port = htons(SERVER_PORT); //Port to connect on
	inet_pton(AF_INET, IPAddress, &(target.sin_addr.s_addr)); // target IP
															  //--------------------------------------------------------


															  // ---- create SOCKET--------------------------------------
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //Create socket
	if (s == INVALID_SOCKET)
	{
		printf("socket error %ld", WSAGetLastError());
		WSACleanup();
		return false; //Couldn't create the socket
	}
	//---------------------------------------------------------


	//---- try CONNECT -----------------------------------------
	if (connect(s, (SOCKADDR *)&target, sizeof(target)) == SOCKET_ERROR)
	{
		printf("connect error %ld", WSAGetLastError());
		WSACleanup();
		return false; //Couldn't connect
	}

	//protocol test
	send(s, "hello", 5, 0);

	//recieve bytes from server
	int n;
	n = recv(s, buf, 50, 0);
	buf[n] = 0; // make a string
	printf("%s\n", buf);

	//---- SEND bytes -------------------------------------------
	gets_s(buf, 99);
	send(s, buf, strlen(buf), 0); // use "send" in windows
	
	if (strcmp(buf, "download") == 0) download(s, n, buf);
	else if (strcmp(buf, "upload") == 0) upload(s, n, buf);
	else {
		printf("protocol error");
		bytesSent = send(s, "protocol error try again", 20, 0);
	}

	//--------------------------------------------------------
	closesocket(s);
	WSACleanup();

	return 0;
}

void download(int s, int n, char buf[]) {
	int byteReceived;
	int rv, cnt;
	struct timeval timeout;
	fd_set rd_set;
	FD_ZERO(&rd_set);
	FD_SET(s, &rd_set);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;


	//----SEND FILE NAME----------------------------------
	n = recv(s, buf, 50, 0);
	buf[n] = 0;
	printf("%s\n", buf); //receive

	gets_s(buf, 99);
	send(s, buf, strlen(buf), 0);

    //---DOWNLOADING--------------------------------------
	char filepath[50];
	sprintf_s(filepath, sizeof(filepath), "%s.txt", buf);

	errno_t err;
	FILE *input = NULL;
	err = fopen_s(&input, filepath, "w+");

	printf("downloading....\n");
	for (cnt=0; cnt<30; cnt++) {
		FD_SET(s, &rd_set);
		rv = select(s + 1, &rd_set, NULL, NULL, &timeout);
		if (rv == -1) {
			perror("select");
		}
		else if (rv == 0) {
			break;
		}
		else {
			if (FD_ISSET(s, &rd_set)) {
				byteReceived = recv(s, buf, 1, 0);
				if (strcmp(buf, "error_1") == 0) {
					printf("can't find filename\n");
					fclose(input);
					return;
				}
				fwrite(buf, 1, 1, input);
			}
		}
	}
	printf("done!\n");
	fclose(input);
}

void upload(int s, int n, char buf[]) {
	int byteSended;
	int cnt = 0;

	//----SEND FILE NAME----------------------------------
	n = recv(s, buf, 49, 0);
	buf[n] = 0;
	printf("%s\n", buf); 
	gets_s(buf, 99);
	send(s, buf, strlen(buf), 0);

	//----UPLOADING---------------------------------
	char content[50];
	memset(content, 0, sizeof(content));

	printf("test : %s\n", buf);
	
	errno_t err;
	FILE *output = NULL;
	err = fopen_s(&output, buf, "r");

	printf("uploading....\n");
	for (;;) {
		byteSended = fread(content, 1, 1, output);
		if (byteSended == 0) break;
		send(s, content, strlen(content), 0);
	}
	printf("done!\n");

	fclose(output);
}