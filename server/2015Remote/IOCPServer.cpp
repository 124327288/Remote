#include "StdAfx.h"
#include "IOCPServer.h"
#include "2015Remote.h"

#include <iostream>
#include "zlib.h"
#include "zconf.h"
using namespace std;
CRITICAL_SECTION IOCPServer::m_cs = {0};
#define HUERISTIC_VALUE 2
IOCPServer::IOCPServer(void)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData)!=0)
	{
		return;
	}

	m_hCompletionPort = NULL;
	m_sListenSocket   = INVALID_SOCKET;
	m_hListenEvent	      = WSA_INVALID_EVENT;
	m_hListenThread       = INVALID_HANDLE_VALUE;

	m_ulMaxConnections = ((CMy2015RemoteApp*)AfxGetApp())->m_iniFile.GetInt("Settings", "MaxConnection");


	if (m_ulMaxConnections==0)   
	{
		m_ulMaxConnections = 100;
	}

	InitializeCriticalSection(&m_cs);

	m_ulWorkThreadCount = 0;


	m_bTimeToKill = FALSE;


	m_ulThreadPoolMin  = 0; 
	m_ulThreadPoolMax  = 0;
	m_ulCPULowThreadsHold  = 0; 
	m_ulCPUHighThreadsHold = 0;
	m_ulCurrentThread = 0;
	m_ulBusyThread = 0;


	m_ulKeepLiveTime = 0;

	m_hKillEvent = NULL;

	memcpy(m_szPacketFlag,"Shine",FLAG_LENGTH);

	m_NotifyProc = NULL;
}


IOCPServer::~IOCPServer(void)
{

	m_bTimeToKill = TRUE;

	Sleep(10);
	SetEvent(m_hKillEvent);

	Sleep(10);


	if (m_hKillEvent!=NULL)
	{
		CloseHandle(m_hKillEvent);
	}

	if (m_sListenSocket!=INVALID_SOCKET)
	{
		closesocket(m_sListenSocket);
		m_sListenSocket = INVALID_SOCKET;
	}

	if (m_hCompletionPort!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = INVALID_HANDLE_VALUE;

	}

	if (m_hListenEvent!=WSA_INVALID_EVENT)
	{
		CloseHandle(m_hListenEvent);
		m_hListenEvent = WSA_INVALID_EVENT;
	}
	DeleteCriticalSection(&m_cs);
	m_ulWorkThreadCount = 0;

	m_ulThreadPoolMin  = 0; 
	m_ulThreadPoolMax  = 0;
	m_ulCPULowThreadsHold  = 0; 
	m_ulCPUHighThreadsHold = 0;
	m_ulCurrentThread = 0;
	m_ulBusyThread = 0;
	m_ulKeepLiveTime = 0;


	WSACleanup();
}
BOOL IOCPServer::StartServer(pfnNotifyProc NotifyProc,USHORT uPort)
{


    m_NotifyProc = NotifyProc;
	
	m_hKillEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

	if (m_hKillEvent==NULL)
	{
		return FALSE;
	}


	m_sListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);     //���������׽���

	
	if (m_sListenSocket == INVALID_SOCKET)
	{
		return FALSE;
	}

	m_hListenEvent = WSACreateEvent();           


	if (m_hListenEvent == WSA_INVALID_EVENT)
	{
		closesocket(m_sListenSocket);

		m_sListenSocket = INVALID_SOCKET;
		return FALSE;
	}


	int iRet = WSAEventSelect(m_sListenSocket,	//�������׽������¼����й���������FD_ACCEPT������
		m_hListenEvent,
		FD_ACCEPT);

	if (iRet == SOCKET_ERROR)
	{
		closesocket(m_sListenSocket);

		m_sListenSocket = INVALID_SOCKET;
		WSACloseEvent(m_hListenEvent);

		m_hListenEvent = WSA_INVALID_EVENT;


		return FALSE;
	}

	SOCKADDR_IN	ServerAddr;		
	ServerAddr.sin_port   = htons(uPort);
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = INADDR_ANY;                                           //��ʼ����������






	//�������׻��ֺ���������bind
	iRet = bind(m_sListenSocket, 
		(sockaddr*)&ServerAddr, 
		sizeof(ServerAddr));

	if (iRet == SOCKET_ERROR)
	{
		int a  = GetLastError();
		closesocket(m_sListenSocket);

		m_sListenSocket = INVALID_SOCKET;
		WSACloseEvent(m_hListenEvent);

		m_hListenEvent = WSA_INVALID_EVENT;


		return FALSE;
	}



	iRet = listen(m_sListenSocket, SOMAXCONN);

	if (iRet == SOCKET_ERROR)
	{
		closesocket(m_sListenSocket);

		m_sListenSocket = INVALID_SOCKET;
		WSACloseEvent(m_hListenEvent);

		m_hListenEvent = WSA_INVALID_EVENT;


		return FALSE;
	}


	m_hListenThread =
		(HANDLE)CreateThread(NULL,			
		0,					
		(LPTHREAD_START_ROUTINE)ListenThreadProc, 
		(void*)this,	      //��Thread�ص���������this �������ǵ��̻߳ص��������еĳ�Ա    
		0,					
		NULL);	
	if (m_hListenThread==INVALID_HANDLE_VALUE)
	{
		closesocket(m_sListenSocket);

		m_sListenSocket = INVALID_SOCKET;
		WSACloseEvent(m_hListenEvent);

		m_hListenEvent = WSA_INVALID_EVENT;
		return FALSE;
	}


	//���������߳�  1  2
	InitializeIOCP();



}


