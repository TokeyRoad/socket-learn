#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include <iostream>

using namespace std;

#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib,"kernel32.lib")

const int DataBufSize = 2 * 1024;
/**
* �ṹ�����ƣ�PER_IO_DATA
* �ṹ�幦�ܣ��ص�I/O��Ҫ�õ��Ľṹ�壬��ʱ��¼IO����
**/
typedef struct{
	OVERLAPPED overlapped;
	WSABUF databuf;
	char buffer[DataBufSize];
	int BufferLen;
	int operationtype;
}PER_IO_OPERATION_DATA,*LPPER_IO_OPERATION_DATA,*LPPER_IO_DATA,PER_IO_DATA;
/**
* �ṹ�����ƣ�PER_HANDLE_DATA
* �ṹ��洢����¼�����׽��ֵ����ݣ��������׽��ֵı������׽��ֵĶ�Ӧ�Ŀͻ��˵ĵ�ַ��
* �ṹ�����ã��������������Ͽͻ���ʱ����Ϣ�洢���ýṹ���У�֪���ͻ��˵ĵ�ַ�Ա��ڻطá�
**/
typedef struct{
	SOCKET socket;
	SOCKADDR_STORAGE ClientAddr;
}PER_HANDLE_DATA,*LPPER_HANDLE_DATA;

//ȫ�ֱ���
#define  PORT 4000
vector <PER_HANDLE_DATA*> clientGroup;

HANDLE hMutex = CreateMutex(NULL, false, NULL);
DWORD WINAPI ServerWorkThread(LPVOID lpParam);
DWORD WINAPI ServerSendThread(LPVOID lpParam);

int main(){
	//����socket��̬���ӿ�
	WORD VersionReq = MAKEWORD(2, 2);
	WSADATA Ws;
	DWORD err = WSAStartup(VersionReq, &Ws);

	if (err != 0){
		cout << "error:" << GetLastError() << endl;
		system("pause");
		return -1;
	}

	// ����IOCP���ں˶���  
	/**
	* ��Ҫ�õ��ĺ�����ԭ�ͣ�
	* HANDLE WINAPI CreateIoCompletionPort(
	*    __in   HANDLE FileHandle,     // �Ѿ��򿪵��ļ�������߿վ����һ���ǿͻ��˵ľ��
	*    __in   HANDLE ExistingCompletionPort, // �Ѿ����ڵ�IOCP���
	*    __in   ULONG_PTR CompletionKey,   // ��ɼ���������ָ��I/O��ɰ���ָ���ļ�
	*    __in   DWORD NumberOfConcurrentThreads // ��������ͬʱִ������߳�����һ���ƽ���CPU������*2
	* );
	**/
	HANDLE CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == CompletionPort){
		cout << "CreateIoCompletionPort error:" << GetLastError() << endl;
		system("pause");
		return -1;
	}
	// ����IOCP�߳�--�߳����洴���̳߳� 
	//ȷ�����ĵ�����
	SYSTEM_INFO systeminfo;
	GetSystemInfo(&systeminfo);

	//���ں������������߳�
	for (DWORD i = 0; i < (systeminfo.dwNumberOfProcessors * 2); i++){
		// �����������������̣߳�������ɶ˿ڴ��ݵ����߳� 
		HANDLE ThreadHandle = CreateThread(NULL, 0, ServerWorkThread, CompletionPort, 0, NULL);
		if (NULL == ThreadHandle){
			cout << "Create Thread Handle failed. Error:" << GetLastError() << endl;
			system("pause");
			return -1;
		}
		CloseHandle(ThreadHandle);
	}
	// ������ʽ�׽���  
	SOCKET srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	
	//��
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

	//����
	res = listen(srvSocket, 10);
	if (res == SOCKET_ERROR){
		cout << "listen error:" << GetLastError() << endl;
		system("pause");
		return -1;
	}

	//��ʼ����io����
	cout << "server�Ѿ������ȴ�client����..." << endl;

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

		// �����������׽��ֹ����ĵ����������Ϣ�ṹ
		Per_handle_data = (LPPER_HANDLE_DATA)GlobalAlloc(GPTR, sizeof(PER_HANDLE_DATA));// �ڶ���Ϊ���PerHandleData����ָ����С���ڴ�  
		Per_handle_data->socket = acceptSocket;
		memcpy(&Per_handle_data->ClientAddr, &saRemote, RemoteLen);
		clientGroup.push_back(Per_handle_data);// �������ͻ�������ָ��ŵ��ͻ�������

		//�������׽��ֺ���ɶ˿ڹ���  
		CreateIoCompletionPort((HANDLE)(Per_handle_data->socket), CompletionPort, (DWORD)Per_handle_data, 0);

		// ��ʼ�ڽ����׽����ϴ���I/Oʹ���ص�I/O����  
		// ���½����׽�����Ͷ��һ�������첽  
		// WSARecv��WSASend������ЩI/O������ɺ󣬹������̻߳�ΪI/O�����ṩ����      
		// ��I/O��������(I/O�ص�) 
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

		// ������׽������Ƿ��д�����  
		if (BytesTransferred == 0){
			closesocket(PerHandleData->socket);
			GlobalFree(PerHandleData);
			GlobalFree(PerIoData);
			continue;
		}

		// ��ʼ���ݴ����������Կͻ��˵����� 
		WaitForSingleObject(hMutex, INFINITE);
		cout << "a client says:" << PerIoData->databuf.buf << endl;
		ReleaseMutex(hMutex);

		// Ϊ��һ���ص����ý�����I/O�������� 
		ZeroMemory(&(PerIoData->overlapped), sizeof(OVERLAPPED));//����ڴ�
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
			//�ҳ�����ַ���ĳ���
		}
		talk[len] = '\n';
		talk[++len] = '\0';
		cout << "I say: " << talk << endl;
		WaitForSingleObject(hMutex, INFINITE);
		for (int i = 0; i < clientGroup.size(); ++i){
			send(clientGroup[i]->socket, talk, MAX_PATH, 0);	//������Ϣ
		}
		ReleaseMutex(hMutex);
	}

	return 0;
}