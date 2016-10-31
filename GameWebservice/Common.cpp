# include "Common.h"
# include "pub.h"

//server

//端口
int gPort = 8080;

//db
char* gDbHostName = "tcp://192.168.1.58:3306/game_db";
char* gDbUserName = "admin";
char* gDbPassword = "123456";

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