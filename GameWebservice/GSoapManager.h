# ifndef GSOAPMANAGER_H
# define GSOAPMANAGER_H
# include "pub.h"

class GSoapManager;

typedef struct SoapThreadArg {
	GSoapManager* main;
	int index;
	SoapThreadArg(GSoapManager* m, int i)
	{
		main = m;
		index = i;
	}
}SOAPTHREADARG, *PSOAPTHREADARG;

int HttpGetHandler(struct soap* soap);
int HttpPostHandler(struct soap* soap);

//Post根据mime分配回调
static struct http_post_handlers ghttpPostHndlers[] =
{
	{ "*/*", HttpPostHandler },
	{ 0 }
};

class GSoapManager
{
private:
	GSoapManager() {}
	GSoapManager(
		int port,
		int backlog,
		int maxThr,
		int maxSock,
		const string& userId,
		const string& pwd);
	~GSoapManager();

private:
	//config
	int _port;
	int _backLog;
	int _maxThr;
	int _maxSsockPool;

	string _authUserId;
	string _authPwd;
	//

	vector<struct soap*> _soapPool;
	vector<THREAD_TYPE> _thrPool;
	queue<SOAP_SOCKET> _ssockPool;

	struct soap _mainSoap;

	THREAD_TYPE _mainLoopThr;

	MUTEX_TYPE _mutexThr;
	COND_TYPE _condThr;

public:
	bool Init();
	void Destory();
	void Run();
	void RunOnThread();
	void Close();
	void WaitThread();

private:
	int Enqueue(SOAP_SOCKET ssock);
	SOAP_SOCKET Dequeue();

private:
	static void* LoopThread(void* arg);
	static void* ProcessThread(void* arg);

public:
	//认证用户
	int CheckAuthorization(struct soap* soap);

private:
	static GSoapManager* _instance;

public:
	static void Create(
		int port,
		int backlog,
		int maxThr,
		int maxSock,
		const string& userId,
		const string& pwd);
	static GSoapManager* get();
};

# endif