//1������ɶ˿�
//2���������߳�
BOOL IOCPServer::InitializeIOCP(VOID)
{
	

	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0 );
	if ( m_hCompletionPort == NULL ) 
	{
	
		return FALSE;
	}

	if (m_hCompletionPort==INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);  //���PC���м���



	
	
	m_ulThreadPoolMin  = 1; 
	m_ulThreadPoolMax  = 16;
	m_ulCPULowThreadsHold  = 10; 
	m_ulCPUHighThreadsHold = 75; 
	m_cpu.Init();

	ULONG ulWorkThreadCount = m_ulThreadPoolMax;

	HANDLE hWorkThread = NULL;
	for (int i=0; i<ulWorkThreadCount; i++ )    
	{
		hWorkThread = (HANDLE)CreateThread(NULL,	             //���������߳�Ŀ���Ǵ���Ͷ�ݵ���ɶ˿��е�����			
			0,						
			(LPTHREAD_START_ROUTINE)WorkThreadProc,     		
			(void*)this,			
			0,						
			NULL);			
		if (hWorkThread == NULL )    		
		{
			CloseHandle(m_hCompletionPort);
			return FALSE;
		}

		m_ulWorkThreadCount++;

		CloseHandle(hWorkThread);
	}


	return TRUE;

}




DWORD IOCPServer::WorkThreadProc(LPVOID lParam)
{
	IOCPServer* This = (IOCPServer*)(lParam);

	HANDLE   hCompletionPort = This->m_hCompletionPort;
	DWORD    dwTrans = 0;

	PCONTEXT_OBJECT  ContextObject = NULL;
	LPOVERLAPPED     Overlapped    = NULL;
	OVERLAPPEDPLUS*  OverlappedPlus = NULL;
	ULONG            ulBusyThread   = 0;
	BOOL             bError         = FALSE;


	InterlockedIncrement(&This->m_ulCurrentThread);      
	InterlockedIncrement(&This->m_ulBusyThread);         

	while (This->m_bTimeToKill==FALSE)
	{

		InterlockedDecrement(&This->m_ulBusyThread);  
		BOOL bOk = GetQueuedCompletionStatus(
			hCompletionPort,
			&dwTrans,
			(LPDWORD)&ContextObject,
			&Overlapped,INFINITE);  

		DWORD dwIOError = GetLastError();   

		OverlappedPlus = CONTAINING_RECORD(Overlapped, OVERLAPPEDPLUS, m_ol);
		

		ulBusyThread = InterlockedIncrement(&This->m_ulBusyThread); //1 1
		if (!bOk && dwIOError != WAIT_TIMEOUT )   //���Է����׻��Ʒ����˹ر�                    
		{
			if (ContextObject && This->m_bTimeToKill == FALSE &&dwTrans==0)
			{
				This->RemoveStaleContext(ContextObject);    
			}
			continue;
		}
		if (!bError) 
		{
			//����һ���µ��̵߳��̵߳��̳߳�
			if (ulBusyThread == This->m_ulCurrentThread)      
			{

				if (ulBusyThread < This->m_ulThreadPoolMax)    
				{

				//	if (This->m_cpu.GetUsage() > This->m_ulCPUHighThreadsHold)
					{

						if (ContextObject != NULL)
						{

							HANDLE hThread = (HANDLE)CreateThread(NULL,				
								0,				
								(LPTHREAD_START_ROUTINE)WorkThreadProc,  
								(void*)This,	    
								0,					
								NULL);	


							InterlockedIncrement(&This->m_ulWorkThreadCount);

							CloseHandle(hThread);
						}
					
					}
				}
			}

			if (!bOk && dwIOError == WAIT_TIMEOUT)      
			{
				if (ContextObject == NULL)
				{
				//	if (This->m_cpu.GetUsage() < This->m_ulCPULowThreadsHold)
					{

						if (This->m_ulCurrentThread > This->m_ulThreadPoolMin)
						{
							break;
						}
					}

					bError = TRUE;
				}
			}
		}
	
		if (!bError)  
		{
			if(bOk && OverlappedPlus!=NULL && ContextObject!=NULL) 
			{
				try
				{   
					
					This->HandleIO(OverlappedPlus->m_ioType, ContextObject, dwTrans);  

					ContextObject = NULL;
			
				}
				catch (...) {}
			}
		}

		if(OverlappedPlus)
		{
			delete OverlappedPlus; 

			OverlappedPlus = NULL;
		}

	}
	InterlockedDecrement(&This->m_ulWorkThreadCount);  

	InterlockedDecrement(&This->m_ulCurrentThread); 
	InterlockedDecrement(&This->m_ulBusyThread);  

	return 0;
	

	
}




