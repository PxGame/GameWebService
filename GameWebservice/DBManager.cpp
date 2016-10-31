# include "DBManager.h"
# include "pub.h"

DBManager::DBManager(const char* hostName, const char* userName, const char* password)
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

bool DBManager::QueryUserInfo(Connection* conn, const char* name, UserInfo& info)
{
	PreparedStatement*  stmt = 0;
	ResultSet* res = 0;
	bool bRet = false;

	try
	{
		stmt = conn->prepareStatement("call user_query(?)");
		stmt->setString(1, name);
		if (stmt->execute())
		{//有返回值
			res = stmt->getResultSet();

			if (res->findColumn("_error") == 0 && res->next())
			{
				info.name = res->getString("name").c_str();
				info.pwd = res->getString("pwd").c_str();
				info.createtime = res->getString("createtime").c_str();
				info.lastlogintime = res->getString("lastlogintime").c_str();
				info.lastloginip = res->getString("lastloginip").c_str();
				info.logintoken = res->getString("logintoken").c_str();
				info.logincount = res->getInt("logincount");

				bRet = true;
			}
		}
	}
	catch (exception& e)
	{
		fprintf(stderr, "QueryUserInfo is exception:%s\n", e.what());
		bRet = false;
	}

	if (res != 0)
	{
		delete res;
	}

	if (stmt != 0)
	{
		while (stmt->getMoreResults())
		{
			delete stmt->getResultSet();
		}

		delete stmt;
	}
	return true;
}

bool DBManager::RegistUser(Connection* conn, const char* name, const char* pwd, const char* ip)
{
	PreparedStatement*  stmt = 0;
	ResultSet* res = 0;
	bool bRet = false;

	try
	{
		stmt = conn->prepareStatement("call user_regist(?,?,?)");
		stmt->setString(1, name);
		stmt->setString(2, pwd);
		stmt->setString(3, ip);
		if (stmt->execute())
		{//有返回值
			res = stmt->getResultSet();

			if (res->findColumn("_error") == 0 && res->next())
			{
				bRet = res->getBoolean("_ret");
			}
		}
	}
	catch (exception& e)
	{
		fprintf(stderr, "RegistUser is exception:%s\n", e.what());
		bRet = false;
	}

	if (res != 0)
	{
		delete res;
	}

	if (stmt != 0)
	{
		while (stmt->getMoreResults())
		{
			delete stmt->getResultSet();
		}

		delete stmt;
	}

	return bRet;
}

bool DBManager::LoginUpdate(Connection * conn, const char * name, const char * ip, const char * token)
{
	PreparedStatement*  stmt = 0;
	ResultSet* res = 0;
	bool bRet = false;

	try
	{
		stmt = conn->prepareStatement("call user_login_update(?,?,?)");
		stmt->setString(1, name);
		stmt->setString(2, ip);
		stmt->setString(3, token);
		if (stmt->execute())
		{//有返回值
			res = stmt->getResultSet();
			bRet = res->findColumn("_error") == 0;
		}
		else
		{
			bRet = true;
		}
	}
	catch (exception& e)
	{
		fprintf(stderr, "LoginUpdate is exception:%s\n", e.what());
		bRet = false;
	}

	if (res != 0)
	{
		delete res;
	}

	if (stmt != 0)
	{
		while (stmt->getMoreResults())
		{
			delete stmt->getResultSet();
		}

		delete stmt;
	}
	return bRet;
}
