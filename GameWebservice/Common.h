#ifndef COMMON_H
#define COMMON_H

//====== configuration
# define BACKLOG (100)
# define MAX_THR (10)			//�̳߳ش�С
# define MAX_QUEUE (1000)		//������д�С
# define AUTH_USERID "admin"		//�û���
# define AUTH_PWD "123456"		//����
//base 64 [AUTH_USERID:AUTH_PWD] > Authorization:Basic YWRtaW46MTIzNDU2

extern int gPort;

int CheckAuthorization(struct soap*);

#endif
