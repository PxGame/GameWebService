# include <iostream>
# include "soapH.h"
# include "threads.h"
# include "httpget.h"
# include "GameWebservice.nsmap"
using namespace std;

//Rest don`t use this.
int ns__test(struct soap* soap)
{
	return SOAP_NO_METHOD;
}

//====== configuration
# define BACKLOG (100)
# define MAX_THR (10)			//线程池大小
# define MAX_QUEUE (1000)		//请求队列大小
# define AUTH_USERID "admin"		//用户名
# define AUTH_PWD "123456"		//密码

//端口
int gPort = 8080;

//====== Globe parameter

struct soap *gSoapthr[MAX_THR] = { NULL };
THREAD_TYPE gThrIds[MAX_THR] = { NULL };

//Socket队列
SOAP_SOCKET gQueSocket[MAX_QUEUE] = {0};
//线程队列头
int gQueHeadSktIdx = 0;
//线程队列尾
int gQueTailSktIdx = 0;

//主循环线程句柄
THREAD_TYPE gServiceLoop = 0;

MUTEX_TYPE gQueMutex;
COND_TYPE gQueCond;

//===================

void InitGlobeArg();
int EnqueueSocket(SOAP_SOCKET);
SOAP_SOCKET DequeueSocket();
void* ServiceLoopThread(void*);
void ServiceLoopThreadExit();
void* ProcessQueThread(void*);
void ProcessQueThreadExit();
int CheckAuthorization(struct soap*);

int http_get(struct soap* soap);

//===

int main(int argc, char* argv[])
{
	cout << "GameWebservice Launching..." << endl;
	
	//服务主循环
	THREAD_CREATE(&gServiceLoop, (void(*)(void*))ServiceLoopThread, NULL);

	//等待主循环结束
	THREAD_JOIN(gServiceLoop);
	return 0;
}

//初始化全局变量
void InitGlobeArg()
{
	int i = 0;

	for (i = 0; i < MAX_THR; i++)
	{
		gSoapthr[i] = NULL;
		gThrIds[i] = NULL;
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
	gServiceLoop = 0;
}

//服务主循环
void* ServiceLoopThread(void*)
{
	struct soap soap;
	SOAP_SOCKET m, s;
	int i;

	cout << "ServiceLoop Launching..." << endl;
	InitGlobeArg();

	//初始化
	soap_init(&soap);

	//设置账号密码
	soap.userid = AUTH_USERID;
	soap.passwd = AUTH_PWD;

	//添加回调
	soap.fget = http_get;

	do
	{
		//绑定端口
		m = soap_bind(&soap, NULL, gPort, BACKLOG);
		if (!soap_valid_socket(m))
		{
			break;
		}

		//设置同步变量
		MUTEX_SETUP(gQueMutex);
		COND_SETUP(gQueCond);

		//创建处理线程
		for (i = 0; i < MAX_THR; i++)
		{
			gSoapthr[i] = soap_copy(&soap);//拷贝soap
			THREAD_CREATE(&gThrIds[i], (void(*)(void*))ProcessQueThread, (void*)gSoapthr[i]);//创建线程
		}

		for (;;)
		{
			s = soap_accept(&soap);
			if (!soap_valid_socket(s))
			{
				if (soap.errnum)
				{
					soap_print_fault(&soap, stderr);
					continue;//重试
				}
				else
				{
					fprintf(stderr, "Server timed out\n");
					break;
				}
			}

			fprintf(stderr, "Thread %d accepts socket %d connection from IP %d.%d.%d.%d\n", i, s, (soap.ip >> 24) & 0xFF, (soap.ip >> 16) & 0xFF, (soap.ip >> 8) & 0xFF, soap.ip & 0xFF);

			while (EnqueueSocket(s) == SOAP_EOM)
			{
				Sleep(1);
			}
		}

		//退出所有线程
		for (i = 0; i < MAX_THR; i++)
		{
			while (EnqueueSocket(SOAP_INVALID_SOCKET) == SOAP_EOM)
			{
				Sleep(1);
			}
		}

		for (i = 0; i < MAX_THR; i++)
		{
			fprintf(stderr, "Waiting for thread %d to terminate...\n", i);
			THREAD_JOIN(gThrIds[i]);
			fprintf(stderr, "terminated\n");

			soap_done(gSoapthr[i]);
			free(gSoapthr[i]);
		}

		//销毁同步变量
		MUTEX_CLEANUP(gQueMutex);
		COND_CLEANUP(gQueCond);

	} while (false);


	soap_destroy(&soap);
	soap_end(&soap);
	soap_done(&soap);

	THREAD_EXIT;
	ServiceLoopThreadExit();
	return NULL;
}

//服务主循环退出
void ServiceLoopThreadExit()
{//用于做一些退出处理
	fprintf(stderr, "ServiceLoopThread Exit.\n");

	DWORD thrId = THREAD_ID;
}

//消息队列出列线程
void* ProcessQueThread(void* soap)
{
	fprintf(stderr, "ProcessQueThread Start.\n");

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

		fprintf(stderr, "served.\n");
	}

	THREAD_EXIT;
	ProcessQueThreadExit();
	return NULL;
}

//处理线程退出
void ProcessQueThreadExit()
{
	fprintf(stderr, "ProcessQueThread Exit.\n");

	DWORD thrId = THREAD_ID;
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


//认证用户
int CheckAuthorization(struct soap* soap)
{
	if (!soap->userid || !soap->passwd ||
		strcmp(soap->userid, AUTH_USERID) ||
		strcmp(soap->passwd, AUTH_PWD))
	{
		//认证失败 401
		return 401;
	}

	return 0;
}

//http rest
int http_get(struct soap* soap)
{
	if (CheckAuthorization(soap))
	{//认证失败
		return 401;
	}

	soap_response(soap, SOAP_HTML);
	soap_send(soap, "<html>Hello world</html>");
	soap_end_send(soap);
	return SOAP_OK;
}