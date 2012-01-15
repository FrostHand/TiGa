#pragma once
class InetClients
{
public:	
	///���������� � �����, ������ ������� �� ������� �������� ��� ������� �������
	virtual void onClientNew(int Id)=0;
	virtual void onClientLost(int Id)=0;
	virtual void onDisconnect()=0;
};

namespace netLowHigh // low and high level network IO
{
	bool init(lua_State *L,InetClients *Reader);
	void done();

/// ������� ������� - ����������� ������ � �������� ������
/// ������������ ��� �������� ����� �����-�� 0-1
	void switchSock(int SockN);

	// - ��� ���� ���� �������� ��� - �� ����� ��������� � ���� �������, ������� ������� �������
	// �����������
	bool doConnect(char *Name,int Port,bool attachLua=false);
	// ��������� ������
	bool doListen(int Port,bool attachLua=false);

///  �������� ��� ����� (������), 0 - ����
	int getMyAddr();

/// ���� �������� ������-�� ��������� Ch (0-255)
	void send(int Addr,int Ch,char *Data, int Len);
// ���� �����������, �������� ��� ������� ��� ������� � ����� �����
	void Update();
}

namespace netLowHigh
{
	bool isConnected();
	void send(char const *data,size_t sz);
	std::string readBuffer();
	//void freeBuffer();
	void run();
}

