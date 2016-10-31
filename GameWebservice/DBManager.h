#ifndef DBMANAGER_H
#define DBMANAGER_H

# include "pub.h"

using namespace sql;

typedef struct UserInfo
{
	string name;
	string pwd;
	string createtime;
	string lastlogintime;
	string lastloginip;
	string logintoken;
	int logincount;
}USERINFO,*PUSERINFO;

class DBManager
{
public:
	DBManager(const char* hostName, const char* userName, const char* password);
	~DBManager();

private:
	SQLString _hostName;
	SQLString _userName;
	SQLString _password;

	MUTEX_TYPE _newConnMutex;

public:
	Connection* GetNewConnection();
	bool QueryUserInfo(Connection* conn, const char* name, UserInfo& info);
	bool RegistUser(Connection* conn, const char* name, const char* pwd, const char* ip);
	bool LoginUpdate(Connection* conn, const char* name, const char* ip, const char* token);
};

#endif // !DBMANAGER_H

