#pragma once

constexpr int MAX_SPEED = 1024 * 50;//最大速度为50KB
//constexpr int RECV_TIMEOUT = 10000;//超时时间
constexpr int BUFFER_SIZE = 1024 * 100;//收发缓存大小
constexpr int HEAD_SIZE = 2 + 2 + 4 + 4;//命令包头大小
constexpr int MAX_SIZE = 1472;//发送包的最大值
constexpr int MIN_SIZE = HEAD_SIZE;//发送包最小值

//HEAD Content
//2:SYNC=0xF1,0xF2
//2:RESV
//4:Tag
//4:Index

class UdpEcho {
public:
	UdpEcho(string ip,int port,int speed,int size,int tag,int16_t id);
	~UdpEcho();
	bool start();
	void stopSend();
	void stop();
private:
	void sendData();
	void recvData();
	void printResult();
private:
	const string ip;
	const int port;
	const int speed;
	const int size;
	const int tag;
	const int id;
	unique_ptr<thread>	sendThread;
	unique_ptr<thread>	recvThread;

	bool sendRunFlag;
	bool recvRunFlag;

	SOCKET so;
private:
	uint64_t allSendCount;
	uint64_t allSendSize;
	uint64_t allRecvCount;
	uint64_t allRecvSize;
	uint64_t _startTime;
	uint64_t _stopTime;
	map<int, uint64_t> sendTimes;
	map<int, uint64_t> recvTimes;
};

