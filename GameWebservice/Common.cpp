# include "Common.h"
# include "pub.h"

//server

//端口
int gPort = 8080;
int gBackLog = 100;
int gMaxThr = 10;
int gMaxQueue = 1000;
char* gAuthUserId = "admin";
char* gAuthPwd = "123456";

//db
char* gDbHostName = "tcp://192.168.2.115:3306/game_db";
char* gDbUserName = "admin";
char* gDbPassword = "123456";

//认证用户
int CheckAuthorization(struct soap* soap)
{
	if (!soap->userid || !soap->passwd ||
		strcmp(soap->userid, gAuthUserId) ||
		strcmp(soap->passwd, gAuthPwd))
	{
		//认证失败 401
		return 401;
	}

	return 0;
}

string TranslateIpToString(unsigned long ip)
{
	char chIp[64] = {0};
	sprintf(chIp, "%d.%d.%d.%d", (int)(ip >> 24) & 0xFF, (int)(ip >> 16) & 0xFF, (int)(ip >> 8) & 0xFF, (int)ip & 0xFF);
	return string(chIp);
}
