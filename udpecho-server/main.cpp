#include <cstdio>
#include <cassert>
#include <signal.h>
#include <winsock2.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>
#include <direct.h>

#include "common/Util.h"
#pragma comment(lib,"ws2_32.lib")

using namespace std;

constexpr int DEFAULT_PORT = 45005;//默认端口
constexpr int RECV_TIMEOUT = 2000;//超时时间
constexpr int BUFFER_SIZE = 1024*1024*5;//收发缓存大小

#define LOG_COUT 0
#define LOG_CERR 1

static void WriteLog(const string & content, int type);

#define COUT(V) \
do{ ostringstream os ; \
	os<<V; \
	WriteLog(os.str(),LOG_COUT);\
}while (0)

#define CERR(V) \
do{ ostringstream os ; \
	os<<V; \
	WriteLog(os.str(),LOG_CERR);\
}while (0)
static FILE* logFile = NULL;


SOCKET _socket = INVALID_SOCKET;

struct Info {
	int tag;
	int16_t id;
	int recvCount;
	int lastIndex;
	int lastTime;
};

map<int, Info> recvInfos;

bool Open(int port) {

	if (port <= 0 || port >= 65535) {
		CERR ( "port is error :" << port );
		return false;
	}


	//初始化WSA
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		CERR ( "WSAStartup error" );
		return false;
	}

	int ret = 0;
	struct sockaddr_in addr;

	_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (_socket == INVALID_SOCKET) {
		CERR ( "socket create fail" );
		return false;
	}
	
	addr.sin_family = AF_INET; //地址家族
	addr.sin_port = htons(port); //注意转化为网络字节序
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); //使用INADDR_ANY 指示任意地址
	int flag1 = 1;
	setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag1, sizeof(flag1));

	flag1 = BUFFER_SIZE;
	setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (const char*)&flag1, sizeof(flag1));
	setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, (const char*)&flag1, sizeof(flag1));

	flag1 = RECV_TIMEOUT;
	setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&flag1, sizeof(flag1));

	ret = bind(_socket, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == SOCKET_ERROR) {
		closesocket(_socket); //关闭套接字
		_socket = INVALID_SOCKET;
		COUT("bind error !");
		return false;
	}
	return true;
}

void printInfo(const Info &info) {
	COUT("tag:" << (info.tag) <<",id:"<<info.id  << ",count:" << info.recvCount <<",index:"<<info.lastIndex);
}

void testInfo(int time) {
	auto item = recvInfos.begin();
	while (item != recvInfos.end()) {
		Info& info = item->second;
		if (info.lastTime > time || time - info.lastTime > 5) {
			printInfo(info);
			item = recvInfos.erase(item);
		} else {
			++item;
		}
	}
}

void Work() {
	uint8_t *buff = new uint8_t[BUFFER_SIZE];
	int recvSize;
	int sendSize;
	struct sockaddr_in addrFrom;
	int fromLen = 0;
	int tag;
	int16_t id;
	int index;
	int count = 0;
	int nowTime;
	bool hasTag = false;
	while (true) {
		fromLen = sizeof(addrFrom);
		recvSize = recvfrom(_socket, (char*)buff, BUFFER_SIZE, 0, (struct sockaddr *)&addrFrom, &fromLen);
		
		nowTime = (int)time(0);
		if (recvSize == 0) {
			CERR("socket closed,error:"<< WSAGetLastError());
			break;
		}

		if (recvSize < 0) {
			testInfo(nowTime);
			continue;
		} 

		id = *((int16_t*)(buff + 2));
		tag = *((int*)(buff + 4));
		index = *((int*)(buff + 8));
		hasTag = recvInfos.count(tag) > 0;
		Info &info = recvInfos[tag];
		if (hasTag) {
			if (info.id != id) {
				printInfo(info);
				info.recvCount = 0;
			}
		} else {
			info.recvCount = 0;
		}
		info.id = id;
		info.tag = tag;
		info.lastIndex = index;
		info.lastTime = nowTime;
		info.recvCount++;
			
		sendSize = sendto(_socket, (char*)buff, recvSize, 0, (struct sockaddr *)&addrFrom, sizeof(addrFrom));
		if (sendSize <= 0) {
			CERR("send failed, ip:" << inet_ntoa(addrFrom.sin_addr) << ",port:" << ntohs(addrFrom.sin_port)<<",tag:"<<
				tag<<",id:"<<id<<",ret:"<< sendSize);
		}

		if (++count > 200) {
			testInfo(nowTime);
			count = 0;
		}
	}
	delete[]buff;
}

void Close() {
	closesocket(_socket); //关闭套接字
	_socket = INVALID_SOCKET;
}

void Handler(int sig) {
	if (sig == SIGINT) {
		Close();
		COUT("user terminaled!");
		exit(-1);
	}
}

string readAllInput() {
	char c;
	string str;
	while ((c = cin.get()) != '\n') {
		str.push_back(c);
	}

	return str;
}

int main(int argc, char* argv[]) {
	signal(SIGINT, Handler);
	string logFilePath = "udpecho-server\\";
	_mkdir(logFilePath.c_str());
	logFilePath.append(nowDateStr());
	logFilePath.append(".txt");
	logFile = fopen(logFilePath.c_str(), "ab");

	//int k = 22 % (-3);

	//while (true) {
	//	int k = rand(-2, 2);
	//	cout << k;
	//}

	int port = -1;
	if (argc > 1)
		port = StringToInt(argv[1], port);

	if (port < 0) {
		cout << "input port(" << DEFAULT_PORT << "):";
		string str = readAllInput();
		port = StringToInt(str.c_str(), DEFAULT_PORT);
	}

	if (!Open(port))
		return -1;
	COUT("started,port:" << port);
	Work();
	Close();
	COUT("stopped!" );
	if (logFile != NULL) {
		fclose(logFile);
	}
}

static void WriteLog(const string & content, int type) {
	static const char* LEVEL[] = { "DEBUG", "ERROR" };

	string str = nowTimeStr();
	str.append(": ").append(LEVEL[type]).append(":").append(content).append("\r\n");
	cout << str;

	if (logFile) {
		fwrite(str.c_str(), str.length(), 1, logFile);
		fflush(logFile);
	}
}