BOOL IOCPServer::HandleIO(IOType PacketFlags,PCONTEXT_OBJECT ContextObject, DWORD dwTrans)   //�ڹ����߳��б�����
{ 
	BOOL bRet = FALSE; 

	if (IOInitialize == PacketFlags) 
	{
		bRet = OnClientInitializing(ContextObject, dwTrans); 
	}

	if (IORead==PacketFlags)   //WsaResv
	{
		bRet = OnClientReceiving(ContextObject,dwTrans);  //
	}

	if (IOWrite==PacketFlags)  //WsaSend
	{
		bRet = OnClientPostSending(ContextObject,dwTrans);
	}
	return bRet;
}


BOOL IOCPServer::OnClientInitializing(PCONTEXT_OBJECT  ContextObject, DWORD dwTrans)
{



/*	int i = 0;
	for (i=0;i<3;i++)
	{
		int nRet = send(ContextObject->sClientSocket,"HelloWorld",strlen("HelloWorld"),0);
	}*/
	
//	MessageBox(NULL,"HelloInit","HelloInit",0);

	return TRUE;
}




BOOL IOCPServer::OnClientReceiving(PCONTEXT_OBJECT  ContextObject, DWORD dwTrans)
{

	CLock cs(m_cs);
	try
	{

		if (dwTrans == 0)    //�Է��ر����׽���
		{
	
		//	MessageBox(NULL,"1","1",0);
			RemoveStaleContext(ContextObject);
			return FALSE;
		}

		ContextObject->InCompressedBuffer.WriteBuffer((PBYTE)ContextObject->szBuffer,dwTrans);    //�����յ������ݿ����������Լ����ڴ���wsabuff    8192


		while (ContextObject->InCompressedBuffer.GetBufferLength() > HDR_LENGTH)          //�鿴���ݰ��������
		{
			char szPacketFlag[FLAG_LENGTH]= {0};
	
		
			CopyMemory(szPacketFlag, ContextObject->InCompressedBuffer.GetBuffer(),FLAG_LENGTH);

	

			if (memcmp(m_szPacketFlag, szPacketFlag, FLAG_LENGTH) != 0)
			{
				throw "bad Buffer";
			}

			ULONG ulPackTotalLength = 0;
			CopyMemory(&ulPackTotalLength, ContextObject->InCompressedBuffer.GetBuffer(FLAG_LENGTH), sizeof(ULONG));//Shine[50][kdjfkdjfkj]

			//ȡ�����ݰ����ܳ�

			
			//50
			if (ulPackTotalLength && (ContextObject->InCompressedBuffer.GetBufferLength()) >= ulPackTotalLength)  
			{
				ULONG ulOriginalLength = 0;  
				ContextObject->InCompressedBuffer.ReadBuffer((PBYTE)szPacketFlag, FLAG_LENGTH);    

				ContextObject->InCompressedBuffer.ReadBuffer((PBYTE) &ulPackTotalLength, sizeof(ULONG));            

				ContextObject->InCompressedBuffer.ReadBuffer((PBYTE) &ulOriginalLength, sizeof(ULONG)); 

				ULONG ulCompressedLength = ulPackTotalLength - HDR_LENGTH;  //461 - 13      448
				PBYTE CompressedBuffer = new BYTE[ulCompressedLength];                 //û�н�ѹ
			
				PBYTE DeCompressedBuffer = new BYTE[ulOriginalLength];  //��ѹ�����ڴ�  436

				if (CompressedBuffer == NULL || DeCompressedBuffer == NULL)
				{
					throw "Bad Allocate";

				}
				ContextObject->InCompressedBuffer.ReadBuffer(CompressedBuffer, ulCompressedLength); //�����ݰ���ǰ��Դ����û�н�ѹ��ȡ��pData   448

				int	iRet = uncompress(DeCompressedBuffer, &ulOriginalLength, CompressedBuffer, ulCompressedLength);     //��ѹ����

				if (iRet == Z_OK)
				{
					ContextObject->InDeCompressedBuffer.ClearBuffer();
					ContextObject->InCompressedBuffer.ClearBuffer();
					ContextObject->InDeCompressedBuffer.WriteBuffer(DeCompressedBuffer, ulOriginalLength);
					
					
					m_NotifyProc(ContextObject);  //֪ͨ����   
				}
				else
				{
					throw "Bad Buffer";
				}

				delete [] CompressedBuffer;
				delete [] DeCompressedBuffer;
			
			}
			else
			{
				break;
			}
		}
		PostRecv(ContextObject);   //Ͷ���µĽ������ݵ�����
	}catch(...)
	{
		ContextObject->InCompressedBuffer.ClearBuffer();
		ContextObject->InDeCompressedBuffer.ClearBuffer();

		PostRecv(ContextObject);
	}
	return TRUE;

}



