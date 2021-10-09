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

#pragma comment (lib, "ws2_32.lib")

void GetFileName(char*, const char*, const char*, size_t);
int PrintAllContentType(char* const, char* const, int);
void* HandleHttp(void*);


int main()
{
	// 1. Initialize winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0)
	{
		perror("Can't Initialize winsock! Quitting\n");
		return 0;
	}

	// Create a socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		perror("Can't create a socket! Quitting\n");
		return 0;
	}

	// Bind the ip address and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; // Could also use inet_pton .... 

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// Tell Winsock the socket is for listening
	listen(listening, SOMAXCONN);

	sockaddr_in client;
	int clientSize = sizeof(client);

	SOCKET clientSocket;

	// 이제 연결 요청이 들어오면 accept로 client socket을 생성한 다음 별도 스레드에서 이를 처리(httpHandle)하도록 수정한다.
	while (1)
	{
		// Wait for a connection
		clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

		pthread_t p_thread;

		pthread_create(&p_thread, NULL, HandleHttp, (void*)clientSocket);
	}

	if (shutdown(clientSocket, SD_BOTH) != 0) {
		perror("shutdown 오류\n");
	}
	
	// Close listening socket
	closesocket(listening);

	// Cleanup winsock
	WSACleanup();

	return 0;
}

void GetFileName(char* fileNamePtr, const char* directory, const char* recvBuf, size_t sizeOfFileBuf)
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
		sprintf_s(fileNamePtr, sizeOfFileBuf, "%s%s", directory, "index.html");
	else
	{
		int count = sprintf_s(fileNamePtr, sizeOfFileBuf, "%s", directory);
		strncpy_s(fileNamePtr + count, sizeOfFileBuf - count, recvBuf + 5, (size_t)(indexOfEnd)-4);
	}
}

int PrintContentTypeInnerFunc(char* const msgBuf, const char* mimeType, int indexOfMsg)
{
	return sprintf_s(msgBuf + indexOfMsg, (size_t)DEFAULT_SIZE - indexOfMsg - 1, "Content-Type: %s\r\n", mimeType);
}

int PrintAllContentType(char* const msgBuf, char* const fileURL, int indexOfMsg)
{
	int count;
	char* type = strchr(fileURL, '.');
	// 현재 서비스 타입: .txt, .html, .htm, .jpeg, .jpg, .gif
	if (strcmp(type, ".txt") == 0)
	{
		count = PrintContentTypeInnerFunc(msgBuf, "text/plain", indexOfMsg);
	}
	else if (strcmp(type, ".html") == 0 || strcmp(type, ".htm") == 0)
	{
		count = PrintContentTypeInnerFunc(msgBuf, "text/html", indexOfMsg);
	}
	else if (strcmp(type, ".jpeg") == 0 || strcmp(type, ".jpg") == 0)
	{
		count = PrintContentTypeInnerFunc(msgBuf, "image/jpeg", indexOfMsg);
	}
	else if (strcmp(type, ".png") == 0)
	{
		count = PrintContentTypeInnerFunc(msgBuf, "image/png", indexOfMsg);
	}
	else if (strcmp(type, ".gif") == 0)
	{
		count = PrintContentTypeInnerFunc(msgBuf, "image/gif", indexOfMsg);
	}
	else if (strcmp(type, ".mp3") == 0 || strcmp(type, ".mp4") == 0)
	{
		count = PrintContentTypeInnerFunc(msgBuf, "audio/mpeg", indexOfMsg);
	}
	else if (strcmp(type, ".wav") == 0)
	{
		count = PrintContentTypeInnerFunc(msgBuf, "audio/x-wav", indexOfMsg);
	}
	else
	{
		count = PrintContentTypeInnerFunc(msgBuf, "application/octet-stream", indexOfMsg);
	}

	// 반납 타입: 글자 수
	return count;
}

void* HandleHttp(void* arg)
{
	SOCKET clientSock = (SOCKET)arg;
	char buf[DEFAULT_SIZE], fileName[DEFAULT_SIZE], *fbuf, *msgBuf;
	const char* directory = "contents\\";
	int index;

	ZeroMemory(buf, DEFAULT_SIZE);

	// Wait for client to send data
	int bytesReceived = recv(clientSock, buf, DEFAULT_SIZE, 0);
	if (bytesReceived == SOCKET_ERROR)
	{
		perror("Error in recv(). Quitting\n");
		closesocket(clientSock);
		return NULL;
	}

	printf("%s", buf);

	if (strstr(buf, "GET") == buf) // GET
	{
		int fileSize, headerLength;
		ZeroMemory(fileName, DEFAULT_SIZE);
		GetFileName(fileName, directory, buf, DEFAULT_SIZE); // 파일 이름 획득

		printf("==requested fileName = %s...==\n\n", fileName);

		// 파일을 읽고 fbuf에 출력
		FILE* fp;
		fopen_s(&fp, fileName, "rb");

		if (fp == NULL)
		{
			// 파일 열기 실패
			perror("cannot open the file...\n\n");
			closesocket(clientSock);
			return NULL; // 수정 요함
		}

		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp);

		index = 0;
		msgBuf = (char*)malloc(sizeof(char) * DEFAULT_SIZE); // 기본 크기: DEFAULT_SIZE

		if (msgBuf == NULL)
		{
			// 에러 발생
			perror("error occurs\n");
			closesocket(clientSock);
			return NULL;
		}

		ZeroMemory(msgBuf, DEFAULT_SIZE);

		// Header
		index = sprintf_s(msgBuf, DEFAULT_SIZE, "HTTP/1.1 200 OK\r\n"); // 상태 메시지
		index += sprintf_s(msgBuf + index, (size_t)DEFAULT_SIZE - index - 1,
			"Content-Length: %d\r\n", fileSize);						// 본문의 길이		
		index += PrintAllContentType(msgBuf, fileName, index);			// 본문의 형식
		index += sprintf_s(msgBuf + index, (size_t)DEFAULT_SIZE - index - 1, 
			"Connection: keep-alive\r\n\r\n");							// 연결 형식
		headerLength = index;

		// Body
		fbuf = (char*)malloc(sizeof(char) * fileSize);

		if (fbuf == NULL)
			return NULL;
		fseek(fp, 0, SEEK_SET); // 위치 초기화
		fread(fbuf, sizeof(char), fileSize, fp);

		int bytesToSend = headerLength + fileSize + 1;
		if (bytesToSend > DEFAULT_SIZE)
		{
			char* oldBuf = msgBuf;
			msgBuf = (char*)realloc(msgBuf, sizeof(char) * bytesToSend);
			if (msgBuf == NULL)
			{
				free(fbuf);
				free(oldBuf);
				fclose(fp);
				closesocket(clientSock);
				return NULL;
			}
		}

		// 문자열이 아닐 경우, 중간에 NULL 문자열이 있어서 formating 형식으로 복제하는 것으로는 부족할 수도 있다. 따라서 한 바이트 한 바이트씩 출력하는 것이 필요하다.
		memcpy_s(msgBuf + headerLength, (size_t)bytesToSend, fbuf, fileSize);
		_snprintf_s(msgBuf + fileSize, 1, 1, "\0");

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