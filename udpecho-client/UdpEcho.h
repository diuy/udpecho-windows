#pragma once

constexpr int MAX_SPEED = 1024 * 300;//����ٶ�Ϊ100MB
//constexpr int RECV_TIMEOUT = 10000;//��ʱʱ��
constexpr int BUFFER_SIZE = 1024 * 100;//�շ������С
constexpr int HEAD_SIZE = 2 + 2 + 4 + 4;//�����ͷ��С
constexpr int MAX_SIZE = 1472;//���Ͱ������ֵ�����������0-1��������
constexpr int MIN_SIZE = HEAD_SIZE;//���Ͱ���Сֵ

//HEAD Content
//2:SYNC=0xF1,0xF2
//2:RESV
//4:Tag
//4:Index

class UdpEcho {
public:
	UdpEcho(string ip,int port,int speed,int size,int tag);
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
	unique_ptr<thread>	sendThread;
	unique_ptr<thread>	recvThread;

	bool sendRunFlag;
	bool recvRunFlag;

	SOCKET so;
private:
	int allSendCount;
	int allSendSize;
	int allRecvCount;
	int allRecvSize;
	DWORD _startTime;
	DWORD _stopTime;
	map<int, DWORD> sendTimes;
	map<int, DWORD> recvTimes;
};

