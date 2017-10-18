#include <Windows.h>
#include <winsock.h>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define PORT 4000
#define IP_ADDRESS "127.0.0.1"
DWORD WINAPI ServerThread(LPVOID lpParameter){
	SOCKET ServerSocket = (SOCKET)lpParameter;
	char SendBuffer[MAX_PATH];
	int ret = 0;

	while (true){
		memset(SendBuffer, 0x00, sizeof(SendBuffer));
		cin.getline(SendBuffer, sizeof(SendBuffer));
		ret = send(ServerSocket, SendBuffer, (int)strlen(SendBuffer), 0);
		if (ret == SOCKET_ERROR){
			cout << "send info error::" << GetLastError() << endl;
			break;
		}
	}

	return 0;
}
DWORD WINAPI RecvThread(LPVOID lpParameter){
	SOCKET ClientSocket = (SOCKET)lpParameter;
	int ret = 0;
	char RecvBuffer[MAX_PATH];

	while (true){
		memset(RecvBuffer, 0x00, sizeof(RecvBuffer));
		ret = recv(ClientSocket, RecvBuffer, MAX_PATH, 0);
		if (ret == 0 || ret == SOCKET_ERROR){
			cout << "客户端退出！" << endl;
			break;
		}
		cout << "接收到客户端的消息为:" << RecvBuffer << endl;
	}

	return 0;
}

int main(){
	WSADATA Ws;
	SOCKET ServerSocket, ClientSocket;
	struct sockaddr_in LocalAddr, ClientAddr;
	int ret = 0;
	int AddrLen = 0;
	HANDLE hThread = NULL;
	HANDLE send_hThread = NULL;
	
	//初始化Windows端口
	if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0){
		cout << "Init Windows Socket failed::" << GetLastError() << endl;
		return -1;
	}

	//创建端口
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ServerSocket == INVALID_SOCKET){
		cout << "Create socket failed::" << GetLastError() << endl;
		return -1;
	}

	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	LocalAddr.sin_port = htons(PORT);
	memset(LocalAddr.sin_zero, 0x00, 8);

	//绑定端口
	ret = bind(ServerSocket, (struct sockaddr*)&LocalAddr, sizeof(LocalAddr));
	if (ret != 0){
		cout << "bind socket failed::" << GetLastError() << endl;
		return -1;
	}

	ret = listen(ServerSocket, 10);
	if (ret != 0){
		cout << "listen socket failed::" << GetLastError() << endl;
		return -1;
	}

	cout << "服务端已经启动" << endl;

	while (true){
		AddrLen = sizeof(ClientAddr);
		ClientSocket = accept(ServerSocket, (struct sockaddr*)&ClientAddr, &AddrLen);
		if (ClientSocket == INVALID_SOCKET){
			cout << "accept failed::" << GetLastError() << endl;
			break;
		}
		cout << "客户端连接::" << inet_ntoa(ClientAddr.sin_addr) << ":" << ClientAddr.sin_port << endl;

		send_hThread = CreateThread(NULL, 0, ServerThread, (LPVOID)ClientSocket, 0, NULL);
		if (send_hThread == NULL){
			cout << "create thread failed" << endl;
			break;
		}
		hThread = CreateThread(NULL, 0, RecvThread, (LPVOID)ClientSocket, 0, NULL);
		if (hThread == NULL){
			cout << "create thread failed" << endl;
			break;
		}
		
		CloseHandle(hThread);
		CloseHandle(send_hThread);
	}
	closesocket(ServerSocket);
	closesocket(ClientSocket);
	WSACleanup();

	system("pause");
	return 0;
}