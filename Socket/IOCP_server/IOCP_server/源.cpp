#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include <iostream>

using namespace std;

#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib,"kernel32.lib")

const int DataBufSize = 2 * 1024;
/**
* 结构体名称：PER_IO_DATA
* 结构体功能：重叠I/O需要用到的结构体，临时记录IO数据
**/
typedef struct{
	OVERLAPPED overlapped;
	WSABUF databuf;
	char buffer[DataBufSize];
	int BufferLen;
	int operationtype;
}PER_IO_OPERATION_DATA,*LPPER_IO_OPERATION_DATA,*LPPER_IO_DATA,PER_IO_DATA;
/**
* 结构体名称：PER_HANDLE_DATA
* 结构体存储：记录单个套接字的数据，包括了套接字的变量及套接字的对应的客户端的地址。
* 结构体作用：当服务器连接上客户端时，信息存储到该结构体中，知道客户端的地址以便于回访。
**/
typedef struct{
	SOCKET socket;
	SOCKADDR_STORAGE ClientAddr;
}PER_HANDLE_DATA,*LPPER_HANDLE_DATA;

//全局变量
#define  PORT 4000
vector <PER_HANDLE_DATA*> clientGroup;

HANDLE hMutex = CreateMutex(NULL, false, NULL);
DWORD WINAPI ServerWorkThread(LPVOID lpParam);
DWORD WINAPI ServerSendThread(LPVOID lpParam);

int main(){
	//加载socket动态链接库
	WORD VersionReq = MAKEWORD(2, 2);
	WSADATA Ws;
	DWORD err = WSAStartup(VersionReq, &Ws);

	if (err != 0){
		cout << "error:" << GetLastError() << endl;
		system("pause");
		return -1;
	}

	// 创建IOCP的内核对象  
	/**
	* 需要用到的函数的原型：
	* HANDLE WINAPI CreateIoCompletionPort(
	*    __in   HANDLE FileHandle,     // 已经打开的文件句柄或者空句柄，一般是客户端的句柄
	*    __in   HANDLE ExistingCompletionPort, // 已经存在的IOCP句柄
	*    __in   ULONG_PTR CompletionKey,   // 完成键，包含了指定I/O完成包的指定文件
	*    __in   DWORD NumberOfConcurrentThreads // 真正并发同时执行最大线程数，一般推介是CPU核心数*2
	* );
	**/
	HANDLE CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == CompletionPort){
		cout << "CreateIoCompletionPort error:" << GetLastError() << endl;
		system("pause");
		return -1;
	}
	// 创建IOCP线程--线程里面创建线程池 
	//确定核心的数量
	SYSTEM_INFO systeminfo;
	GetSystemInfo(&systeminfo);

	//基于核心数量创建线程
	for (DWORD i = 0; i < (systeminfo.dwNumberOfProcessors * 2); i++){
		// 创建服务器工作器线程，并将完成端口传递到该线程 
		HANDLE ThreadHandle = CreateThread(NULL, 0, ServerWorkThread, CompletionPort, 0, NULL);
		if (NULL == ThreadHandle){
			cout << "Create Thread Handle failed. Error:" << GetLastError() << endl;
			system("pause");
			return -1;
		}
		CloseHandle(ThreadHandle);
	}
	// 建立流式套接字  
	SOCKET srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	
	//绑定
	SOCKADDR_IN srvaddr;
	srvaddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(PORT);
	int res = bind(srvSocket, (SOCKADDR*)&srvaddr, sizeof(SOCKADDR));
	if (res == SOCKET_ERROR){
		cout << "bind error:" << GetLastError() << endl;
		system("pause");
		return -1;
	}

	//监听
	res = listen(srvSocket, 10);
	if (res == SOCKET_ERROR){
		cout << "listen error:" << GetLastError() << endl;
		system("pause");
		return -1;
	}

	//开始处理io数据
	cout << "server已就绪，等待client接入..." << endl;

	HANDLE sendThread = CreateThread(NULL, 0, ServerSendThread, 0, 0,NULL);

	while (1){
		PER_HANDLE_DATA* Per_handle_data = NULL;
		SOCKADDR_IN saRemote;
		int RemoteLen = sizeof(saRemote);
		SOCKET acceptSocket = accept(srvSocket,(SOCKADDR*)&saRemote,&RemoteLen);
		if (acceptSocket == SOCKET_ERROR){
			cout << "accept error:" << GetLastError() << endl;
			system("pause");
			return -1;
		}

		// 创建用来和套接字关联的单句柄数据信息结构
		Per_handle_data = (LPPER_HANDLE_DATA)GlobalAlloc(GPTR, sizeof(PER_HANDLE_DATA));// 在堆中为这个PerHandleData申请指定大小的内存  
		Per_handle_data->socket = acceptSocket;
		memcpy(&Per_handle_data->ClientAddr, &saRemote, RemoteLen);
		clientGroup.push_back(Per_handle_data);// 将单个客户端数据指针放到客户端组中

		//将接受套接字和完成端口关联  
		CreateIoCompletionPort((HANDLE)(Per_handle_data->socket), CompletionPort, (DWORD)Per_handle_data, 0);

		// 开始在接受套接字上处理I/O使用重叠I/O机制  
		// 在新建的套接字上投递一个或多个异步  
		// WSARecv或WSASend请求，这些I/O请求完成后，工作者线程会为I/O请求提供服务      
		// 单I/O操作数据(I/O重叠) 
		LPPER_IO_OPERATION_DATA Per_io_data = (LPPER_IO_OPERATION_DATA)GlobalAlloc(GPTR, sizeof(PER_IO_OPERATION_DATA));
		ZeroMemory(&(Per_io_data->overlapped), sizeof(OVERLAPPED));
		Per_io_data->databuf.buf = Per_io_data->buffer;
		Per_io_data->operationtype = 0;	//read

		DWORD RecvBytes;
		DWORD flags;
		WSARecv(Per_handle_data->socket, &(Per_io_data->databuf), 1, &RecvBytes, &flags, &(Per_io_data->overlapped), NULL);
	}

	system("pause");
	return 0;
}


