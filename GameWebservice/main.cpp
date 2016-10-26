# include <iostream>
# include "soapH.h"
# include "threads.h"
# include "httpget.h"
# include "httppost.h"
# include "httpform.h"
# include "GameWebservice.nsmap"
using namespace std;

//Rest don`t use this.
int ns__test(struct soap* soap)
{
	return SOAP_NO_METHOD;
}

//====== configuration
# define BACKLOG (100)
# define MAX_THR (10)			//�̳߳ش�С
# define MAX_QUEUE (1000)		//������д�С
# define AUTH_USERID "admin"		//�û���
# define AUTH_PWD "123456"		//����

//�˿�
int gPort = 8080;

//====== Globe parameter

struct soap *gSoapthr[MAX_THR] = { NULL };
THREAD_TYPE gThrIds[MAX_THR] = { NULL };

//Socket����
SOAP_SOCKET gQueSocket[MAX_QUEUE] = {0};
//�̶߳���ͷ
int gQueHeadSktIdx = 0;
//�̶߳���β
int gQueTailSktIdx = 0;

//��ѭ���߳̾��
THREAD_TYPE gServiceLoop = 0;

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
int CheckAuthorization(struct soap*);

int HttpGetHandler(struct soap* soap);
int HttpFormHandler(struct soap* soap);
int HttpPostHandler(struct soap* soap);

//===

int main(int argc, char* argv[])
{
	char operation[64];
	fprintf(stderr, "GameWebservice Launching...\n", THREAD_ID);
	
	while (true)
	{
		scanf("%s", operation);
		fflush(NULL);
		if (0 == strcmp(operation, "start"))
		{
			//������ѭ��
			THREAD_CREATE(&gServiceLoop, (void(*)(void*))ServiceLoopThread, NULL);
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

		Sleep(1);
	}

	//�ȴ���ѭ������
	THREAD_JOIN(gServiceLoop);

	fprintf(stderr, "GameWebservice quit.\n");
	return 0;
}

//��ʼ��ȫ�ֱ���
void InitGlobeArg()
{
	int i = 0;

	for (i = 0; i < MAX_THR; i++)
	{
		gSoapthr[i] = NULL;
		gThrIds[i] = NULL;
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
	gServiceLoop = 0;
}

//������ѭ��
void* ServiceLoopThread(void*)
{
	SOAP_SOCKET m, s;
	int i;

	fprintf(stderr, "ServiceLoopThread %d Launching...\n", THREAD_ID);
	InitGlobeArg();

	//��ʼ��
	soap_init(&gMainSoap);

	//�����˺�����
	gMainSoap.userid = AUTH_USERID;
	gMainSoap.passwd = AUTH_PWD;

	//��ӻص�
	soap_register_plugin_arg(&gMainSoap, http_get, HttpGetHandler);
	soap_register_plugin_arg(&gMainSoap, http_post, HttpPostHandler);
	soap_register_plugin_arg(&gMainSoap, http_form, HttpFormHandler);

	do
	{
		//�󶨶˿�
		m = soap_bind(&gMainSoap, NULL, gPort, BACKLOG);
		if (!soap_valid_socket(m))
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
			THREAD_CREATE(&gThrIds[i], (void(*)(void*))ProcessQueThread, (void*)gSoapthr[i]);//�����߳�
		}

		//main loop
		for (;;)
		{
			s = soap_accept(&gMainSoap);
			if (!soap_valid_socket(s))
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

			fprintf(stderr, "Thread %d accepts socket %d connection from IP %d.%d.%d.%d\n", (DWORD)gThrIds[i], s, (gMainSoap.ip >> 24) & 0xFF, (gMainSoap.ip >> 16) & 0xFF, (gMainSoap.ip >> 8) & 0xFF, gMainSoap.ip & 0xFF);

			while (EnqueueSocket(s) == SOAP_EOM)
			{
				Sleep(1);
			}
		}

		//�˳������߳�
		for (i = 0; i < MAX_THR; i++)
		{
			while (EnqueueSocket(SOAP_INVALID_SOCKET) == SOAP_EOM)
			{
				Sleep(1);
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
	return NULL;
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
	return NULL;
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


//��֤�û�
int CheckAuthorization(struct soap* soap)
{
	if (!soap->userid || !soap->passwd ||
		strcmp(soap->userid, AUTH_USERID) ||
		strcmp(soap->passwd, AUTH_PWD))
	{
		//��֤ʧ�� 401
		return 401;
	}

	return 0;
}

//http rest
int HttpGetHandler(struct soap* soap)
{
	if (CheckAuthorization(soap))
	{//��֤ʧ��
		return 401;
	}

	soap_response(soap, SOAP_HTML);
	soap_send(soap, "<html>Hello world</html>");
	soap_end_send(soap);
	return SOAP_OK;
}

int HttpPostHandler(struct soap* soap)
{
	if (CheckAuthorization(soap))
	{//��֤ʧ��
		return 401;
	}

	return SOAP_OK;
}

int HttpFormHandler(struct soap* soap)
{
	if (CheckAuthorization(soap))
	{//��֤ʧ��
		return 401;
	}



	return SOAP_OK;
}