# include "Common.h"
# include "pub.h"

//server

//�˿�
int gPort = 8080;

//db
char* gDbHostName = "tcp://192.168.1.58:3306/game_db";
char* gDbUserName = "admin";
char* gDbPassword = "123456";

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