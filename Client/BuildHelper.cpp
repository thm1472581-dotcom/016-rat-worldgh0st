#include "stdafx.h"
#include "BuildHelper.h"
#include <urlmon.h>

#pragma comment(lib, "urlmon.lib")

BOOL WgGetExeDir(LPTSTR exeDir, DWORD cchExeDir)
{
	if (GetModuleFileName(NULL, exeDir, cchExeDir) == 0)
		return FALSE;
	LPTSTR lastSlash = _tcsrchr(exeDir, _T('\\'));
	if (lastSlash == NULL)
		return FALSE;
	*lastSlash = _T('\0');
	return TRUE;
}

BOOL WgResolveServerTemplate(LPCTSTR exeDir, LPTSTR templatePath, DWORD cchTemplatePath)
{
	_stprintf_s(templatePath, cchTemplatePath, _T("%s\\Dat\\Install.dat"), exeDir);
	return GetFileAttributes(templatePath) != INVALID_FILE_ATTRIBUTES;
}

static CString TrimText(const CString& text)
{
	CString result = text;
	result.Trim(_T("\r\n\t "));
	return result;
}

BOOL WgIsValidIPv4(LPCTSTR ip)
{
	if (ip == NULL || *ip == 0)
		return FALSE;

	int a = 0, b = 0, c = 0, d = 0;
	TCHAR extra = 0;
	if (_stscanf(ip, _T("%d.%d.%d.%d%c"), &a, &b, &c, &d, &extra) != 4)
		return FALSE;

	return a >= 0 && a <= 255 && b >= 0 && b <= 255 && c >= 0 && c <= 255 && d >= 0 && d <= 255;
}

BOOL WgIsValidHost(LPCTSTR host)
{
	if (host == NULL || *host == 0)
		return FALSE;

	CString h = TrimText(host);
	if (h.IsEmpty() || h.GetLength() >= WG_HOST_SLOT_SIZE)
		return FALSE;

	if (WgIsValidIPv4(h))
		return TRUE;

	BOOL hasAlphaNum = FALSE;
	for (int i = 0; i < h.GetLength(); ++i)
	{
		TCHAR ch = h[i];
		if ((ch >= _T('a') && ch <= _T('z')) ||
			(ch >= _T('A') && ch <= _T('Z')) ||
			(ch >= _T('0') && ch <= _T('9')))
		{
			hasAlphaNum = TRUE;
			continue;
		}
		if (ch == _T('.') || ch == _T('-'))
			continue;
		return FALSE;
	}
	return hasAlphaNum;
}

BOOL WgPatchHostIntoFile(LPCTSTR filePath, LPCSTR hostA)
{
	if (filePath == NULL || hostA == NULL || hostA[0] == 0)
		return FALSE;

	size_t hostLen = strlen(hostA);
	if (hostLen == 0 || hostLen >= WG_HOST_SLOT_SIZE)
		return FALSE;

	HANDLE hFile = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize == INVALID_FILE_SIZE || fileSize < WG_HOST_SEARCH_LEN + 1)
	{
		CloseHandle(hFile);
		return FALSE;
	}

	BYTE* buf = (BYTE*)malloc(fileSize);
	if (buf == NULL)
	{
		CloseHandle(hFile);
		return FALSE;
	}

	DWORD read = 0;
	if (!ReadFile(hFile, buf, fileSize, &read, NULL) || read != fileSize)
	{
		free(buf);
		CloseHandle(hFile);
		return FALSE;
	}

	// Match marker+underscores so we hit g_szCfgHost, not a string-literal copy.
	const BYTE* marker = (const BYTE*)WG_HOST_SEARCH;
	DWORD pos = (DWORD)-1;
	for (DWORD i = 0; i + WG_HOST_SEARCH_LEN <= fileSize; ++i)
	{
		if (memcmp(buf + i, marker, WG_HOST_SEARCH_LEN) == 0)
		{
			pos = i;
			break;
		}
	}

	if (pos == (DWORD)-1 || pos + WG_HOST_SLOT_SIZE > fileSize)
	{
		free(buf);
		CloseHandle(hFile);
		return FALSE;
	}

	memset(buf + pos, 0, WG_HOST_SLOT_SIZE);
	memcpy(buf + pos, hostA, hostLen);

	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	DWORD written = 0;
	BOOL ok = WriteFile(hFile, buf, fileSize, &written, NULL) && written == fileSize;
	free(buf);
	CloseHandle(hFile);
	return ok;
}

