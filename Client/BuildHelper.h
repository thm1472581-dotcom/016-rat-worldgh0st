#pragma once
#include "HostConfig.h"

// Shared helpers for generating Install_v1 / Install_v2 packages.
BOOL WgGetExeDir(LPTSTR exeDir, DWORD cchExeDir);
BOOL WgResolveServerTemplate(LPCTSTR exeDir, LPTSTR templatePath, DWORD cchTemplatePath);
BOOL WgPatchHostIntoFile(LPCTSTR filePath, LPCSTR hostA);
BOOL WgIsValidIPv4(LPCTSTR ip);
BOOL WgIsValidHost(LPCTSTR host); // IPv4 or DNS name
CString WgGetInternetIP();
BOOL WgBuildInstallPackage(CWnd* pParent, LPCTSTR host, BOOL bShowUI);
