#include <cstdio>
#include <cassert>
#include <signal.h>

#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <time.h>
#include <sstream>

#ifdef WIN32
#define _WIN32_
#endif

#ifdef _WIN32_
#   include <winsock2.h>
#   include <direct.h>
#else
#   include <sys/socket.h>
#   include <sys/types.h>
#   include <netinet/in.h>
#   include <unistd.h>
#   include <arpa/inet.h>
#   include <sys/stat.h>
#   define SOCKET int
#   define INVALID_SOCKET -1
#   define SOCKET_ERROR -1
#endif


using namespace std;

constexpr int DEFAULT_PORT = 45005;//默认端口
constexpr int RECV_TIMEOUT = 2000;//超时时间
constexpr int BUFFER_SIZE = 1024*1024*5;//收发缓存大小

#define LOG_COUT 0
#define LOG_CERR 1

static void WriteLog(const string & content, int type);
std::string nowTimeStr();
std::string nowDateStr();
int StringToInt(const char* str, int defaultValue);
std::string IntToString(int i);
int rand(int min, int max);

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

#ifdef _WIN32_
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		CERR ( "WSAStartup error" );
		return false;
	}
#endif
	int ret = 0;
	struct sockaddr_in addr;

	_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (_socket == INVALID_SOCKET) {
		CERR ( "socket create fail" );
		return false;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
#ifdef _WIN32_
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#else
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
	int flag1 = 1;
    ret = setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag1, sizeof(flag1));

	flag1 = BUFFER_SIZE;
    ret = setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (const char*)&flag1, sizeof(flag1));
    ret = setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, (const char*)&flag1, sizeof(flag1));
#ifdef _WIN32_
	flag1 = RECV_TIMEOUT;
    ret = setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&flag1, sizeof(flag1));
#else
    struct timeval timeout={RECV_TIMEOUT/1000,0};//3s
    ret = setsockopt(_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#endif

	ret = bind(_socket, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == SOCKET_ERROR) {
#ifdef _WIN32_
		closesocket(_socket);
#else
        close(_socket);
#endif
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

	struct sockaddr_in addrFrom;
#ifdef _WIN32_
	int fromLen = 0;
    int recvSize;
	int sendSize;
#else
    socklen_t fromLen = 0;
    ssize_t recvSize;
    ssize_t sendSize;
#endif
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
#ifdef  _WIN32_
			CERR("socket closed,error:"<< WSAGetLastError());
#else
            CERR("socket closed");
#endif
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
#ifdef _WIN32_
	closesocket(_socket);
#else
    close(_socket);
#endif
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
#ifdef _WIN32_
	_mkdir(logFilePath.c_str());
#else
    mkdir(logFilePath.c_str(),777);
#endif
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


std::string nowTimeStr() {
	char str[255];
	time_t t = time(NULL);
	tm* t2;
	t2 = localtime(&t);
	strftime(str, sizeof(str), "%Y-%m-%d %H:%M:%S", t2);
	return std::string(str);
}

std::string nowDateStr() {
	char str[255];
	time_t t = time(NULL);
	tm* t2;
	t2 = localtime(&t);
	strftime(str, sizeof(str), "%Y%m%d", t2);
	return std::string(str);
}


int StringToInt(const char * str, int defaultValue) {
	istringstream is(str);
	int k = defaultValue;
	is >> k;
	if (!is.eof())
		return defaultValue;
	return k;
}

std::string IntToString(int i) {

	ostringstream os;
	os << i;

	return os.str();
}

int rand(int min, int max) {
	if (min > max)
		return min;
	if (max == min)
		return min;
	return (rand() % (max - min)) + min;
}