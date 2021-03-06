#include "stdafx.h"
#include "UdpEcho.h"


UdpEcho::UdpEcho(string ip, int port, int speed, int size, int tag, int16_t id)
	:ip(ip), port(port), speed(speed), size(size), tag(tag),id(id)
	, so(INVALID_SOCKET), sendRunFlag(false),recvRunFlag(false)
	, allSendCount(0), allSendSize(0), allRecvCount(0), allRecvSize(0), _startTime(0), _stopTime(0){
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
		CERR("端口设置错误 :" << port<<",范围:(0,65535)");
		return false;
	}
	if (size <MIN_SIZE || size > MAX_SIZE) {
		CERR("大小设置错误 :" << size<<",范围:["<< MIN_SIZE<<","<<MAX_SIZE<<"]");
		return false;
	}

	if (speed<size) {
		CERR("(带宽)不能小于(大小)值 :" << speed);
		return false;
	}

	if ( speed > MAX_SPEED) {
		CERR("带宽设置错误 :" << speed<<",最大值:"<<MAX_SPEED);
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
		CERR("connect fail,ip:" << ip << ",port:" << port << ",ret:" << ret);
		return false;
	}


	sendRunFlag = recvRunFlag = true;
	allSendCount = 0;
	allSendSize = 0;
	allRecvCount = 0;
	allRecvSize = 0;
	_startTime = GetNowTime();
	recvThread.reset(new thread(mem_fn(&UdpEcho::recvData), this));
	sendThread.reset(new thread(mem_fn(&UdpEcho::sendData), this));
	COUT("started,windows!");
	COUT("ip:" << ip << ",port:" << port <<",tag:"<<tag<<",id:"<<id);
	COUT("speed:" << speed << ",size:" << size << ",countPerSecond:" << setiosflags(ios::fixed) << setprecision(2)<<speed*1.0 / size);
	return true;
}

void UdpEcho::stopSend() {
	if (!sendRunFlag )
		return;
	_stopTime = GetNowTime();
	sendRunFlag = false;
	COUT("send stopped!");
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
	COUT("stopped!");
}

void UdpEcho::sendData() {
	string data(size*2, 0);
	char* d = &data[0];
	d[0] = (char)0xf1;
	d[1] = (char) 0xf2;
	*((int16_t*)(d + 2)) = id;

	*((int*)(d + 4)) = tag;
	int index = 0;
	int realSize = 0;
	int sendSize = 0;
	uint64_t startTime = GetNowTime();
	uint64_t nowTime;
	uint64_t timeSpan = 0;

	while (sendRunFlag) {
		nowTime = GetNowTime();
		timeSpan = (nowTime - startTime);
	
		if (timeSpan>0 && (allSendSize * 1000 / timeSpan) > speed) {
			Sleep(1);
			continue;
		}
		*((int*)(d + 8)) = index;
		realSize = size+rand(-size / 3, size / 3);//随机把包大小加减1/3
		if (realSize < MIN_SIZE) {
			realSize = MIN_SIZE;
		} else if (realSize > MAX_SIZE) {
			realSize = MAX_SIZE;
		}
		//修改三个字节的为随机值
		data[rand(MIN_SIZE, realSize)] = (char)rand(1, 255);
		data[rand(MIN_SIZE, realSize)] = (char)rand(1, 255);
		data[rand(MIN_SIZE, realSize)] = (char)rand(1, 255);

		sendTimes[index] = nowTime;
		sendSize = ::send(so, &data[0], realSize, 0);
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
	uint64_t nowTime;

	while (recvRunFlag) {
		recvSize = ::recv(so, &data[0], BUFFER_SIZE, 0);
		nowTime = GetNowTime();

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
			CERR("recv tag error,tag:" << tag);
			continue;
		}
		allRecvCount++;
		allRecvSize += recvSize;
	}
}

