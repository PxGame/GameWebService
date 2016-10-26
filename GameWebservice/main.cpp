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
	
	//������ѭ��
	THREAD_CREATE(&gServiceLoop, (void(*)(void*))ServiceLoopThread, NULL);

	//�ȴ���ѭ������
	THREAD_JOIN(gServiceLoop);
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
	struct soap soap;
	SOAP_SOCKET m, s;
	int i;

	cout << "ServiceLoop Launching..." << endl;
	InitGlobeArg();

	//��ʼ��
	soap_init(&soap);

	//�����˺�����
	soap.userid = AUTH_USERID;
	soap.passwd = AUTH_PWD;

	//��ӻص�
	soap.fget = http_get;

	do
	{
		//�󶨶˿�
		m = soap_bind(&soap, NULL, gPort, BACKLOG);
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
			gSoapthr[i] = soap_copy(&soap);//����soap
			THREAD_CREATE(&gThrIds[i], (void(*)(void*))ProcessQueThread, (void*)gSoapthr[i]);//�����߳�
		}

		for (;;)
		{
			s = soap_accept(&soap);
			if (!soap_valid_socket(s))
			{
				if (soap.errnum)
				{
					soap_print_fault(&soap, stderr);
					continue;//����
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
			fprintf(stderr, "Waiting for thread %d to terminate...\n", i);
			THREAD_JOIN(gThrIds[i]);
			fprintf(stderr, "terminated\n");

			soap_done(gSoapthr[i]);
			free(gSoapthr[i]);
		}

		//����ͬ������
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

//������ѭ���˳�
void ServiceLoopThreadExit()
{//������һЩ�˳�����
	fprintf(stderr, "ServiceLoopThread Exit.\n");

	DWORD thrId = THREAD_ID;
}

//��Ϣ���г����߳�
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

//�����߳��˳�
void ProcessQueThreadExit()
{
	fprintf(stderr, "ProcessQueThread Exit.\n");

	DWORD thrId = THREAD_ID;
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
int http_get(struct soap* soap)
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