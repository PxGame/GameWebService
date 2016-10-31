# include "pub.h"
# include "Common.h"
# include "ProcessHttpForm.h"
# include "ProcessHttpGet.h"
# include "ProcessHttpPost.h"
# include "DBManager.h"
# include "GameWebservice.nsmap"//该文件不要重复包含，会产生重复定义

//====== Globe parameter

static struct soap **gSoapthr;
static THREAD_TYPE *gThrIds;

//Socket队列
static SOAP_SOCKET *gQueSocket;
//线程队列头
static int gQueHeadSktIdx = 0;
//线程队列尾
static int gQueTailSktIdx = 0;

//主循环线程句柄
static THREAD_TYPE gServiceLoopThr = 0;

//线程同步
static MUTEX_TYPE gQueMutex;
static COND_TYPE gQueCond;

//主soap
static struct soap gMainSoap;

//DBManager
static DBManager* gDbManager = 0;


//===================

void InitGlobeArg();
int EnqueueSocket(SOAP_SOCKET);
SOAP_SOCKET DequeueSocket();
void* ServiceLoopThread(void*);
void* ProcessQueThread(void*);

//===

//Post根据mime分配回调
static struct http_post_handlers ghttpPostHndlers[] =
{
	{ "*/*", HttpPostHandler },
	{ 0 }
};

//int main1()
//{
//	char* name = "xxxxxxx";
//	gDbManager = new DBManager(gDbHostName, gDbUserName, gDbPassword);
//	auto_ptr<Connection> conn(gDbManager->GetNewConnection());
//	if (gDbManager->RegistUser(conn.get(), name, "123456","123"))
//	{
//		printf("ok");
//	}
//	else
//	{
//		printf("failed");
//	}
//	if (gDbManager->LoginUpdate(conn.get(), name, "abc", "qqq"))
//	{
//		printf("ok");
//	}
//	else
//	{
//		printf("failed");
//	}
//	USERINFO userinfo;
//	if (gDbManager->QueryUserInfo(conn.get(), name,userinfo))
//	{
//		printf("ok");
//	}
//	else
//	{
//		printf("failed");
//	}
//	return 0;
//}

int main(int argc, char* argv[])
{
	gDbManager = new DBManager(gDbHostName, gDbUserName, gDbPassword);

	char operation[64];
	fprintf(stderr, "GameWebservice Launching...\n");

	while (true)
	{
		scanf("%s", operation);
		fflush(0);
		if (0 == strcmp(operation, "start"))
		{
			//服务主循环
			THREAD_CREATE(&gServiceLoopThr, FIXEDTHREADFUNC(ServiceLoopThread), 0);
		}
		else if (0 == strcmp(operation, "stop"))
		{
			soap_done(&gMainSoap);
		}
		else if (0 == strcmp(operation, "quit"))
		{
			soap_done(&gMainSoap);
			break;
		}

		SLEEP(1);
	}

	//等待主循环结束
	THREAD_JOIN(gServiceLoopThr);
	delete gDbManager;
	fprintf(stderr, "GameWebservice quit.\n");

	return 0;
}

//初始化全局变量
void InitGlobeArg()
{
	int i = 0;

	if (gSoapthr != 0)
	{
		delete[] gSoapthr;
	}
	gSoapthr = new struct soap*[gMaxThr];
	if (gThrIds != 0)
	{
		delete[] gThrIds;
	}
	gThrIds = new THREAD_TYPE[gMaxThr];
	if (gQueSocket != 0)
	{
		delete[] gQueSocket;
	}
	gQueSocket = new SOAP_SOCKET[gMaxQueue];

	for (i = 0; i < gMaxThr; i++)
	{
		gSoapthr[i] = 0;
		gThrIds[i] = 0;
	}
	//Socket队列
	for (i = 0; i < gMaxQueue; i++)
	{
		gQueSocket[i] = 0;
	}


	//线程队列头
	gQueHeadSktIdx = 0;
	//线程队列尾
	gQueTailSktIdx = 0;
	//主循环线程句柄
	gServiceLoopThr = 0;
}