void UdpEcho::printResult() {
	uint64_t lastCount = allSendCount-allRecvCount  ;
	uint64_t lastSize = allSendSize-allRecvSize  ;
	double lastCountPercent = lastCount*100.00/ allSendCount;
	double lastSizePercent = lastSize*100.00/allSendSize;
	double sendCountPerSecond = allSendCount*1000.0 / (_stopTime -_startTime);
	double sendSizePerSecond = allSendSize*1000.0 / (_stopTime - _startTime);
	double sendSizePerPack = allSendSize *1.0/ allSendCount;
	double recvSizePerPack = allRecvSize *1.0 / allRecvCount;

	COUT("发送时间(毫秒):" << _stopTime - _startTime);

	if (lastCountPercent >= 100) {
		CERR("无法连接");
	} else if (lastCountPercent > 20) {
		CERR("丢包非常严重");
	} else if (lastCountPercent > 10) {
		COUT("丢包比较严重");
	} 
	//if (lastCountPercent < 100) {
	if(lastCountPercent!=1000){

		COUT("发送包数:" << allSendCount << ",发送流量:" << allSendSize << 
			",发送包平均大小:" << setiosflags(ios::fixed) << setprecision(2) << sendSizePerPack);
		COUT("接收包数:" << allRecvCount << ",接收流量:" << allRecvSize << 
			",接收包平均大小:" << setiosflags(ios::fixed) << setprecision(2) << recvSizePerPack);

		COUT("每秒发送包数:" << setiosflags(ios::fixed) << setprecision(2)<<sendCountPerSecond <<
			",每秒发送流量:" << setiosflags(ios::fixed) << setprecision(2)<<sendSizePerSecond);

		COUT("丢包数量:" << lastCount << ",丢包流量:" << lastSize);
		COUT("丢包数量百分比:" << setiosflags(ios::fixed) << setprecision(2)<<
			lastCountPercent << "%,丢包流量百分比:" << setiosflags(ios::fixed) << setprecision(2) << lastSizePercent<<"%");
		
		
		map<pair<uint64_t, uint64_t>, int> times;
		times[pair<uint64_t, uint64_t>(0, 10)] = 0;
		times[pair<uint64_t, uint64_t>(10, 20)] = 0;
		times[pair<uint64_t, uint64_t>(20, 50)] = 0;
		times[pair<uint64_t, uint64_t>(50, 100)] = 0;
		times[pair<uint64_t, uint64_t>(100, 150)] = 0;
		times[pair<uint64_t, uint64_t>(150, 200)] = 0;
		times[pair<uint64_t, uint64_t>(200, 300)] = 0;
		times[pair<uint64_t, uint64_t>(300, 500)] = 0;
		times[pair<uint64_t, uint64_t>(500, 700)] = 0;
		times[pair<uint64_t, uint64_t>(700, 1000)] = 0;
		times[pair<uint64_t, uint64_t>(1000, 1500)] = 0;
		times[pair<uint64_t, uint64_t>(1500, 2000)] = 0;
		times[pair<uint64_t, uint64_t>(2000, 3000)] = 0;
		times[pair<uint64_t, uint64_t>(3000, 4000)] = 0;
		times[pair<uint64_t, uint64_t>(4000, 5000)] = 0;
		times[pair<uint64_t, uint64_t>(5000, 7000)] = 0;
		times[pair<uint64_t, uint64_t>(7000, 10000)] = 0;
		times[pair<uint64_t, uint64_t>(10000, 1000000000)] = 0;


	
		for (auto item = recvTimes.begin(); item != recvTimes.end(); item++) {
			uint64_t t = item->second - sendTimes[item->first];
			for (auto item1 = times.begin(); item1 != times.end(); item1++) {
				if (t >= item1->first.first && t < item1->first.second) {
					item1->second++;
					break;
				}
			}
		}
		COUT("接收数据包详情");
		for (auto item = times.begin(); item != times.end(); item++) {
			uint64_t d1 = item->first.first;
			uint64_t d2 = item->first.second;
			int count = item->second;
			if (count > 0) {
				double p = count*100.0 / allRecvCount;
				COUT("时间(毫秒):[" << setw(4) << setfill('0') << d1 << ","
					<< setw(4) << setfill('0') << d2 << "),包数量:" << setw(5) << setfill('0') << count
					<< ",百分比:" << setw(5) << setfill('0') << setiosflags(ios::fixed) << setprecision(2) << p << "%");
			}
		}
	}
}

