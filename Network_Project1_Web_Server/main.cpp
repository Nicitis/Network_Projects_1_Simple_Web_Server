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

	// ���� ���� ��û�� ������ accept�� client socket�� ������ ���� ���� �����忡�� �̸� ó��(httpHandle)�ϵ��� �����Ѵ�.
	while (1)
	{
		// Wait for a connection
		clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

		pthread_t p_thread;

		pthread_create(&p_thread, NULL, HandleHttp, (void*)clientSocket);
	}

	if (shutdown(clientSocket, SD_BOTH) != 0) {
		perror("shutdown ����\n");
	}
	
	// Close listening socket
	closesocket(listening);

	// Cleanup winsock
	WSACleanup();

	return 0;
}

void GetFileName(char* fileNamePtr, const char* directory, const char* recvBuf, size_t sizeOfFileBuf)
{
	// �䱸�ϴ� ���� ��� �м�
	int indexOfEnd, length = strlen(recvBuf), spaceCount = 0;
	for (indexOfEnd = 0; indexOfEnd < length; indexOfEnd++) // �� ��° ���⸦ ã�´�.
	{
		if (recvBuf[indexOfEnd] == ' ')
		{
			spaceCount++;
			if (spaceCount == 2)
				break;
		}
	}
	indexOfEnd--;
	if (indexOfEnd == 4) // �ƹ��͵� �Է����� ������ index.html�� �ҷ��´�.
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
	// ���� ���� Ÿ��: .txt, .html, .htm, .jpeg, .jpg, .gif
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

	// �ݳ� Ÿ��: ���� ��
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
		GetFileName(fileName, directory, buf, DEFAULT_SIZE); // ���� �̸� ȹ��

		printf("==requested fileName = %s...==\n\n", fileName);

		// ������ �а� fbuf�� ���
		FILE* fp;
		fopen_s(&fp, fileName, "rb");

		if (fp == NULL)
		{
			// ���� ���� ����
			perror("cannot open the file...\n\n");
			closesocket(clientSock);
			return NULL; // ���� ����
		}

		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp);

		index = 0;
		msgBuf = (char*)malloc(sizeof(char) * DEFAULT_SIZE); // �⺻ ũ��: DEFAULT_SIZE

		if (msgBuf == NULL)
		{
			// ���� �߻�
			perror("error occurs\n");
			closesocket(clientSock);
			return NULL;
		}

		ZeroMemory(msgBuf, DEFAULT_SIZE);

		// Header
		index = sprintf_s(msgBuf, DEFAULT_SIZE, "HTTP/1.1 200 OK\r\n"); // ���� �޽���
		index += sprintf_s(msgBuf + index, (size_t)DEFAULT_SIZE - index - 1,
			"Content-Length: %d\r\n", fileSize);						// ������ ����		
		index += PrintAllContentType(msgBuf, fileName, index);			// ������ ����
		index += sprintf_s(msgBuf + index, (size_t)DEFAULT_SIZE - index - 1, 
			"Connection: keep-alive\r\n\r\n");							// ���� ����
		headerLength = index;

		// Body
		fbuf = (char*)malloc(sizeof(char) * fileSize);

		if (fbuf == NULL)
			return NULL;
		fseek(fp, 0, SEEK_SET); // ��ġ �ʱ�ȭ
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

		// ���ڿ��� �ƴ� ���, �߰��� NULL ���ڿ��� �־ formating �������� �����ϴ� �����δ� ������ ���� �ִ�. ���� �� ����Ʈ �� ����Ʈ�� ����ϴ� ���� �ʿ��ϴ�.
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