static CString ReadTextFile(LPCTSTR path)
{
	CFile file;
	if (!file.Open(path, CFile::modeRead | CFile::shareDenyWrite))
		return _T("");

	ULONGLONG len = file.GetLength();
	if (len == 0 || len > 65536)
	{
		file.Close();
		return _T("");
	}

	CString data;
	LPTSTR buffer = data.GetBuffer((int)len + 1);
	file.Read(buffer, (UINT)len);
	buffer[len] = 0;
	data.ReleaseBuffer();
	file.Close();
	return TrimText(data);
}

static CString FetchIPFromURL(LPCTSTR url, LPCTSTR tempFile)
{
	DeleteFile(tempFile);
	if (URLDownloadToFile(NULL, url, tempFile, 0, NULL) != S_OK)
		return _T("");

	CString ip = ReadTextFile(tempFile);
	DeleteFile(tempFile);
	if (WgIsValidIPv4(ip))
		return ip;

	return _T("");
}

static CString GetLocalIPv4()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return _T("");

	CString result;
	char hostname[256] = {0};
	if (gethostname(hostname, sizeof(hostname) - 1) == 0)
	{
		hostent* host = gethostbyname(hostname);
		if (host != NULL && host->h_addr_list != NULL && host->h_addr_list[0] != NULL)
			result = CString(inet_ntoa(*(in_addr*)host->h_addr_list[0]));
	}

	WSACleanup();
	return result;
}

CString WgGetInternetIP()
{
	TCHAR tempPath[MAX_PATH] = {0};
	GetTempPath(_countof(tempPath), tempPath);

	TCHAR tempFile[MAX_PATH] = {0};
	_stprintf_s(tempFile, _T("%sve_ip.txt"), tempPath);

	static const LPCTSTR kIpUrls[] = {
		_T("https://api.ipify.org"),
		_T("http://api.ipify.org"),
		_T("https://checkip.amazonaws.com"),
	};

	for (int i = 0; i < _countof(kIpUrls); ++i)
	{
		CString ip = FetchIPFromURL(kIpUrls[i], tempFile);
		if (!ip.IsEmpty())
			return ip;
	}

	CString localIP = GetLocalIPv4();
	if (WgIsValidIPv4(localIP) && localIP != _T("127.0.0.1"))
		return localIP;

	return _T("");
}

BOOL WgBuildInstallPackage(CWnd* pParent, LPCTSTR host, BOOL bShowUI)
{
	if (!WgIsValidHost(host))
	{
		if (pParent)
			pParent->MessageBox(_T("服务器地址无效，请填写合法的 IP 或域名。"), _T("提示"), MB_ICONWARNING);
		return FALSE;
	}

	TCHAR exeDir[MAX_PATH] = {0};
	if (!WgGetExeDir(exeDir, _countof(exeDir)))
	{
		if (pParent)
			pParent->MessageBox(_T("无法获取程序目录。"), _T("提示"), MB_ICONERROR);
		return FALSE;
	}

	TCHAR templatePath[MAX_PATH] = {0};
	if (!WgResolveServerTemplate(exeDir, templatePath, _countof(templatePath)))
	{
		if (pParent)
		{
			pParent->MessageBox(
				_T("未找到 Release 版服务端模板 Dat\\Install.dat。\r\n")
				_T("请用 Visual Studio 以 Release 配置编译 Server 项目后再生成。\r\n")
				_T("（勿使用 Debug 版 Install.exe，被控端会缺少 MSVCR120D.dll）"),
				_T("提示"), MB_ICONERROR);
		}
		return FALSE;
	}

	TCHAR outputPath[MAX_PATH] = {0};
	if (bShowUI)
		_stprintf_s(outputPath, _T("%s\\Install_v1.exe"), exeDir);
	else
		_stprintf_s(outputPath, _T("%s\\Install_v2.exe"), exeDir);

	if (!CopyFile(templatePath, outputPath, FALSE))
	{
		if (pParent)
		{
			CString msg;
			msg.Format(_T("生成服务端失败，无法复制文件（错误码 %u）。"), GetLastError());
			pParent->MessageBox(msg, _T("提示"), MB_ICONERROR);
		}
		return FALSE;
	}

	CStringA hostA(host);
	if (!WgPatchHostIntoFile(outputPath, hostA))
	{
		DeleteFile(outputPath);
		if (pParent)
		{
			pParent->MessageBox(
				_T("写入服务器地址失败。\r\n")
				_T("请确认已用最新 Release 重新编译 Server（Install.dat 需含地址配置槽）。"),
				_T("提示"), MB_ICONERROR);
		}
		return FALSE;
	}

	if (pParent)
	{
		CString msg;
		msg.Format(_T("生成成功！\r\n输出文件：%s\r\n服务器地址：%s\r\n\r\n请将生成的安装包部署到被控端。"),
			outputPath, host);
		pParent->MessageBox(msg, _T("提示"), MB_ICONINFORMATION);
	}
	return TRUE;
}
