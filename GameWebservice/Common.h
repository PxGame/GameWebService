#ifndef COMMON_H
#define COMMON_H


#if defined(WIN32)
# define FIXEDTHREADFUNC(x) ((void(*)(void*))(x))
# define SLEEP(x) Sleep(x)
#elif defined(_POSIX_THREADS) || defined(_SC_THREADS)
# define FIXEDTHREADFUNC(x) (x)
# define SLEEP(x) usleep(x*1000)
#endif


//====== configuration
# define BACKLOG (100)
# define MAX_THR (10)			//线程池大小
# define MAX_QUEUE (1000)		//请求队列大小
# define AUTH_USERID "admin"		//用户名
# define AUTH_PWD "123456"		//密码
//base 64 [AUTH_USERID:AUTH_PWD] > Authorization:Basic YWRtaW46MTIzNDU2

extern int gPort;

int CheckAuthorization(struct soap*);

#endif
