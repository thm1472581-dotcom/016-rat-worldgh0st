// BuildDlgEx.cpp : implementation file
//

#include "stdafx.h"
#include "Client.h"
#include "BuildDlgEx.h"
#include "BuildHelper.h"
#include "MainFrm.h"

extern CMainFrame *g_pFrame;

IMPLEMENT_DYNCREATE(CBuildDlgEx, CFormView)

CBuildDlgEx::CBuildDlgEx()
	: CFormView(CBuildDlgEx::IDD)
{
}

CBuildDlgEx::~CBuildDlgEx()
{
}

void CBuildDlgEx::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_BUILD_EX, m_btnbuild);
	DDX_Control(pDX, IDC_RADIO_EX1, m_ck1);
	DDX_Control(pDX, IDC_RADIO_EX2, m_ck2);
}

BEGIN_MESSAGE_MAP(CBuildDlgEx, CFormView)
	ON_WM_PAINT()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BTN_BUILD_EX, &CBuildDlgEx::OnBnClickedBtnBuild)
END_MESSAGE_MAP()

#ifdef _DEBUG
void CBuildDlgEx::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void CBuildDlgEx::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif

BOOL CBuildDlgEx::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style &= ~WS_BORDER;
	return CFormView::PreCreateWindow(cs);
}

void CBuildDlgEx::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();

	m_brush.CreateSolidBrush(RGB(255,255,255));

	CFont m_font1,m_font2;
	m_font1.CreatePointFont(110,_T("Î¢ÈíÑÅºÚ"));
	GetDlgItem(IDC_STATIC_BUILD_EX)->SetFont(&m_font1);
	m_font1.Detach();

	m_font2.CreatePointFont(90,_T("Î¢ÈíÑÅºÚ"));
	GetDlgItem(IDC_STATIC_BUILD_EX2)->SetFont(&m_font2);
	m_font2.Detach();

	m_btnbuild.EnableMarkup();
	m_btnbuild.SetBitmap(0, IDB_PNG_BUILD);
	m_btnbuild.SetUseVisualStyle(TRUE);

	m_ck1.SetFlatStyle(TRUE);
	m_ck2.SetFlatStyle(TRUE);
	m_ck1.SetUseVisualStyle(TRUE);
	m_ck2.SetUseVisualStyle(TRUE);

	CButton* radio=(CButton*)GetDlgItem(IDC_RADIO_EX1);
	radio->SetCheck(1);

	// Prefill with public IP if available
	CString ip = WgGetInternetIP();
	if (!ip.IsEmpty())
		SetDlgItemText(IDC_EDIT_HOST, ip);
}

void CBuildDlgEx::OnPaint()
{
	CPaintDC dc(this);
	CRect rect;
	GetClientRect(rect);
	dc.FillSolidRect(rect,RGB(255, 255, 255));
}

HBRUSH CBuildDlgEx::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CFormView::OnCtlColor(pDC, pWnd, nCtlColor);

	switch(nCtlColor)
	{
	case CTLCOLOR_STATIC:
		{
			pDC->SetBkMode(TRANSPARENT);
			if(pWnd->GetDlgCtrlID() == IDC_STATIC_BUILD_EX)
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

void CBuildDlgEx::OnBnClickedBtnBuild()
{
	CString host;
	GetDlgItemText(IDC_EDIT_HOST, host);
	host.Trim();

	if (host.IsEmpty())
	{
		MessageBox(_T("ÇëÌîÐ´·þÎñÆ÷ IP »òÓòÃû¡£"), _T("ÌáÊ¾"), MB_ICONWARNING);
		return;
	}

	CButton* radio = (CButton*)GetDlgItem(IDC_RADIO_EX1);
	BOOL bShowUI = radio->GetCheck() ? TRUE : FALSE;
	if (WgBuildInstallPackage(this, host, bShowUI))
		g_pFrame->OnFileUser();
}