VOID IOCPServer::OnClientPreSending(CONTEXT_OBJECT* ContextObject, PBYTE szBuffer , ULONG ulOriginalLength)  
{
	if (ContextObject == NULL)
	{
		return;
	}

	try
	{
		if (ulOriginalLength > 0)
		{
			unsigned long	ulCompressedLength = (double)ulOriginalLength * 1.001  + 12;
			LPBYTE			CompressedBuffer = new BYTE[ulCompressedLength];
			int	iRet = compress(CompressedBuffer, &ulCompressedLength, (LPBYTE)szBuffer, ulOriginalLength);

			if (iRet != Z_OK)
			{
				delete [] CompressedBuffer;
				return;
			}


			ULONG ulPackTotalLength = ulCompressedLength + HDR_LENGTH;    

			ContextObject->OutCompressedBuffer.WriteBuffer((LPBYTE)m_szPacketFlag,FLAG_LENGTH);  

			ContextObject->OutCompressedBuffer.WriteBuffer((PBYTE)&ulPackTotalLength, sizeof(ULONG));      

			ContextObject->OutCompressedBuffer.WriteBuffer((PBYTE) &ulOriginalLength, sizeof(ULONG));      

			ContextObject->OutCompressedBuffer.WriteBuffer(CompressedBuffer, ulCompressedLength);        
				
			delete [] CompressedBuffer;


		}
	
			
		OVERLAPPEDPLUS* OverlappedPlus = new OVERLAPPEDPLUS(IOWrite);   

	

		PostQueuedCompletionStatus(m_hCompletionPort, 0, (DWORD)ContextObject, &OverlappedPlus->m_ol);     
	}catch(...){}
}




BOOL IOCPServer::OnClientPostSending(CONTEXT_OBJECT* ContextObject,ULONG ulCompletedLength)   
{

	try
	{

			DWORD ulFlags = MSG_PARTIAL;

			ContextObject->OutCompressedBuffer.RemoveComletedBuffer(ulCompletedLength);             //����ɵ����ݴ����ݽṹ��ȥ��
			if (ContextObject->OutCompressedBuffer.GetBufferLength() == 0)
			{
				ContextObject->OutCompressedBuffer.ClearBuffer();
				return true;		                             //�ߵ�����˵�����ǵ�����������ȫ����
			}
			else
			{
					
				OVERLAPPEDPLUS * OverlappedPlus = new OVERLAPPEDPLUS(IOWrite);           //����û�����  ���Ǽ���Ͷ�� ��������
				
				ContextObject->wsaOutBuffer.buf = (char*)ContextObject->OutCompressedBuffer.GetBuffer();
				ContextObject->wsaOutBuffer.len = ContextObject->OutCompressedBuffer.GetBufferLength();                 //���ʣ������ݺͳ���    

				int iOk = WSASend(ContextObject->sClientSocket, 
					&ContextObject->wsaOutBuffer,
					1,
					&ContextObject->wsaOutBuffer.len, 
					ulFlags,
					&OverlappedPlus->m_ol, 
					NULL);
				if (iOk == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING )
				{
					int a = GetLastError();
					RemoveStaleContext(ContextObject);
				}

			}
		}catch(...){}
			
	return FALSE;			
}



