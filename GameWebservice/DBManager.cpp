# include "DBManager.h"
# include "pub.h"

DBManager::DBManager(
	string hostName,
	string userName,
	string password,
	int maxConn)
{
	_hostName = hostName;
	_userName = userName;
	_password = password;
	_maxPoolSize = maxConn;

	_driver = get_driver_instance();

	_curPoolSize = 0;

	MUTEX_SETUP(_threadMutex);

	//初始化连接池
	InitConnectionPool(_maxPoolSize / 2);
}

DBManager::~DBManager()
{
	MUTEX_CLEANUP(_threadMutex);
}

Connection * DBManager::GetConnection()
{
	Connection* conn = 0;
	MUTEX_LOCK(_threadMutex);

	if (_connPool.size() > 0)
	{//连接池还有可用连接
		conn = _connPool.front();
		_connPool.pop();

		if (conn->isClosed())
		{//连接无效
			delete conn;
			conn = CreateConnection();
		}
	}
	else
	{
		if (_curPoolSize < _maxPoolSize)
		{//还可以创建新的连接
			conn = CreateConnection();
			if (0 != conn)
			{
				++_curPoolSize;
			}
		}
	}
	
	MUTEX_UNLOCK(_threadMutex);
	return conn;
}

void DBManager::ReleaseConnection(Connection * conn)
{
	if (0 != conn)
	{
		MUTEX_LOCK(_threadMutex);

		_connPool.push(conn);

		MUTEX_UNLOCK(_threadMutex);
	}
}

Connection * DBManager::CreateConnection()
{
	Connection* conn = 0;

	try
	{
		conn = _driver->connect(_hostName.c_str(), _userName.c_str(), _password.c_str());
	}
	catch (const std::exception& e)
	{
		conn = 0;
		fprintf(stderr, "DBManager::CreateConnection exception:%s\n", e.what());
	}

	return conn;
}

void DBManager::InitConnectionPool(int initSize)
{
	Connection* conn = 0;

	//首先销毁原有连接
	DestoryPool();

	MUTEX_LOCK(_threadMutex);

	for (int i = 0; i < initSize; i++)
	{
		conn = CreateConnection();
		if (0 != conn)
		{
			_connPool.push(conn);
			++_curPoolSize;
		}
		else
		{
			fprintf(stderr, "DBManager::InitConnectionPool error.\n");
		}
	}

	MUTEX_UNLOCK(_threadMutex);
}

void DBManager::DestoryConnection(Connection * conn)
{
	if (0 != conn)
	{
		try
		{
			conn->close();
		}
		catch (const std::exception& e)
		{
			fprintf(stderr, "DBManager::DestoryConnection exception:%s\n", e.what());
		}
		delete conn;
	}
}

void DBManager::DestoryPool()
{
	MUTEX_LOCK(_threadMutex);

	Connection* conn;
	while (_connPool.size() > 0)
	{
		conn = _connPool.front();
		_connPool.pop();

		DestoryConnection(conn);
	}

	_curPoolSize = 0;

	MUTEX_UNLOCK(_threadMutex);
}

//===================================================================
DBManager* DBManager::_instance = 0;

void DBManager::Create(
	const string& hostName, 
	const string& userName, 
	const string& password,
	int maxConn)
{
	_instance = new DBManager(
		hostName,
		userName,
		password,
		maxConn);
}

DBManager * DBManager::get()
{
	return _instance;
}

bool DBManager::QueryUserInfo(
	Connection* conn, 
	const string& name, 
	UserInfo& info)
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

bool DBManager::RegistUser(
	Connection* conn,
	const string& name,
	const string& pwd,
	const string& ip)
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

bool DBManager::LoginUpdate(
	Connection * conn, 
	const string& name, 
	const string& ip, 
	const string& token)
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
