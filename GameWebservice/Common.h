#ifndef COMMON_H
#define COMMON_H

# include "pub.h"

//server
extern int gPort;
extern int gBackLog;
extern int gMaxThr;
extern int gMaxQueue;

extern char* gAuthUserId;
extern char* gAuthPwd;
//base 64 [AUTH_USERID:AUTH_PWD] > Authorization:Basic YWRtaW46MTIzNDU2

//db
extern char* gDbHostName;
extern char* gDbUserName;
extern char* gDbPassword;
extern int gDbMaxConnPool;

#endif
