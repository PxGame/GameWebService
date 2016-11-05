# ifndef TOOLFUNC_H
# define TOOLFUNC_H

# include "pub.h"
# include "DBManager.h"

#if defined(WIN32)
# define FIXEDTHREADFUNC(x) ((void(*)(void*))(x))
# define SLEEP(x) Sleep(x)
#elif defined(_POSIX_THREADS) || defined(_SC_THREADS)
# define FIXEDTHREADFUNC(x) (x)
# define SLEEP(x) usleep(x*1000)
#endif

string TranslateIpToString(unsigned long ip);

string GenericNameUUID(string data);

string GenericRandomUUID();

string GenericUserToken(UserInfo& userInfo);

string WStringToUtf8(const wstring& str);

//int转char[4]
void  IntToByte(int32_t i, char *bytes);

//char[4]转int
int32_t BytesToInt(char* bytes);

# endif