//服务主循环
void* ServiceLoopThread(void*)
{
	SOAP_SOCKET sMainSocket, sTemp;
	int i;

	fprintf(stderr, "ServiceLoopThread Launching...\n");
	InitGlobeArg();

	//初始化
	soap_init(&gMainSoap);

	//设置账号密码
	gMainSoap.userid = gAuthUserId;
	gMainSoap.passwd = gAuthPwd;

	//添加回调
	soap_register_plugin_arg(&gMainSoap, http_post, (void*)ghttpPostHndlers);
	soap_register_plugin_arg(&gMainSoap, http_get, (void*)HttpGetHandler);
	soap_register_plugin_arg(&gMainSoap, http_form, (void*)HttpFormHandler);

	do
	{
		//绑定端口
		sMainSocket = soap_bind(&gMainSoap, 0, gPort, gBackLog);
		if (!soap_valid_socket(sMainSocket))
		{
			break;
		}

		//设置同步变量
		MUTEX_SETUP(gQueMutex);
		COND_SETUP(gQueCond);

		//创建处理线程
		for (i = 0; i < gMaxThr; i++)
		{
			gSoapthr[i] = soap_copy(&gMainSoap);//拷贝soap
			THREAD_CREATE(&gThrIds[i], FIXEDTHREADFUNC(ProcessQueThread), (void*)gSoapthr[i]);//创建线程
		}

		//main loop
		for (;;)
		{
			sTemp = soap_accept(&gMainSoap);
			if (!soap_valid_socket(sTemp))
			{
				if (gMainSoap.errnum)
				{
					soap_print_fault(&gMainSoap, stderr);
					continue;//重试
				}
				else
				{
					fprintf(stderr, "Server timed out\n");
					break;
				}
			}

			fprintf(stderr, "Thread %lu accepts socket %lu connection from IP %s\n",
				(unsigned long)gThrIds[i], (unsigned long)sTemp, TranslateIpToString(gMainSoap.ip).c_str());

			while (EnqueueSocket(sTemp) == SOAP_EOM)
			{
				SLEEP(1);
			}
		}

		//退出所有线程
		for (i = 0; i < gMaxThr; i++)
		{
			while (EnqueueSocket(SOAP_INVALID_SOCKET) == SOAP_EOM)
			{
				SLEEP(1);
			}
		}

		for (i = 0; i < gMaxThr; i++)
		{
			THREAD_JOIN(gThrIds[i]);
			soap_done(gSoapthr[i]);
			free(gSoapthr[i]);
		}

		//销毁同步变量
		MUTEX_CLEANUP(gQueMutex);
		COND_CLEANUP(gQueCond);

	} while (false);


	soap_destroy(&gMainSoap);
	soap_end(&gMainSoap);
	soap_done(&gMainSoap);

	fprintf(stderr, "ServiceLoopThread Exit.\n");
	THREAD_EXIT;
	return 0;
}

//消息队列出列线程
void* ProcessQueThread(void* soap)
{
	struct soap* tsoap = (struct soap*)soap;
	Connection* conn = gDbManager->GetNewConnection();
	fprintf(stderr, "ProcessQueThread Start. DB:%s\n", conn != 0 ? "true" : "false");

	for (;;)
	{
		tsoap->socket = DequeueSocket();
		if (!soap_valid_socket(tsoap->socket))
		{
			break;
		}
		tsoap->
		soap_serve(tsoap);
		soap_destroy(tsoap);
		soap_end(tsoap);
	}
	delete conn;
	fprintf(stderr, "ProcessQueThread Exit.\n");
	THREAD_EXIT;
	return 0;
}

//加入队列
int EnqueueSocket(SOAP_SOCKET sock)
{
	int status = SOAP_OK;
	int next;

	MUTEX_LOCK(gQueMutex);

	next = gQueTailSktIdx + 1;
	if (next >= gMaxQueue)
	{
		next = 0;
	}

	if (next == gQueHeadSktIdx)
	{
		status = SOAP_EOM;
	}
	else
	{
		gQueSocket[gQueTailSktIdx] = sock;
		gQueTailSktIdx = next;
		COND_SIGNAL(gQueCond);
	}

	MUTEX_UNLOCK(gQueMutex);
	return status;
}

//弹出队列
SOAP_SOCKET DequeueSocket()
{
	SOAP_SOCKET sock;

	MUTEX_LOCK(gQueMutex);

	while (gQueHeadSktIdx == gQueTailSktIdx)
	{
		COND_WAIT(gQueCond, gQueMutex);
	}

	sock = gQueSocket[gQueHeadSktIdx++];

	if (gQueHeadSktIdx >= gMaxQueue)
	{
		gQueHeadSktIdx = 0;
	}

	MUTEX_UNLOCK(gQueMutex);
	return sock;
}

//Rest don`t use this.
int ns__test(struct soap* soap)
{
	return SOAP_NO_METHOD;
}
