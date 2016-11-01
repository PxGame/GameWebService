# include "GSoapManager.h"
# include "pub.h"
# include "Common.h"
# include "DBManager.h"

GSoapManager::GSoapManager(
	int port,
	int backlog,
	int maxThr,
	int maxSock,
	const string& userId,
	const string& pwd)
{
	_port = port;
	_backLog = backlog;
	_maxThr = maxThr;
	_maxSsockPool = maxSock;
	_authUserId = userId;
	_authPwd = pwd;
}

GSoapManager::~GSoapManager()
{
}

bool GSoapManager::Init()
{
	fprintf(stderr, "Init.\n");
	//初始话gSoap
	soap_init(&_mainSoap);
	_mainSoap.userid = _authUserId.c_str();
	_mainSoap.passwd = _authPwd.c_str();

	soap_register_plugin_arg(&_mainSoap, http_post, (void*)ghttpPostHndlers);
	soap_register_plugin_arg(&_mainSoap, http_get, (void*)HttpGetHandler);

	SOAP_SOCKET ssock = soap_bind(&_mainSoap, 0, _port, _backLog);
	if (!soap_valid_socket(ssock))
	{
		return false;
	}

	//初始化同步
	MUTEX_SETUP(_mutexThr);
	COND_SETUP(_condThr);

	//创建处理线程
	_soapPool.clear();
	_thrPool.clear();
	struct soap* spTemp = 0;
	THREAD_TYPE thrTemp = 0;
	for (int i = 0; i < _maxThr; i++)
	{
		spTemp = soap_copy(&_mainSoap);
		THREAD_CREATE(&thrTemp, FIXEDTHREADFUNC(ProcessThread), (void*)new SoapThreadArg(this, i));

		_soapPool.push_back(spTemp);
		_thrPool.push_back(thrTemp);
	}

	return true;
}

void GSoapManager::Destory()
{
	fprintf(stderr, "Destory start.\n");
	for (int i = 0; i < _maxThr; i++)
	{
		while (Enqueue(SOAP_INVALID_SOCKET) == SOAP_EOM)
		{
			SLEEP(1);
		}
	}

	for (int i = 0; i < _maxThr; i++)
	{
		THREAD_JOIN(_thrPool[i]);
		soap_done(_soapPool[i]);
		free(_soapPool[i]);
	}

	MUTEX_CLEANUP(_mutexThr);
	COND_CLEANUP(_condThr);

	soap_destroy(&_mainSoap);
	soap_end(&_mainSoap);
	soap_done(&_mainSoap);
	fprintf(stderr, "Destory end.\n");
}

void GSoapManager::Run()
{
	fprintf(stderr, "Run start.\n");
	if (!Init())
	{

	}
	SOAP_SOCKET ssTemp;
	while (true)
	{
		ssTemp = soap_accept(&_mainSoap);
		if (!soap_valid_socket(ssTemp))
		{
			if (_mainSoap.errnum)
			{
				soap_print_fault(&_mainSoap, stderr);
				continue;//重试
			}
			else
			{
				fprintf(stderr, "Server timed out\n");
				break;
			}
		}

		fprintf(stderr, "accepts socket %lu connection from IP %s\n",
			(unsigned long)ssTemp, TranslateIpToString(_mainSoap.ip).c_str());

		while (Enqueue(ssTemp) == SOAP_EOM)
		{
			SLEEP(1);
		}
	}
	Destory();
	fprintf(stderr, "Run end.\n");
}

void GSoapManager::RunOnThread()
{
	THREAD_CREATE(&_mainLoopThr, FIXEDTHREADFUNC(GSoapManager::LoopThread), (void*)new SoapThreadArg(this, 0));
}

void GSoapManager::Close()
{
	fprintf(stderr, "Close.\n");
	soap_done(&_mainSoap);
}

void GSoapManager::WaitThread()
{
	fprintf(stderr, "WaitThread.\n");
	THREAD_JOIN(_mainLoopThr);
}

int GSoapManager::Enqueue(SOAP_SOCKET ssock)
{
	int status = SOAP_OK;
	MUTEX_LOCK(_mutexThr);

	if (_ssockPool.size() != _maxSsockPool)
	{
		_ssockPool.push(ssock);
		COND_SIGNAL(_condThr);
	}
	else
	{
		status = SOAP_EOM;
	}

	MUTEX_UNLOCK(_mutexThr);

	return status;
}

SOAP_SOCKET GSoapManager::Dequeue()
{
	SOAP_SOCKET ssock;

	MUTEX_LOCK(_mutexThr);

	while (_ssockPool.size() == 0)
	{
		COND_WAIT(_condThr, _mutexThr);
	}

	ssock = _ssockPool.front();
	_ssockPool.pop();

	MUTEX_UNLOCK(_mutexThr);
	return ssock;
}

void * GSoapManager::LoopThread(void * arg)
{
	SoapThreadArg* thrArg = (SoapThreadArg*)arg;
	GSoapManager* manager = thrArg->main;

	manager->Run();

	delete thrArg;
	THREAD_EXIT;
	return 0;
}

void * GSoapManager::ProcessThread(void * arg)
{
	SoapThreadArg* thrArg = (SoapThreadArg*)arg;
	GSoapManager* manager = thrArg->main;
	struct soap* soap = manager->_soapPool[thrArg->index];

	fprintf(stderr, "ProcessThread start %d.\n", thrArg->index);
	while (true)
	{
		soap->socket = manager->Dequeue();
		if (!soap_valid_socket(soap->socket))
		{
			break;
		}

		soap_serve(soap);
		soap_destroy(soap);
		soap_end(soap);
	}

	fprintf(stderr, "ProcessThread end %d.\n", thrArg->index);
	delete thrArg;
	THREAD_EXIT;
	return 0;
}

int GSoapManager::CheckAuthorization(soap * soap)
{
	if (!soap->userid || !soap->passwd ||
		strcmp(soap->userid, _authUserId.c_str()) ||
		strcmp(soap->passwd, _authPwd.c_str()))
	{
		//认证失败 401
		return 401;
	}

	return 0;
}

GSoapManager* GSoapManager::_instance = 0;

void GSoapManager::Create(
	int port,
	int backlog,
	int maxThr,
	int maxSock,
	const string& userId,
	const string& pwd)
{
	_instance = new GSoapManager(
		port,
		backlog,
		maxThr,
		maxSock,
		userId,
		pwd);
}

GSoapManager * GSoapManager::get()
{
	return _instance;
}

//==========================================

int HttpGetHandler(struct soap* soap)
{
	fprintf(stderr, "HttpGetHandler\n");
	if (GSoapManager::get()->CheckAuthorization(soap))
	{//认证失败
		return 401;
	}

	Connection* ss = DBManager::get()->GetConnection();

	DBManager::get()->RegistUser(ss, "xiangmu", "123", "abc");

	DBManager::get()->ReleaseConnection(ss);
	soap_response(soap, SOAP_HTML);
	soap_send(soap, "<html>HttpGetHandler</html>");
	soap_end_send(soap);
	return SOAP_OK;
}

int HttpPostHandler(struct soap* soap)
{
	fprintf(stderr, "HttpPostHandler\n");
	if (GSoapManager::get()->CheckAuthorization(soap))
	{//认证失败
		return 401;
	}

	soap_response(soap, SOAP_HTML);
	soap_send(soap, "<html>HttpPostHandler</html>");
	soap_end_send(soap);
	return SOAP_OK;
}