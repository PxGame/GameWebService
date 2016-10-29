# include "DBManager.h"
# include "pub.h"

DBManager::DBManager(const sql::SQLString& hostName, const sql::SQLString& userName, const sql::SQLString& password)
{
	_hostName = hostName;
	_userName = userName;
	_password = password;

	MUTEX_SETUP(_newConnMutex);
}


DBManager::~DBManager()
{
	MUTEX_CLEANUP(_newConnMutex);
}

sql::Connection * DBManager::GetNewConnection()
{
	sql::Connection* conn = 0;
	MUTEX_LOCK(_newConnMutex);
	conn = sql::mysql::get_mysql_driver_instance()->connect(_hostName, _userName, _password);
	MUTEX_UNLOCK(_newConnMutex);
	return conn;
}
