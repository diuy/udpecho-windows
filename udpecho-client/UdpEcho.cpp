#include "stdafx.h"
#include "UdpEcho.h"



UdpEcho::UdpEcho(string ip, int port, int speed, int size, int tag)
	:ip(ip), port(port), speed(speed), size(size), tag(tag)
	, so(INVALID_SOCKET), sendRunFlag(false),recvRunFlag(false)
	, allSendCount(0), allSendSize(0), allRecvCount(0), allRecvSize(0) {
}

UdpEcho::~UdpEcho() {
	stop();
}

bool UdpEcho::start() {
	if (sendRunFlag || recvRunFlag)
		return true;
	if (ip.empty()) {
		CERR("IP地址不能为空");
		return false;
	}

	if (port <= 0 || port >= 65535) {
		CERR("端口设置错误 :" << port);
		return false;
	}

	if (speed<size || speed > MAX_SPEED) {
		CERR("带宽设置错误 :" << speed);
		return false;
	}

	if (size <MIN_SIZE || size > MAX_SIZE) {
		CERR("大小设置错误 :" << size);
		return false;
	}

	int ret = 0;
	struct sockaddr_in addr;

	so = socket(AF_INET, SOCK_DGRAM, 0);
	if (so == INVALID_SOCKET) {
		CERR("socket create fail");
		return false;
	}

	addr.sin_family = AF_INET; //地址家族
	addr.sin_port = htons(port); //注意转化为网络字节序
	addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str()); //使用INADDR_ANY 指示任意地址

	int flag1 = BUFFER_SIZE;
	setsockopt(so, SOL_SOCKET, SO_RCVBUF, (const char*)&flag1, sizeof(flag1));
	setsockopt(so, SOL_SOCKET, SO_SNDBUF, (const char*)&flag1, sizeof(flag1));

	ret = connect(so, (struct sockaddr *)&addr, sizeof(addr));
	if (ret != 0) {
		closesocket(so); //关闭套接字
		so = INVALID_SOCKET;
		CERR("connect fail,ip:" << inet_ntoa(addr.sin_addr) << ",port:" << addr.sin_port << ",ret:" << ret);
		return false;
	}

	sendRunFlag = recvRunFlag = true;
	allSendCount = 0;
	allSendSize = 0;
	allRecvCount = 0;
	allRecvSize = 0;
	recvThread.reset(new thread(mem_fn(&UdpEcho::recvData), this));
	sendThread.reset(new thread(mem_fn(&UdpEcho::sendData), this));
	COUT("started!");
	return true;
}

void UdpEcho::stopSend() {
	if (!sendRunFlag )
		return;
	sendRunFlag = false;
	COUT("send stoped!");
}

void UdpEcho::stop() {

	if (!sendRunFlag && !recvRunFlag)
		return;
	sendRunFlag = recvRunFlag = false;

	//shutdown(so, SD_BOTH);
	closesocket(so);
	recvThread->join();
	sendThread->join();

	recvThread = nullptr;
	sendThread = nullptr;
	so = INVALID_SOCKET;
	printResult();
	COUT("stoped!\r\n");
}

void UdpEcho::sendData() {
	string data(size*2, 0);
	char* d = &data[0];
	d[0] = (char)0xf1;
	d[1] = (char) 0xf2;

	*((int*)(d + 4)) = tag;
	int index = 0;
	int randSize = 0;
	int sendSize = 0;
	DWORD startTime = GetTickCount();
	int timeSpan = 0;
	while (sendRunFlag) {
		timeSpan = (int)(GetTickCount()- startTime);
		if (timeSpan>0 && (allSendSize * 1000 / timeSpan) > speed) {
			Sleep(1);
			continue;
		}

		index++;
		*((int*)(d + 8)) = index;
		randSize = rand() % size;
		sendSize = ::send(so, &data[0], size+randSize, 0);
		if (sendSize <= 0) {
			if (sendRunFlag) {
				CERR("send fail,error:" << WSAGetLastError());
			}
			break;
		}
		allSendCount++;
		allSendSize += sendSize;
	}
}

void UdpEcho::recvData() {
	string data(BUFFER_SIZE, 0);
	int recvSize = 0;

	int t;
	int index;
	const char * d = data.c_str();

	while (recvRunFlag) {
		recvSize = ::recv(so, &data[0], BUFFER_SIZE, 0);
		if (recvSize <= 0) {
			if (recvRunFlag) {
				CERR("recv fail,error:"<<WSAGetLastError());
			}
			break;
		}
		if (d[0] != (char)0xf1 || d[1] != (char)0xf2) {
			CERR("recv sync error");
			continue;
		}
		t = *((int*)(d + 4));
		index = *((int*)(d + 8));
		if (t != tag) {
			CERR("recv tag error," << tag);
			continue;
		}
		allRecvCount++;
		allRecvSize += recvSize;
	}
}

void UdpEcho::printResult() {
	int lastCount = allSendCount-allRecvCount  ;
	int lastSize = allSendSize-allRecvSize  ;
	double lastCountPercent = lastCount*100/ allSendCount;
	double lastSizePercent = lastSize*100/allSendSize;

	if (lastCountPercent >= 100) {
		CERR("无法连接");
	} else if (lastCountPercent > 20) {
		CERR("丢包非常严重");
	} else if (lastCountPercent > 10) {
		COUT("丢包比较严重");
	} else if (lastCountPercent > 5) {
		COUT("网络比较流畅");
	} else {
		COUT("网络非常流畅");
	}
	if (lastCountPercent < 100) {
		COUT("发送包数:" << allSendCount << ",发送流量:" << allSendSize);
		COUT("接收包数:" << allRecvCount << ",接收流量:" << allRecvSize);
		COUT("丢包数量:" << lastCount << ",丢包流量:" << lastSize);
		COUT("丢包数量百分比:" << lastCountPercent << ",丢包流量百分比:" << lastSizePercent);
	}
}

