# include "pub.h"
# include "Common.h"
# include "ProcessHttpForm.h"
# include "ProcessHttpGet.h"
# include "ProcessHttpPost.h"

# include "GameWebservice.nsmap"//���ļ���Ҫ�ظ�������������ظ�����

//====== Globe parameter

struct soap *gSoapthr[MAX_THR] = { 0 };
THREAD_TYPE gThrIds[MAX_THR] = { 0 };

//Socket����
SOAP_SOCKET gQueSocket[MAX_QUEUE] = {0};
//�̶߳���ͷ
int gQueHeadSktIdx = 0;
//�̶߳���β
int gQueTailSktIdx = 0;

//��ѭ���߳̾��
THREAD_TYPE gServiceLoopThr = 0;

//�߳�ͬ��
MUTEX_TYPE gQueMutex;
COND_TYPE gQueCond;

//��soap
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
			//������ѭ��
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

	//�ȴ���ѭ������
	THREAD_JOIN(gServiceLoopThr);

	fprintf(stderr, "GameWebservice %ld quit.\n", THREAD_ID);
	return 0;
}

//��ʼ��ȫ�ֱ���
void InitGlobeArg()
{
	int i = 0;

	for (i = 0; i < MAX_THR; i++)
	{
		gSoapthr[i] = 0;
		gThrIds[i] = 0;
	}

	//Socket����
	for (i = 0; i < MAX_QUEUE; i++)
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

	fprintf(stderr, "ServiceLoopThread %d Launching...\n", THREAD_ID);
	InitGlobeArg();

	//��ʼ��
	soap_init(&gMainSoap);

	//�����˺�����
	gMainSoap.userid = AUTH_USERID;
	gMainSoap.passwd = AUTH_PWD;

	//��ӻص�
	soap_register_plugin_arg(&gMainSoap, http_post, (void*)ghttpPostHndlers);
	soap_register_plugin_arg(&gMainSoap, http_get, (void*)HttpGetHandler);
	soap_register_plugin_arg(&gMainSoap, http_form, (void*)HttpFormHandler);

	do
	{
		//�󶨶˿�
		sMainSocket = soap_bind(&gMainSoap, 0, gPort, BACKLOG);
		if (!soap_valid_socket(sMainSocket))
		{
			break;
		}

		//����ͬ������
		MUTEX_SETUP(gQueMutex);
		COND_SETUP(gQueCond);

		//���������߳�
		for (i = 0; i < MAX_THR; i++)
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

			fprintf(stderr, "Thread %d accepts socket %d connection from IP %d.%d.%d.%d\n", 
				(int)gThrIds[i], (int)sTemp, (int)(gMainSoap.ip >> 24) & 0xFF, (int)(gMainSoap.ip >> 16) & 0xFF, (int)(gMainSoap.ip >> 8) & 0xFF, (int)gMainSoap.ip & 0xFF);

			while (EnqueueSocket(sTemp) == SOAP_EOM)
			{
				SLEEP(1);
			}
		}

		//�˳������߳�
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

		//����ͬ������
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

//��Ϣ���г����߳�
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

//�������
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
