#ifndef COMMON_H
#define COMMON_H

# include "pub.h"


#if defined(WIN32)
# define FIXEDTHREADFUNC(x) ((void(*)(void*))(x))
# define SLEEP(x) Sleep(x)
#elif defined(_POSIX_THREADS) || defined(_SC_THREADS)
# define FIXEDTHREADFUNC(x) (x)
# define SLEEP(x) usleep(x*1000)
#endif

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

string TranslateIpToString(unsigned long ip);

#endif
