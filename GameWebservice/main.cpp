# include "pub.h"
# include "Common.h"
# include "ProcessHttpForm.h"
# include "ProcessHttpGet.h"
# include "ProcessHttpPost.h"

# include "GameWebservice.nsmap"//该文件不要重复包含，会产生重复定义

//====== Globe parameter

struct soap *gSoapthr[MAX_THR] = { 0 };
THREAD_TYPE gThrIds[MAX_THR] = { 0 };

//Socket队列
SOAP_SOCKET gQueSocket[MAX_QUEUE] = {0};
//线程队列头
int gQueHeadSktIdx = 0;
//线程队列尾
int gQueTailSktIdx = 0;

//主循环线程句柄
THREAD_TYPE gServiceLoopThr = 0;

//线程同步
MUTEX_TYPE gQueMutex;
COND_TYPE gQueCond;

//主soap
struct soap gMainSoap;
//===================

void InitGlobeArg();
int EnqueueSocket(SOAP_SOCKET);
SOAP_SOCKET DequeueSocket();
void* ServiceLoopThread(void*);
void* ProcessQueThread(void*);

//===

struct http_post_handlers ghttpPostHndlers[] =
{ 
	{ "*/*", HttpPostHandler },
	{ 0 }
};

int main(int argc, char* argv[])
{
	char operation[64];
	fprintf(stderr, "GameWebservice %ld Launching...\n", THREAD_ID);
	
	while (true)
	{
		scanf("%s", operation);
		fflush(0);
		if (0 == strcmp(operation, "start"))
		{
			//服务主循环
			THREAD_CREATE(&gServiceLoopThr, FIXEDTHREADFUNC(ServiceLoopThread), 0);
		}
		else if(0 == strcmp(operation, "stop"))
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

	fprintf(stderr, "GameWebservice %ld quit.\n", THREAD_ID);
	return 0;
}

//初始化全局变量
void InitGlobeArg()
{
	int i = 0;

	for (i = 0; i < MAX_THR; i++)
	{
		gSoapthr[i] = 0;
		gThrIds[i] = 0;
	}

	//Socket队列
	for (i = 0; i < MAX_QUEUE; i++)
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

	fprintf(stderr, "ServiceLoopThread %d Launching...\n", THREAD_ID);
	InitGlobeArg();

	//初始化
	soap_init(&gMainSoap);

	//设置账号密码
	gMainSoap.userid = AUTH_USERID;
	gMainSoap.passwd = AUTH_PWD;

	//添加回调
	soap_register_plugin_arg(&gMainSoap, http_post, (void*)ghttpPostHndlers);
	soap_register_plugin_arg(&gMainSoap, http_get, (void*)HttpGetHandler);
	soap_register_plugin_arg(&gMainSoap, http_form, (void*)HttpFormHandler);

	do
	{
		//绑定端口
		sMainSocket = soap_bind(&gMainSoap, 0, gPort, BACKLOG);
		if (!soap_valid_socket(sMainSocket))
		{
			break;
		}

		//设置同步变量
		MUTEX_SETUP(gQueMutex);
		COND_SETUP(gQueCond);

		//创建处理线程
		for (i = 0; i < MAX_THR; i++)
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

			fprintf(stderr, "Thread %d accepts socket %d connection from IP %d.%d.%d.%d\n", 
				(int)gThrIds[i], (int)sTemp, (int)(gMainSoap.ip >> 24) & 0xFF, (int)(gMainSoap.ip >> 16) & 0xFF, (int)(gMainSoap.ip >> 8) & 0xFF, (int)gMainSoap.ip & 0xFF);

			while (EnqueueSocket(sTemp) == SOAP_EOM)
			{
				SLEEP(1);
			}
		}

		//退出所有线程
		for (i = 0; i < MAX_THR; i++)
		{
			while (EnqueueSocket(SOAP_INVALID_SOCKET) == SOAP_EOM)
			{
				SLEEP(1);
			}
		}

		for (i = 0; i < MAX_THR; i++)
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

	fprintf(stderr, "ServiceLoopThread %d Exit.\n", THREAD_ID);
	THREAD_EXIT;
	return 0;
}

//消息队列出列线程
void* ProcessQueThread(void* soap)
{
	fprintf(stderr, "ProcessQueThread %d Start.\n", THREAD_ID);

	struct soap* tsoap = (struct soap*)soap;
	for (;;)
	{
		tsoap->socket = DequeueSocket();
		if (!soap_valid_socket(tsoap->socket))
		{
			break;
		}

		soap_serve(tsoap);
		soap_destroy(tsoap);
		soap_end(tsoap);
	}

	fprintf(stderr, "ProcessQueThread %d Exit.\n", THREAD_ID);
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
	if (next >= MAX_QUEUE)
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

	if (gQueHeadSktIdx >= MAX_QUEUE)
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
