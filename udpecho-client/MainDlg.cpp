
#include "stdafx.h"
#include "MainApp.h"
#include "MainDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



CMainDlg::CMainDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_UDPECHOCLIENT_DIALOG, pParent)
	, tag(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TEXT_CONSOLE, textConsole);
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_START, &CMainDlg::OnBnClickedBtnStart)
	ON_BN_CLICKED(IDC_BTN_STOP, &CMainDlg::OnBnClickedBtnStop)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


static void Startup() {
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2); 
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		CERR ("WSAStartup error!" );
	}
}

static void Cleanup() {
	WSACleanup();
}
static CEdit *textConsole = NULL;
static CStringA msg;

void WriteLog(const string & str, int type) {

}

BOOL CMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	Startup();

	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	srand((uint32_t)time(0));
	while (tag == 0) {
		tag = rand();
	}
	::textConsole = &this->textConsole;
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}



void CMainDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CMainDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMainDlg::OnBnClickedBtnStart() {
}


void CMainDlg::OnBnClickedBtnStop() {
}


void CMainDlg::OnDestroy() {
	Cleanup();
	CDialogEx::OnDestroy();
}