DWORD WINAPI ServerWorkThread(LPVOID IpParam){
	HANDLE CompletionPort = (HANDLE)IpParam;
	DWORD BytesTransferred;
	LPOVERLAPPED IpOverlapped;
	LPPER_HANDLE_DATA PerHandleData = NULL;
	LPPER_IO_DATA PerIoData = NULL;
	DWORD RecvBytes;
	DWORD flags = 0;
	BOOL bret = FALSE;

	while (1){
		bret = GetQueuedCompletionStatus(CompletionPort, &BytesTransferred, (PULONG_PTR)&PerHandleData, (LPOVERLAPPED*)&IpOverlapped, INFINITE);
		if (bret == 0){
			cout << "GetQueuedCompletionStatus error:" << GetLastError() << endl;
			system("pause");
			return -1;
		}
		PerIoData = (LPPER_IO_DATA)CONTAINING_RECORD(IpOverlapped, PER_IO_DATA, overlapped);

		// 检查在套接字上是否有错误发生  
		if (BytesTransferred == 0){
			closesocket(PerHandleData->socket);
			GlobalFree(PerHandleData);
			GlobalFree(PerIoData);
			continue;
		}

		// 开始数据处理，接收来自客户端的数据 
		WaitForSingleObject(hMutex, INFINITE);
		cout << "a client says:" << PerIoData->databuf.buf << endl;
		ReleaseMutex(hMutex);

		// 为下一个重叠调用建立单I/O操作数据 
		ZeroMemory(&(PerIoData->overlapped), sizeof(OVERLAPPED));//清空内存
		PerIoData->databuf.len = 1024;
		PerIoData->databuf.buf = PerIoData->buffer;
		PerIoData->operationtype = 0;	//read
		WSARecv(PerHandleData->socket,&(PerIoData->databuf),1,&RecvBytes,&flags,&(PerIoData->overlapped),NULL);
	}

	return 0;
}
DWORD WINAPI ServerSendThread(LPVOID IpParam){
	while (1){
		char talk[MAX_PATH];
		gets_s(talk);
		int len;
		for (len = 0; talk[len] != '\n'; ++len){
			//找出这个字符组的长度
		}
		talk[len] = '\n';
		talk[++len] = '\0';
		cout << "I say: " << talk << endl;
		WaitForSingleObject(hMutex, INFINITE);
		for (int i = 0; i < clientGroup.size(); ++i){
			send(clientGroup[i]->socket, talk, MAX_PATH, 0);	//发送信息
		}
		ReleaseMutex(hMutex);
	}

	return 0;
}