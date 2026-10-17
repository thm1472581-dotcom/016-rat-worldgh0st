// BuildDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Client.h"
#include "BuildDlg.h"
#include "BuildHelper.h"
#include "MainFrm.h"

extern CMainFrame *g_pFrame;

IMPLEMENT_DYNCREATE(CBuildDlg, CFormView)

CBuildDlg::CBuildDlg()
	: CFormView(CBuildDlg::IDD)
{
}

CBuildDlg::~CBuildDlg()
{
}

void CBuildDlg::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_BUILD, m_btnbuild);
	DDX_Control(pDX, IDC_RADIO1, m_ck1);
	DDX_Control(pDX, IDC_RADIO2, m_ck2);
}

BEGIN_MESSAGE_MAP(CBuildDlg, CFormView)
	ON_WM_PAINT()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BTN_BUILD, &CBuildDlg::OnBnClickedBtnBuild)
END_MESSAGE_MAP()

#ifdef _DEBUG
void CBuildDlg::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void CBuildDlg::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG

BOOL CBuildDlg::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style &= ~WS_BORDER;
	return CFormView::PreCreateWindow(cs);
}

void CBuildDlg::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();

	m_brush.CreateSolidBrush(RGB(255,255,255));

	CFont m_font1,m_font2;
	m_font1.CreatePointFont(110,_T("微软雅黑"));
	GetDlgItem(IDC_STATIC_BUILD)->SetFont(&m_font1);
	m_font1.Detach();

	m_font2.CreatePointFont(90,_T("微软雅黑"));
	GetDlgItem(IDC_STATIC_BUILD2)->SetFont(&m_font2);
	m_font2.Detach();

	m_btnbuild.EnableMarkup();
	m_btnbuild.SetBitmap(0, IDB_PNG_BUILD);
	m_btnbuild.SetUseVisualStyle(TRUE);

	m_ck1.SetFlatStyle(TRUE);
	m_ck2.SetFlatStyle(TRUE);
	m_ck1.SetUseVisualStyle(TRUE);
	m_ck2.SetUseVisualStyle(TRUE);

	CButton* radio=(CButton*)GetDlgItem(IDC_RADIO1);
	radio->SetCheck(1);
}

void CBuildDlg::OnPaint()
{
	CPaintDC dc(this);

	CRect rect;
	GetClientRect(rect);
	dc.FillSolidRect(rect,RGB(255, 255, 255));
}

HBRUSH CBuildDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CFormView::OnCtlColor(pDC, pWnd, nCtlColor);

	switch(nCtlColor)
	{
	case CTLCOLOR_STATIC:
		{
			pDC->SetBkMode(TRANSPARENT);
			if(pWnd->GetDlgCtrlID() == IDC_STATIC_BUILD)
				pDC->SetTextColor(RGB(65,105,225));
			else
				pDC->SetTextColor(RGB(110,110,110));
			hbr=(HBRUSH)m_brush;
			break;
		}
	case CTLCOLOR_DLG:
		return (HBRUSH)m_brush.GetSafeHandle();
	case CTLCOLOR_BTN:
		hbr=(HBRUSH)m_brush;
		break;
	default:
		break;
	}
	return hbr;
}

void CBuildDlg::OnBnClickedBtnBuild()
{
	CString strIP = WgGetInternetIP();
	if (strIP.IsEmpty())
	{
		MessageBox(_T("无法获取本机 IP 地址，请检查网络连接后重试。"),
			_T("提示"), MB_ICONWARNING);
		return;
	}

	CButton* radio = (CButton*)GetDlgItem(IDC_RADIO1);
	BOOL bShowUI = radio->GetCheck() ? TRUE : FALSE;
	if (WgBuildInstallPackage(this, strIP, bShowUI))
		g_pFrame->OnFileUser();
}
