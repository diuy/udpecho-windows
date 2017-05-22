
#include "stdafx.h"
#include "MainApp.h"
#include "MainDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define DEFAULT_IP "172.20.2.220"
#define DEFAULT_PORT 40000
#define DEFAULT_SPEED 1000*50
#define DEFAULT_SIZE 200



CMainDlg::CMainDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_UDPECHOCLIENT_DIALOG, pParent)
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
static CString message;
static FILE* logFile = NULL;
static const char* iniFile = ".\\udpecho.ini";
static int tag = -1;

void WriteLog(const string & content, int type) {
	static const char* LEVEL[] = {"DEBUG", "ERROR" };

	CStringA  str(CTime::GetCurrentTime().Format("%Y-%m-%d %H:%M:%S"));
	str.AppendFormat(": %s: %s\r\n", LEVEL[type], content.c_str());

	TRACE(str);

	message.Append(CString(str));
	textConsole->SetWindowText(message);
	textConsole->LineScroll(textConsole->GetLineCount());

	if (logFile) {
		fwrite((LPCSTR)str, str.GetLength(), 1, logFile);
	}
}


static void ReadConfig(string &ip,int& port,int &speed,int& size,int& tag) {
	char tmp[255] = { 0 };
	GetPrivateProfileStringA("Config", "ip", DEFAULT_IP, tmp, 255, iniFile);
	ip = tmp;
	port = GetPrivateProfileIntA("Config", "port", DEFAULT_PORT, iniFile);
	speed = GetPrivateProfileIntA("Config", "speed", DEFAULT_SPEED, iniFile);
	size = GetPrivateProfileIntA("Config", "size", DEFAULT_SIZE, iniFile);
	srand((uint32_t)time(0));
	int defaultTag = rand();
	tag = GetPrivateProfileIntA("Config", "tag", defaultTag, iniFile);
}

static void WriteConfig(string ip, int port, int speed, int size, int tag) {
	WritePrivateProfileStringA("Config", "ip", ip.c_str(), iniFile);
	WritePrivateProfileStringA("Config", "port",IntToString(port).c_str(), iniFile);
	WritePrivateProfileStringA("Config", "speed", IntToString(speed).c_str(), iniFile);
	WritePrivateProfileStringA("Config", "size", IntToString(size).c_str(), iniFile);
	WritePrivateProfileStringA("Config", "tag", IntToString(tag).c_str(), iniFile);
}


BOOL CMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	Startup();

	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	::textConsole = &this->textConsole;

	string ip;
	int port ;
	int speed ;
	int size ;
	ReadConfig(ip, port, speed, size, tag);
	SetDlgItemText(IDC_TEXT_IP, CStringW(ip.c_str()));
	SetDlgItemText(IDC_TEXT_PORT, CStringW(IntToString(port).c_str()));
	SetDlgItemText(IDC_TEXT_SPEED, CStringW(IntToString(speed).c_str()));
	SetDlgItemText(IDC_TEXT_SIZE, CStringW(IntToString(size).c_str()));
	SetDlgItemText(IDC_TEXT_TAG, CStringW(IntToString(tag).c_str()));

	_mkdir(".\\log");
	CStringA logFileName(CTime::GetCurrentTime().Format("log\\%Y%m%d.log"));
	logFile = fopen(logFileName, "ab");

	return TRUE;  
}



void CMainDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
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
	CString strIp, strPort,strSpeed,strSize;
	string ip;
	int port = -1;
	int speed = -1;
	int size = -1;

	GetDlgItemText(IDC_TEXT_IP, strIp);
	GetDlgItemText(IDC_TEXT_PORT, strPort);
	GetDlgItemText(IDC_TEXT_SPEED, strSpeed);
	GetDlgItemText(IDC_TEXT_SIZE, strSize);


	if (strIp.IsEmpty()) {
		CERR("��ַ����Ϊ��");
		return;
	}
	ip = (const char*)CStringA(strIp);
	if (strPort.IsEmpty()) {
		CERR("�˿ڲ���Ϊ��");
		return;
	}
	port = StringToInt((const char*)CStringA(strPort), port);
	if (port == -1) {
		CERR("�˿ڸ�ʽ����");
		return;
	}

	if (strSpeed.IsEmpty()) {
		CERR("�ٶȲ���Ϊ��");
		return;
	}
	speed = StringToInt((const char*)CStringA(strSpeed), speed);
	if (speed == -1) {
		CERR("�ٶȸ�ʽ����");
		return;
	}

	if (strSize.IsEmpty()) {
		CERR("��С����Ϊ��");
		return;
	}
	size = StringToInt((const char*)CStringA(strSize), size);
	if (size == -1) {
		CERR("��С��ʽ����");
		return;
	}

	echo.reset(new UdpEcho(ip, port, speed, size, tag));
	if (echo->start()) {
		WriteConfig(ip, port, speed, size, tag);
	}
}


void CMainDlg::OnBnClickedBtnStop() {
	if (echo) {
		echo->stop();
		echo.reset();
	}
}


void CMainDlg::OnDestroy() {
	if (logFile) {
		fclose(logFile);
		logFile = NULL;
	}
	Cleanup();
	CDialogEx::OnDestroy();
}
