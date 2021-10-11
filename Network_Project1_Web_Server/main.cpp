/*
	Network project #1 Web server
	2021.10.04.
*/

#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <string.h>

#define DEFAULT_SIZE 4096
#define STATUS_LENGTH 40
#define MIME_LENGTH 40
#define SERVER_ERROR_MSG "500 Internal server errors occur."
#define PAGE_NOT_FOUND "400 Page not found."

#pragma comment (lib, "ws2_32.lib")

int InitializeWinsock();
void BindSock(SOCKET, int);
void GetLocalURI(char*, const char*, const char*, size_t);
void GetMimeType(char* const, char* const);
void GetStatusText(char* statusText, int statusCode);
int FillHeader(SOCKET, char*, const char*, int, int);
void SendFailMsg(SOCKET, int);
void* HandleHttp(void*);


int main()
{
	// Initialize winsock
	if (InitializeWinsock() == 0)
		return 0;

	// Create a socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		perror("Can't create a socket! Quitting\n");
		return 0;
	}
	
	// Bind socket
	BindSock(listening, 54000);

	// Tell Winsock the socket is for listening
	listen(listening, SOMAXCONN);

	sockaddr_in client;
	int clientSize = sizeof(client);
	SOCKET clientSocket;

	// 연결 요청이 들어오면 accept로 client socket을 생성한 다음 별도 스레드에서 이를 처리(httpHandle)한다.
	while (1)
	{
		// Wait for a connection
		clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
		pthread_t p_thread;
		pthread_create(&p_thread, NULL, HandleHttp, (void*)clientSocket);
	}

	if (shutdown(clientSocket, SD_BOTH) != 0)
	{
		perror("shutdown\n");
	}
	
	// Close listening socket
	closesocket(listening);

	// Cleanup winsock
	WSACleanup();

	return 0;
}

int InitializeWinsock()
{
	// Initialize winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0)
	{
		perror("Can't Initialize winsock! Quitting\n");
		return 0;
	}
	return 1;
}

void BindSock(SOCKET listeningSock, int port)
{
	// Bind the ip address and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; // Could also use inet_pton .... 

	bind(listeningSock, (sockaddr*)&hint, sizeof(hint));
}

// 상대 경로명을 가져온다.
void GetLocalURI(char* fileURI, const char* directory, const char* recvBuf, size_t sizeOfFileBuf)
{
	// 요구하는 파일 경로 분석
	int indexOfEnd, length = strlen(recvBuf), spaceCount = 0;
	for (indexOfEnd = 0; indexOfEnd < length; indexOfEnd++) // 두 번째 띄어쓰기를 찾는다.
	{
		if (recvBuf[indexOfEnd] == ' ')
		{
			spaceCount++;
			if (spaceCount == 2)
				break;
		}
	}
	indexOfEnd--;
	if (indexOfEnd == 4) // 아무것도 입력하지 않으면 index.html을 불러온다.
		//sprintf_s(fileNamePtr, sizeOfFileBuf, "%s\\index.html", directory);
		sprintf_s(fileURI, sizeOfFileBuf, "%s%s", directory, "index.html");
	else
	{
		int count = sprintf_s(fileURI, sizeOfFileBuf, "%s", directory);
		strncpy_s(fileURI + count, sizeOfFileBuf - count, recvBuf + 5, (size_t)(indexOfEnd)-4);
	}
}

void GetMimeType(char* const mimeType, char* const fileURI)
{
	char* type = strchr(fileURI, '.');
	// 현재 서비스 타입: .txt, .html, .htm, .jpeg, .jpg, .gif, .mp3, .mp4, .wav
	if (strcmp(type, ".txt") == 0)
	{
		strcpy_s(mimeType, MIME_LENGTH, "text/plain");
	}
	else if (strcmp(type, ".html") == 0 || strcmp(type, ".htm") == 0)
	{
		strcpy_s(mimeType, MIME_LENGTH, "text/html");
	}
	else if (strcmp(type, ".jpeg") == 0 || strcmp(type, ".jpg") == 0)
	{
		strcpy_s(mimeType, MIME_LENGTH, "image/jpeg");
	}
	else if (strcmp(type, ".png") == 0)
	{
		strcpy_s(mimeType, MIME_LENGTH, "image/png");
	}
	else if (strcmp(type, ".gif") == 0)
	{
		strcpy_s(mimeType, MIME_LENGTH, "image/gif");
	}
	else if (strcmp(type, ".mp3") == 0 || strcmp(type, ".mp4") == 0)
	{
		strcpy_s(mimeType, MIME_LENGTH, "audio/mpeg");
	}
	else if (strcmp(type, ".wav") == 0)
	{
		strcpy_s(mimeType, MIME_LENGTH, "audio/x-wav");
	}
	else
	{
		strcpy_s(mimeType, MIME_LENGTH, "application/octet-stream");
	}
}

void GetStatusText(char* statusText, int statusCode)
{
	switch (statusCode)
	{
		case 200:
			strcpy_s(statusText, STATUS_LENGTH, "200 OK");
			break;
		case 404:
			strcpy_s(statusText, STATUS_LENGTH, "404 Not Found");
			break;
		case 500:
			strcpy_s(statusText, STATUS_LENGTH, "500 Internal Server Error");
			break;
	}
}

