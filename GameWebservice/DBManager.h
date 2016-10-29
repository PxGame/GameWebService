#ifndef DBMANAGER_H
#define DBMANAGER_H

# include "pub.h"

class DBManager
{
public:
	DBManager(const sql::SQLString& hostName, const sql::SQLString& userName, const sql::SQLString& password);
	~DBManager();

private:
	sql::SQLString _hostName;
	sql::SQLString _userName;
	sql::SQLString _password;

	MUTEX_TYPE _newConnMutex;

public:
	sql::Connection* GetNewConnection();
};

#endif // !DBMANAGER_H

