#include <iostream>
#include <stdio.h>
#include <cstring>
#include <string>
#include <winsock2.h>
#include <Windows.h>
#include <WS2tcpip.h>

using namespace std;

#pragma comment(lib,"Ws2_32.lib")

SOCKET sockClient;
HANDLE bufferMutex;
#define PORT 4000
#define IP_ADDRESS "127.0.0.1"

int main(){
	WORD versionRequest = MAKEWORD(2, 2);
	WSADATA ws;
	int err = WSAStartup(versionRequest, &ws);
	if (err != 0){
		return -1;
	}

	sockClient = socket(AF_INET, SOCK_STREAM, 0);
	if (sockClient == INVALID_SOCKET){
		cout << "failed:" << GetLastError() << endl;
		return -1;
	}

	SOCKADDR_IN addrsrv;
	addrsrv.sin_addr.S_un.S_addr = inet_pton(AF_INET, IP_ADDRESS, (void *)&addrsrv);;
	addrsrv.sin_family = AF_INET;
	addrsrv.sin_port = htons(PORT);

	while (SOCKET_ERROR == connect(sockClient, (SOCKADDR*)&addrsrv, sizeof(SOCKADDR))){
		cout << "服务器连接失败，是否重新连接？（Y/N):";
		char choice;
		while (cin >> choice && (!((choice != 'Y' && choice == 'N') || (choice == 'Y' && choice != 'N')))){
			cout << "输入错误，请重新输入:";
			cin.sync();
			cin.clear();
		}
		if (choice == 'Y'){
			continue;
		}
		else{
			cout << "退出系统中...";
			system("pause");
			return 0;
		}
	}
	cin.sync();
	cout << "本客户端已准备就绪，用户可直接输入文字向服务器反馈信息。\n";
	cout << 1 << endl;
	send(sockClient, "\nAttention: a client has enter...\n", 200, 0);
	cout << 2 << endl;
	bufferMutex = CreateSemaphore(NULL, 1, 1, NULL);
	DWORD WINAPI SendMessageThread(LPVOID lpParameter);
	DWORD WINAPI ReceiveMessageThread(LPVOID lpParameter);

	HANDLE sendThread = CreateThread(NULL, 0, SendMessageThread, NULL, 0, NULL);
	HANDLE recvThread = CreateThread(NULL, 0, ReceiveMessageThread, NULL, 0, NULL);

	WaitForSingleObject(sendThread, INFINITE);
	closesocket(sockClient);
	CloseHandle(sendThread);
	CloseHandle(recvThread);
	CloseHandle(bufferMutex);
	WSACleanup();

	cout << "End Linking..." << endl << endl;

	system("pause");
	return 0;
}


DWORD WINAPI SendMessageThread(LPVOID lpParameter){
	while (1){
		string talk;
		getline(cin, talk);
		WaitForSingleObject(bufferMutex, INFINITE);
		if ("quit" == talk){
			talk.push_back('\0');
			send(sockClient, talk.c_str(), 200, 0);
			break;
		}
		else{
			talk.append("\n");
		}
		cout << "\nI Say:(\"quit\"to exit):" << talk << endl;
		send(sockClient, talk.c_str(), 200, 0);
		ReleaseSemaphore(bufferMutex, 1, NULL);
	}
	return 0;
}
DWORD WINAPI ReceiveMessageThread(LPVOID lpParameter){
	while (1){
		char recvbuf[300];
		recv(sockClient, recvbuf, 200, 0);
		WaitForSingleObject(bufferMutex, INFINITE);

		printf("%s Says: %s", "Server", recvbuf);       // 接收信息 

		ReleaseSemaphore(bufferMutex, 1, NULL);
	}
	return 0;
}