int FillHeader(SOCKET clientSock, char* msgBuf, const char* mimeType, int statusCode, int fileSize)
{
	// status code 획득
	char statusText[STATUS_LENGTH];
	int length;
	const char* header_fmt =
		"HTTP/1.1 %s\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: %s\r\n"
		"Connection: keep-alive\r\n"
		"\r\n";
	char* contentType;

	GetStatusText(statusText, statusCode);

	length = sprintf_s(msgBuf, DEFAULT_SIZE,
		"HTTP/1.1 %s\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: %s\r\n"
		"Connection: keep-alive\r\n"
		"\r\n",
		statusText,
		fileSize,
		mimeType);

	return length;
}

void SendFailMsg(SOCKET clientSock, int status)
{
	char msgBuf[DEFAULT_SIZE], body[DEFAULT_SIZE];
	int bytesToSend, headerLength, bodyLength;

	switch (status)
	{
		case 404:
			strcpy_s(body, DEFAULT_SIZE, PAGE_NOT_FOUND);
			break;
		case 500:
			strcpy_s(body, DEFAULT_SIZE, SERVER_ERROR_MSG);
			break;
		default:
			body[0] = '\0';
			break;
	}

	bodyLength = strlen(body);
	headerLength = FillHeader(clientSock, msgBuf, "text/html", status, bodyLength);
	bytesToSend = headerLength + bodyLength + 1;

	memcpy_s(msgBuf + headerLength, (size_t)DEFAULT_SIZE, body, bodyLength);
	_snprintf_s(msgBuf + bytesToSend - 1, 1, 1, "\0");

	int iResult = send(clientSock, msgBuf, bytesToSend, 0);

	if (iResult == SOCKET_ERROR)
	{
		perror("Error in send(). Quitting\n");
	}
	closesocket(clientSock);
}

void* HandleHttp(void* arg)
{
	SOCKET clientSock = (SOCKET)arg;
	char buf[DEFAULT_SIZE], localURI[DEFAULT_SIZE], *fbuf, *msgBuf;
	const char* directory = "contents\\";
	int index;

	ZeroMemory(buf, DEFAULT_SIZE);

	// Wait for client to send data
	int bytesReceived = recv(clientSock, buf, DEFAULT_SIZE, 0);
	if (bytesReceived == SOCKET_ERROR)
	{
		perror("Error in recv(). Quitting\n");
		SendFailMsg(clientSock, 500); // 500
		return NULL;
	}

	printf("%s", buf);

	if (strstr(buf, "GET") == buf) // GET
	{
		int fileSize, headerLength;
		ZeroMemory(localURI, DEFAULT_SIZE);
		GetLocalURI(localURI, directory, buf, DEFAULT_SIZE); // 파일 이름 획득

		//printf("==requested fileName = %s...==\n\n", fileName);

		// 파일을 읽고 fbuf에 출력
		FILE* fp;
		fopen_s(&fp, localURI, "rb");

		if (fp == NULL)
		{
			// 파일 열기 실패
			perror("cannot open the file...\n\n");
			SendFailMsg(clientSock, 404); // 404 NOT FOUND
			return NULL;
		}

		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp);

		index = 0;
		msgBuf = (char*)malloc(sizeof(char) * DEFAULT_SIZE); // 기본 크기: DEFAULT_SIZE

		if (msgBuf == NULL)
		{
			// 에러 발생
			perror("error occurs\n");
			SendFailMsg(clientSock, 500); // 500
			free(msgBuf);
			fclose(fp);
			return NULL;
		}

		ZeroMemory(msgBuf, DEFAULT_SIZE);

		// Header
		char mimeType[MIME_LENGTH];
		GetMimeType(mimeType, localURI);
		headerLength = FillHeader(clientSock, msgBuf, mimeType, 200, fileSize);

		// Body
		fbuf = (char*)malloc(sizeof(char) * fileSize);

		if (fbuf == NULL)
		{
			SendFailMsg(clientSock, 500); // 500
			free(fbuf);
			free(msgBuf);
			fclose(fp);
			return NULL;
		}
		fseek(fp, 0, SEEK_SET); // 위치 초기화
		fread(fbuf, sizeof(char), fileSize, fp);

		// Resize
		int bytesToSend = headerLength + fileSize + 1;
		if (bytesToSend > DEFAULT_SIZE)
		{
			char* oldBuf = msgBuf;
			msgBuf = (char*)realloc(msgBuf, sizeof(char) * bytesToSend);
			if (msgBuf == NULL)
			{
				// Fail to resize msg buffer.
				SendFailMsg(clientSock, 500); // 500
				free(fbuf);
				free(oldBuf);
				fclose(fp);
				return NULL;
			}
		}

		// Copy memory
		memcpy_s(msgBuf + headerLength, (size_t)bytesToSend, fbuf, fileSize);
		_snprintf_s(msgBuf + headerLength + fileSize, 1, 1, "\0");

		int iResult = send(clientSock, msgBuf, bytesToSend, 0);

		if (iResult == SOCKET_ERROR)
		{
			perror("Error in send(). Quitting\n");
			closesocket(clientSock);
		}

		//printf("Bytes sent: %d\n", iResult);

		free(fbuf);
		free(msgBuf);
		fclose(fp);
	}

	// Close the socket
	closesocket(clientSock);
}