#pragma once

#include <shlobj.h>

#define WG_INSTALL_DIR   _T("C:\\ProgramData\\Microsoft\\Framework")
#define WG_INI_PATH      _T("C:\\ProgramData\\Microsoft\\Framework\\V-Eye.ini")
#define WG_EXE_PATH      _T("C:\\ProgramData\\Microsoft\\Framework\\V-Eye.exe")

inline BOOL WgEnsureInstallDir()
{
	int r = SHCreateDirectoryEx(NULL, WG_INSTALL_DIR, NULL);
	DWORD err = GetLastError();
	return r == ERROR_SUCCESS || err == ERROR_ALREADY_EXISTS;
}

inline BOOL WgPathsEqual(LPCTSTR a, LPCTSTR b)
{
	TCHAR fa[MAX_PATH] = {0};
	TCHAR fb[MAX_PATH] = {0};
	if (!GetFullPathName(a, MAX_PATH, fa, NULL))
		return FALSE;
	if (!GetFullPathName(b, MAX_PATH, fb, NULL))
		return FALSE;
	return lstrcmpi(fa, fb) == 0;
}