DWORD IOCPServer::ListenThreadProc(LPVOID lParam)   //�����߳�
{
		
	IOCPServer* This = (IOCPServer*)(lParam);
	WSANETWORKEVENTS NetWorkEvents;

	while(1)
	{
		
		if (WaitForSingleObject(This->m_hKillEvent, 100) == WAIT_OBJECT_0)
		{
			break;     
		}

		DWORD dwRet;
		dwRet = WSAWaitForMultipleEvents(1,
			&This->m_hListenEvent,
			FALSE,
			100,
			FALSE);  

		if (dwRet == WSA_WAIT_TIMEOUT)
		{
			continue;
		}

		int iRet = WSAEnumNetworkEvents(This->m_sListenSocket,    
			//����¼����� ���Ǿͽ����¼�ת����һ�������¼� ���� �ж�
			This->m_hListenEvent,
			&NetWorkEvents);

		if (iRet == SOCKET_ERROR)
		{
			break;
		}

		if (NetWorkEvents.lNetworkEvents & FD_ACCEPT)
		{
			if (NetWorkEvents.iErrorCode[FD_ACCEPT_BIT] == 0)
			{
				//�����һ�������������Ǿͽ���OnAccept()�������д���
				This->OnAccept(); 
			}
			else
			{
				break;
			}

		}

	} 

	return 0;
}



void IOCPServer::OnAccept()
{

	SOCKADDR_IN	ClientAddr = {0};     
	SOCKET		sClientSocket = INVALID_SOCKET;

	int			iRet = 0;
	int			iLen = 0;;

	iLen = sizeof(SOCKADDR_IN);
	sClientSocket = accept(m_sListenSocket,
		(sockaddr*)&ClientAddr,
		&iLen);                     //ͨ�����ǵļ����׽���������һ����֮�ź�ͨ�ŵ��׽���
	if (sClientSocket == SOCKET_ERROR)
	{
		/*nRet = WSAGetLastError();
		if (nRet != WSAEWOULDBLOCK)    //����
		{
			return;
		}*/

		return;
	}

	//����������Ϊÿһ��������ź�ά����һ����֮���������ݽṹ������Ϊ�û������±�����
	PCONTEXT_OBJECT ContextObject = AllocateContext();   //    Context

	if (ContextObject == NULL)
	{

		closesocket(sClientSocket);
		sClientSocket = INVALID_SOCKET;
		return;
	}



	ContextObject->sClientSocket = sClientSocket;


	ContextObject->wsaInBuf.buf = (char*)ContextObject->szBuffer;
	ContextObject->wsaInBuf.len = sizeof(ContextObject->szBuffer);


	HANDLE Handle = CreateIoCompletionPort((HANDLE)sClientSocket, m_hCompletionPort, (DWORD)ContextObject, 0);


	if (Handle!=m_hCompletionPort)
	{

		delete ContextObject;
		ContextObject = NULL;

		if (sClientSocket!=INVALID_SOCKET)
		{
			closesocket(sClientSocket);
			sClientSocket = INVALID_SOCKET;
		}

		return;
	}



	//�����׽��ֵ�ѡ� Set KeepAlive ����������� SO_KEEPALIVE 
	//�������Ӽ��Է������Ƿ�������2Сʱ���ڴ��׽ӿڵ���һ����û
	//�����ݽ�����TCP���Զ����Է� ��һ�����ִ��
	m_ulKeepLiveTime = 3;
	const BOOL bKeepAlive = TRUE;
	if (setsockopt(ContextObject->sClientSocket,SOL_SOCKET,SO_KEEPALIVE,(char*)&bKeepAlive,sizeof(bKeepAlive))!= 0)
	{
	}

	//���ó�ʱ��ϸ��Ϣ
	tcp_keepalive	KeepAlive;
	KeepAlive.onoff = 1; // ���ñ���
	KeepAlive.keepalivetime = m_ulKeepLiveTime;       //����3����û�����ݣ��ͷ���̽���
	KeepAlive.keepaliveinterval = 1000 * 10;         //���Լ��Ϊ10�� Resend if No-Reply
	WSAIoctl
		(
		ContextObject->sClientSocket, 
		SIO_KEEPALIVE_VALS,
		&KeepAlive,
		sizeof(KeepAlive),
		NULL,
		0,
		(unsigned long *)&bKeepAlive,
		0,
		NULL
		);
		
	//����������ʱ����������ͻ������߻�ϵ�ȷ������Ͽ����������������û������SO_KEEPALIVEѡ�
	//���һֱ���ر�SOCKET����Ϊ�ϵĵ�������Ĭ������Сʱʱ��̫�����������Ǿ��������ֵ


	CLock cs(m_cs);
	m_ContextConnectionList.AddTail(ContextObject);     //���뵽���ǵ��ڴ��б���

	OVERLAPPEDPLUS	*OverlappedPlus = new OVERLAPPEDPLUS(IOInitialize);   //ע��������ص�IO������ �û���������
 
	BOOL bOk = PostQueuedCompletionStatus(m_hCompletionPort, 0, (DWORD)ContextObject, &OverlappedPlus->m_ol);     //  �����߳�
	//��Ϊ���ǽ��ܵ���һ���û����ߵ�������ô���Ǿͽ��������͸����ǵ���ɶ˿� �����ǵĹ����̴߳�����

	if ( (!bOk && GetLastError() != ERROR_IO_PENDING))  //���Ͷ��ʧ��
	{            
		RemoveStaleContext(ContextObject);
		return;
	}

	PostRecv(ContextObject);                                         
}



