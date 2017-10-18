#include <winsock2.h>
#include <iostream>
using namespace std;

#define PORT 4000
//#define MSGSIZE 1024

#pragma comment(lib,"ws2_32.lib")

int g_iTotalConn1 = 0;
int g_iTotalConn2 = 0;

SOCKET g_cliSocketArr1[FD_SETSIZE];
SOCKET g_cliSocketArr2[FD_SETSIZE];

DWORD WINAPI WorkerThread1(LPVOID lpParam);
int CALLBACK ConditionFunc1(LPWSABUF lpCallerId, LPWSABUF lpCallerData, LPQOS lpSQOS, LPQOS lpGQOS, LPWSABUF lpCalleeId, LPWSABUF lpCalleeData, GROUP FAR * g, DWORD dwCallbackData);
// DWORD WINAPI WorkerThread2(LPVOID lpParam);
// int CALLBACK ConditionFunc2(LPWSABUF lpCallerId, LPWSABUF lpCallerData, LPQOS lpSQOS, LPQOS lpGQOS, LPWSABUF lpCalleeId, LPWSABUF lpCalleeData, GROUP FAR * g, DWORD dwCallbackData);

int main(){
	WSADATA wsaData;
	SOCKET sListen, sClient;
	SOCKADDR_IN local, client;
	int iAddrsize = sizeof(SOCKADDR_IN);
	DWORD dwThreadId;

	//初始化
	WSAStartup(0x0202, &wsaData);

	//创建监听端口
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//绑定
	local.sin_family = AF_INET;
	local.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	local.sin_port = htons(PORT);
	bind(sListen, (sockaddr*)&local, sizeof(SOCKADDR_IN));

	//监听
	listen(sListen, 3);

	//创建工作者线程
	CreateThread(NULL, 0, WorkerThread1, NULL, 0, &dwThreadId);
	//CreateThread(NULL, 0, WorkerThread1, NULL, 0, &dwThreadId);

	while (true){
		sClient = WSAAccept(sListen, (sockaddr*)&client, &iAddrsize, ConditionFunc1, 0);
		cout << "1:Accepted client:" << inet_ntoa(client.sin_addr) << ntohs(client.sin_port) << g_iTotalConn1 << endl;
		g_cliSocketArr1[g_iTotalConn1++] = sClient;
	}

	return 0;
}

DWORD WINAPI WorkerThread1(LPVOID lpParam){
	int i;
	fd_set fdread;
	int ret;
	struct timeval tv = { 1, 0 };
	char szMessage[MAX_PATH];
	while (true){
		FD_ZERO(&fdread);	//1清空队列
		for (i = 0; i < g_iTotalConn1; i++){
			FD_SET(g_cliSocketArr1[i], &fdread);	//2将要检查的套接字加入队列
		}

		ret = select(0, &fdread, NULL, NULL, &tv);	//3查询满足要求的套接字，不满足要求，出队 
		if (ret == 0){
			continue;
		}

		for (i = 0; i < g_iTotalConn1; i++){
			if (FD_ISSET(g_cliSocketArr1[i], &fdread)){
				ret = recv(g_cliSocketArr1[i], szMessage, MAX_PATH, 0);
				if (ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)){
					cout << "1:Client socket " << g_cliSocketArr1[i] << " closed" << endl;
					closesocket(g_cliSocketArr1[i]);
					if (i < g_iTotalConn1 - 1)
						g_cliSocketArr1[i--] = g_cliSocketArr1[--g_iTotalConn1];
				}
				else{
					szMessage[ret] = '\0';
					send(g_cliSocketArr1[i], szMessage, strlen(szMessage), 0);
				}
				cout << "client:" << szMessage << endl;
			}
		}
	}
}

int CALLBACK ConditionFunc1(LPWSABUF lpCallerId, LPWSABUF lpCallerData, LPQOS lpSQOS, LPQOS lpGQOS, LPWSABUF lpCalleeId, LPWSABUF lpCalleeData, GROUP FAR * g, DWORD dwCallbackData){
	if (g_iTotalConn1 < FD_SETSIZE)
		return CF_ACCEPT;
	else
		return CF_REJECT;
}