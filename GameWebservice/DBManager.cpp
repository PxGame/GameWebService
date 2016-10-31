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
	auto_ptr<PreparedStatement >  pstmt;
	auto_ptr<ResultSet > res;

	pstmt.reset(conn->prepareStatement("call user_query(?)"));
	pstmt->setString(1, name);
	if (!pstmt->execute())
	{//没有返回值
		return false;
	}
	res.reset(pstmt->getResultSet());
	if (res->findColumn("_error") != 0)
	{
		return false;
	}

	if (!res->next())
	{
		return false;	
	}

	info.name = res->getString("name").c_str();
	info.pwd = res->getString("pwd").c_str();
	info.createtime = res->getString("createtime").c_str();
	info.lastlogintime = res->getString("lastlogintime").c_str();
	info.lastloginip = res->getString("lastloginip").c_str();
	info.logintoken = res->getString("logintoken").c_str();
	info.logincount = res->getInt("logincount");

	while (res->next()) {}
	pstmt->close();

	return true;
}

bool DBManager::RegistUser(Connection* conn, const char* name, const char* pwd, const char* ip)
{
	auto_ptr<PreparedStatement >  pstmt;
	auto_ptr<ResultSet > res;

	pstmt.reset(conn->prepareStatement("call user_regist(?,?,?)"));
	pstmt->setString(1, name);
	pstmt->setString(2, pwd);
	pstmt->setString(3, ip);
	if (!pstmt->execute())
	{//没有返回值
		return true;
	}
	res.reset(pstmt->getResultSet());

	bool bRet = res->findColumn("_error") != 0;
	if (bRet)
	{
		bRet = res->getBoolean("_ret");
	}

	while (res->next()) {}
	pstmt->close();
	return bRet;
}

bool DBManager::LoginUpdate(Connection * conn, const char * name, const char * ip, const char * token)
{
	auto_ptr<PreparedStatement >  pstmt;
	auto_ptr<ResultSet > res;

	pstmt.reset(conn->prepareStatement("call user_login_update(?,?,?)"));
	pstmt->setString(1, name);
	pstmt->setString(2, ip);
	pstmt->setString(3, token);
	if (!pstmt->execute())
	{//没有返回值
		return true;
	}

	res.reset(pstmt->getResultSet());
	bool bRet = res->findColumn("_error") != 0;
	while (res->next()){}
	pstmt->close();
	return bRet;
}
