#include <cstdio>
#include <cassert>
#include <signal.h>
#include <winsock2.h>
#include <iostream>
#include <iomanip>
#include <map>

#include "common/Util.h"
#pragma comment(lib,"ws2_32.lib")

using namespace std;

constexpr int DEFAULT_PORT = 40000;//默认端口
constexpr int RECV_TIMEOUT = 10000;//超时时间
constexpr int BUFFER_SIZE = 1024*100;//收发缓存大小


#define COUT(V) std::cout<<nowTimeStr()<<": "<<V<<endl
#define CERR(V) std::cerr<<nowTimeStr()<<": "<<V<<endl

SOCKET _socket = INVALID_SOCKET;

map<int, int> recvInfos;

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



void Work() {
	uint8_t *buff = new uint8_t[BUFFER_SIZE];
	int recvSize;
	int sendSize;
	struct sockaddr_in addrFrom;
	int fromLen = 0;
	int tag;
	//int index;
	while (true) {
		fromLen = sizeof(addrFrom);
		recvSize = recvfrom(_socket, (char*)buff, BUFFER_SIZE, 0, (struct sockaddr *)&addrFrom, &fromLen);
		if (recvSize == 0) {
			CERR("socket closed,error:"<< WSAGetLastError());
			break;
		}
		if (recvSize < 0) {
			for (auto item = recvInfos.begin(); item != recvInfos.end(); item++) {
				COUT ("tag:" << (item->first)<<",count:"<<item->second);
			}
			recvInfos.clear();
		} else {
			tag = *((int*)(buff + 4));
			//index = *((int*)(buff + 8));
			recvInfos[tag]++;
			sendSize = sendto(_socket, (char*)buff, recvSize, 0, (struct sockaddr *)&addrFrom, sizeof(addrFrom));
			if (sendSize <= 0) {
				CERR("send failed, ip:" << inet_ntoa(addrFrom.sin_addr) << ",port:" << addrFrom.sin_port << ",ret:"<< sendSize);
			}
		}
	}
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
	COUT("stoped!" );
}

