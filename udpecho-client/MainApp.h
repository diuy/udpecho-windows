
// udpecho-client.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CUdpEchoClientApp: 
// �йش����ʵ�֣������ udpecho-client.cpp
//

class CMainApp : public CWinApp
{
public:
	CMainApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CMainApp theApp;