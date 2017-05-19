#include "stdafx.h"
#include "UdpEcho.h"



UdpEcho::UdpEcho(string ip, int port, int speed, int size)
	:ip(ip), port(port), speed(speed), size(size)
	, so(INVALID_SOCKET), runFlag(false) {
}

UdpEcho::~UdpEcho() {
	stop();
}

bool UdpEcho::start() {
	if (runFlag)
		return true;
	if (ip.empty()) {
		CERR("IP��ַ����Ϊ��");
		return false;
	}

	if (port <= 0 || port >= 65535) {
		CERR("�˿����ô��� :" << port);
		return false;
	}

	if (speed<1 || speed > MAX_SPEED) {
		CERR("�������ô��� :" << speed);
		return false;
	}

	if (size <1 || size > BUFFER_SIZE) {
		CERR("��С���ô��� :" << size);
		return false;
	}

	int ret = 0;
	struct sockaddr_in addr;

	so = socket(AF_INET, SOCK_DGRAM, 0);
	if (so == INVALID_SOCKET) {
		CERR("socket create fail");
		return false;
	}

	addr.sin_family = AF_INET; //��ַ����
	addr.sin_port = htons(port); //ע��ת��Ϊ�����ֽ���
	addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str()); //ʹ��INADDR_ANY ָʾ�����ַ

	int flag1 = BUFFER_SIZE;
	setsockopt(so, SOL_SOCKET, SO_RCVBUF, (const char*)&flag1, sizeof(flag1));
	setsockopt(so, SOL_SOCKET, SO_SNDBUF, (const char*)&flag1, sizeof(flag1));

	ret = connect(so, (struct sockaddr *)&addr, sizeof(addr));
	if (ret != 0) {
		closesocket(so); //�ر��׽���
		so = INVALID_SOCKET;
		CERR("connect fail,ip:" << inet_ntoa(addr.sin_addr) << ",port:" << addr.sin_port << ",ret:" << ret);
		return false;
	}

	runFlag = true;

	recvThread.reset(new thread(mem_fn(&UdpEcho::recv), this));
	sendThread.reset(new thread(mem_fn(&UdpEcho::send), this));
}

void UdpEcho::stop() {
	if (!runFlag)
		return;
	runFlag = false;
	//shutdown(so, SD_BOTH);
	closesocket(so);
	recvThread->join();
	sendThread->join();

	recvThread = nullptr;
	sendThread = nullptr;
	so = INVALID_SOCKET;
}

void UdpEcho::send() {
}

void UdpEcho::recv() {
}

