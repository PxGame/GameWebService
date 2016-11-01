#ifndef DBMANAGER_H
#define DBMANAGER_H

# include "pub.h"

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
private:
	DBManager() {}
	DBManager(
		string hostName,
		string userName,
		string password,
		int maxConn);
	~DBManager();

private:
	string _hostName;
	string _userName;
	string _password;
	int _maxPoolSize;

	Driver* _driver;

	queue<Connection*> _connPool;
	int _curPoolSize;

	MUTEX_TYPE _threadMutex;

	//pool
public:
	Connection* GetConnection();
	void ReleaseConnection(Connection* conn);
private:
	Connection* CreateConnection();
	void InitConnectionPool(int initSize);
	void DestoryConnection(Connection* conn);
	void DestoryPool();

private:
	static DBManager* _instance;

public:
	static void Create(
		const string& hostName,
		const string& userName,
		const string& password,
		int maxConn);
	static DBManager* get();

	//sql
	static bool QueryUserInfo(
		Connection* conn, 
		const string& name, 
		UserInfo& info);
	static bool RegistUser(
		Connection* conn, 
		const string& name, 
		const string& pwd, 
		const string& ip);
	static bool LoginUpdate(
		Connection* conn, 
		const string& name, 
		const string& ip, 
		const string& token);
};

#endif // !DBMANAGER_H

