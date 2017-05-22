
// udpecho-clientDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "UdpEcho.h"

class CMainDlg : public CDialogEx
{
public:
	CMainDlg(CWnd* pParent = NULL);	// ��׼���캯��

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_UDPECHOCLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnStart();
	afx_msg void OnBnClickedBtnStop();
private:
	CEdit textConsole;
	unique_ptr<UdpEcho> echo;
public:
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBtnStopSend();
};
