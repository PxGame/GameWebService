# include "pub.h"
# include "Common.h"
# include "ProcessHttpForm.h"
# include "ProcessHttpGet.h"
# include "ProcessHttpPost.h"
# include "DBManager.h"
# include "GameWebservice.nsmap"//���ļ���Ҫ�ظ�������������ظ�����

//====== Globe parameter

static struct soap **gSoapthr;
static THREAD_TYPE *gThrIds;

//Socket����
static SOAP_SOCKET *gQueSocket;
//�̶߳���ͷ
static int gQueHeadSktIdx = 0;
//�̶߳���β
static int gQueTailSktIdx = 0;

//��ѭ���߳̾��
static THREAD_TYPE gServiceLoopThr = 0;

//�߳�ͬ��
static MUTEX_TYPE gQueMutex;
static COND_TYPE gQueCond;

//��soap
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

//Post����mime����ص�
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
			//������ѭ��
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

	//�ȴ���ѭ������
	THREAD_JOIN(gServiceLoopThr);
	delete gDbManager;
	fprintf(stderr, "GameWebservice quit.\n");

	return 0;
}

//��ʼ��ȫ�ֱ���
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
	//Socket����
	for (i = 0; i < gMaxQueue; i++)
	{
		gQueSocket[i] = 0;
	}


	//�̶߳���ͷ
	gQueHeadSktIdx = 0;
	//�̶߳���β
	gQueTailSktIdx = 0;
	//��ѭ���߳̾��
	gServiceLoopThr = 0;
}

//������ѭ��
void* ServiceLoopThread(void*)
{
	SOAP_SOCKET sMainSocket, sTemp;
	int i;

	fprintf(stderr, "ServiceLoopThread Launching...\n");
	InitGlobeArg();

	//��ʼ��
	soap_init(&gMainSoap);

	//�����˺�����
	gMainSoap.userid = gAuthUserId;
	gMainSoap.passwd = gAuthPwd;

	//��ӻص�
	soap_register_plugin_arg(&gMainSoap, http_post, (void*)ghttpPostHndlers);
	soap_register_plugin_arg(&gMainSoap, http_get, (void*)HttpGetHandler);
	soap_register_plugin_arg(&gMainSoap, http_form, (void*)HttpFormHandler);

	do
	{
		//�󶨶˿�
		sMainSocket = soap_bind(&gMainSoap, 0, gPort, gBackLog);
		if (!soap_valid_socket(sMainSocket))
		{
			break;
		}

		//����ͬ������
		MUTEX_SETUP(gQueMutex);
		COND_SETUP(gQueCond);

		//���������߳�
		for (i = 0; i < gMaxThr; i++)
		{
			gSoapthr[i] = soap_copy(&gMainSoap);//����soap
			THREAD_CREATE(&gThrIds[i], FIXEDTHREADFUNC(ProcessQueThread), (void*)gSoapthr[i]);//�����߳�
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
					continue;//����
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

		//�˳������߳�
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

		//����ͬ������
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

//��Ϣ���г����߳�
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

//�������
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

//��������
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
