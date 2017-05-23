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
		CERR("IP��ַ����Ϊ��");
		return false;
	}

	if (port <= 0 || port >= 65535) {
		CERR("�˿����ô��� :" << port);
		return false;
	}

	if (speed<size || speed > MAX_SPEED) {
		CERR("�������ô��� :" << speed);
		return false;
	}

	if (size <MIN_SIZE || size > MAX_SIZE) {
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


	sendRunFlag = recvRunFlag = true;
	allSendCount = 0;
	allSendSize = 0;
	allRecvCount = 0;
	allRecvSize = 0;
	recvThread.reset(new thread(mem_fn(&UdpEcho::recvData), this));
	sendThread.reset(new thread(mem_fn(&UdpEcho::sendData), this));
	COUT("started, ip:" << inet_ntoa(addr.sin_addr) << ", port : " << addr.sin_port<<
	",tag:"<<tag);

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
	DWORD nowTime;
	int timeSpan = 0;
	while (sendRunFlag) {
		nowTime = GetTickCount();
		timeSpan = (int)(nowTime - startTime);
		if (timeSpan>0 && (allSendSize * 1000 / timeSpan) > speed) {
			Sleep(1);
			continue;
		}

		*((int*)(d + 8)) = index;
		randSize = rand() % size;
		sendTimes[index] = nowTime;
		sendSize = ::send(so, &data[0], size+randSize, 0);
		if (sendSize <= 0) {
			if (sendRunFlag) {
				CERR("send fail,error:" << WSAGetLastError());
			}
			break;
		}
		allSendCount++;
		allSendSize += sendSize;
		index++;
	}
}

void UdpEcho::recvData() {
	string data(BUFFER_SIZE, 0);
	int recvSize = 0;

	int t;
	int index;
	const char * d = data.c_str();
	DWORD nowTime;

	while (recvRunFlag) {
		recvSize = ::recv(so, &data[0], BUFFER_SIZE, 0);
		nowTime = GetTickCount();

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
		recvTimes[index] = nowTime;
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
		CERR("�޷�����");
	} else if (lastCountPercent > 20) {
		CERR("�����ǳ�����");
	} else if (lastCountPercent > 10) {
		COUT("�����Ƚ�����");
	} else if (lastCountPercent > 5) {
		COUT("����Ƚ�����");
	} else {
		COUT("����ǳ�����");
	}
	if (lastCountPercent < 100) {
		COUT("���Ͱ���:" << allSendCount << ",��������:" << allSendSize);
		COUT("���հ���:" << allRecvCount << ",��������:" << allRecvSize);
		COUT("��������:" << lastCount << ",��������:" << lastSize);
		COUT("���������ٷֱ�:" << setiosflags(ios::fixed) << setprecision(2)<<
			lastCountPercent << "%,���������ٷֱ�:" << setiosflags(ios::fixed) << setprecision(2) << lastSizePercent<<"%");
		
		
		map<pair<DWORD, DWORD>, int> times;
		times[pair<DWORD, DWORD>(0, 10)] = 0;
		times[pair<DWORD, DWORD>(10, 20)] = 0;
		times[pair<DWORD, DWORD>(20, 50)] = 0;
		times[pair<DWORD, DWORD>(50, 100)] = 0;
		times[pair<DWORD, DWORD>(100, 150)] = 0;
		times[pair<DWORD, DWORD>(150, 200)] = 0;
		times[pair<DWORD, DWORD>(200, 300)] = 0;
		times[pair<DWORD, DWORD>(300, 500)] = 0;
		times[pair<DWORD, DWORD>(500, 700)] = 0;
		times[pair<DWORD, DWORD>(700, 1000)] = 0;
		times[pair<DWORD, DWORD>(1000, 1500)] = 0;
		times[pair<DWORD, DWORD>(1500, 2000)] = 0;
		times[pair<DWORD, DWORD>(2000, 3000)] = 0;
		times[pair<DWORD, DWORD>(3000, 4000)] = 0;
		times[pair<DWORD, DWORD>(4000, 5000)] = 0;
		times[pair<DWORD, DWORD>(5000, 7000)] = 0;
		times[pair<DWORD, DWORD>(7000, 10000)] = 0;
		times[pair<DWORD, DWORD>(10000, 1000000000)] = 0;


	
		for (auto item = recvTimes.begin(); item != recvTimes.end(); item++) {
			DWORD t = item->second - sendTimes[item->first];
			for (auto item1 = times.begin(); item1 != times.end(); item1++) {
				if (t >= item1->first.first && t < item1->first.second) {
					item1->second++;
					break;
				}
			}
		}
		COUT("�������ݰ�����");
		for (auto item = times.begin(); item != times.end(); item++) {
			DWORD d1 = item->first.first;
			DWORD d2 = item->first.second;
			int count = item->second;
			if (count > 0) {
				double p = count*100.0 / allRecvCount;
				COUT("ʱ��(����):[" << setw(4) << setfill('0') << d1 << ","
					<< setw(4) << setfill('0') << d2 << "),������:" << setw(5) << setfill('0') << count
					<< ",�ٷֱ�:" << setw(5) << setfill('0') << setiosflags(ios::fixed) << setprecision(2) << p << "%");
			}
		}
	}
}

