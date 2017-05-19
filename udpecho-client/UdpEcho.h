#pragma once

class UdpEcho {
public:
	UdpEcho(string ip,int port,int speed,int size);
	~UdpEcho();
	bool start();
	void stop();
private:
	void send();
	void recv();
private:
	const string ip;
	const int port;
	const int speed;
	const int size;

	unique_ptr<thread>	sendThread;
	unique_ptr<thread>	recvThread;

	bool runFlag;
	SOCKET so;
};