VOID IOCPServer::PostRecv(CONTEXT_OBJECT* ContextObject)
{
	OVERLAPPEDPLUS * OverlappedPlus = new OVERLAPPEDPLUS(IORead);    //�����ǵĸ����ߵ��û���Ͷ��һ���������ݵ�����  ����û��ĵ�һ�����ݰ�����Ҳ�;��Ǳ��ض˵ĵ�½���󵽴����ǵĹ����߳̾�
				         //����Ӧ	������ProcessIOMessage����
	DWORD			dwReturn;
	ULONG			ulFlags = MSG_PARTIAL;
	int iOk = WSARecv(ContextObject->sClientSocket, 
		&ContextObject->wsaInBuf,
		1,
		&dwReturn, 
		&ulFlags,
		&OverlappedPlus->m_ol, 
		NULL);

	if ( iOk == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) 
	{
		int a = GetLastError();
		RemoveStaleContext(ContextObject);
	}
}



PCONTEXT_OBJECT IOCPServer::AllocateContext()
{
	PCONTEXT_OBJECT ContextObject = NULL;

	CLock cs(m_cs);       

	if (m_ContextFreePoolList.IsEmpty()==FALSE)                        
	{
		ContextObject = m_ContextFreePoolList.RemoveHead();    
	}
	else
	{
		ContextObject = new CONTEXT_OBJECT;                   
	}

	if (ContextObject != NULL)
	{
		ContextObject->InitMember();
	}
	return ContextObject;
}



VOID IOCPServer::RemoveStaleContext(CONTEXT_OBJECT* ContextObject)
{
	CLock cs(m_cs);

	if (m_ContextConnectionList.Find(ContextObject))    //���ڴ��в��Ҹ��û������±��������ݽṹ
	{

		CancelIo((HANDLE)ContextObject->sClientSocket);  //ȡ���ڵ�ǰ�׽��ֵ��첽IO   -->PostRecv    

		closesocket(ContextObject->sClientSocket);      //�ر��׽���
		ContextObject->sClientSocket = INVALID_SOCKET;

		while (!HasOverlappedIoCompleted((LPOVERLAPPED)ContextObject))   //�жϻ���û���첽IO�����ڵ�ǰ�׽�����
		{
			Sleep(0);
		}

		MoveContextToFreePoolList(ContextObject);  //�����ڴ�ṹ�������ڴ��
	}
}



VOID IOCPServer::MoveContextToFreePoolList(CONTEXT_OBJECT* ContextObject)
{
	CLock cs(m_cs);

	POSITION Pos = m_ContextConnectionList.Find(ContextObject);
	if (Pos) 
	{
		
		ContextObject->InCompressedBuffer.ClearBuffer();
		ContextObject->InDeCompressedBuffer.ClearBuffer();
		ContextObject->OutCompressedBuffer.ClearBuffer();

		memset(ContextObject->szBuffer,0,8192);
		m_ContextFreePoolList.AddTail(ContextObject);                            //�������ڴ��
		m_ContextConnectionList.RemoveAt(Pos);                                //���ڴ�ṹ���Ƴ�

	}
}