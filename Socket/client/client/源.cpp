#include <iostream>
#include <Windows.h>
#include <winsock.h>

#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define PORT 4000
#define IP_ADDRESS "127.0.0.1"

DWORD WINAPI RecvThread(LPVOID lpParameter){
	SOCKET ClientSocket = (SOCKET)lpParameter;
	int ret = 0;
	char RecvBuffer[MAX_PATH];

	while (true){
		memset(RecvBuffer, 0x00, sizeof(RecvBuffer));
		ret = recv(ClientSocket, RecvBuffer, MAX_PATH, 0);
		if (ret == 0 || ret == SOCKET_ERROR){
			cout << "服务端退出！" << endl;
			break;
		}
		cout << "接收到服务端的消息为:" << RecvBuffer << endl;
	}

	return 0;
}

int main(){
	WSADATA Ws;
	SOCKET ClientSocket;
	struct sockaddr_in ServerAddr;
	int ret = 0;
	int AddrLen = 0;
	HANDLE hThread = NULL;
	char SendBuffer[MAX_PATH];

	//初始化Windows端口
	if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0){
		cout << "Init Windows Socket Failed::" << GetLastError() << endl;
		return -1;
	}

	//创建端口
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET){
		cout << "create socket failed::" << GetLastError() << endl;
		return -1;
	}

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	ServerAddr.sin_port = htons(PORT);
	memset(ServerAddr.sin_zero, 0x00, 8);

	ret = connect(ClientSocket, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
	if (ret == SOCKET_ERROR){
		cout << "connect error::" << GetLastError() << endl;
		return -1;
	}
	else{
		cout << "连接成功" << endl;
	}
	hThread = CreateThread(NULL, 0, RecvThread, (LPVOID)ClientSocket, 0, NULL);
	if (hThread == NULL){
		cout << "create thread failed" << endl;
	}
	while (true){
		cin.getline(SendBuffer, sizeof(SendBuffer));
		ret = send(ClientSocket, SendBuffer, (int)strlen(SendBuffer),0);
		if (ret == SOCKET_ERROR){
			cout << "send info error::" << GetLastError() << endl;
			break;
		}
	}
	closesocket(ClientSocket);
	WSACleanup();

	system("pause");
	return 0;
}