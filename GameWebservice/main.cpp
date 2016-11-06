# include "pub.h"
# include "Common.h"
# include "DBManager.h"
# include "GSoapManager.h"
# include "GameWebservice.nsmap"//该文件不要重复包含，会产生重复定义

int main(int argc, char* argv[])
{

	try
	{
		GSoapManager::Create(gPort, gBackLog, gMaxThr, gMaxQueue, gAuthUserId, gAuthPwd);
		DBManager::Create(gDbHostName, gDbUserName, gDbPassword, gDbMaxConnPool);
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "GameWebservice Create exception:%s\n", e.what());
		return -1;
	}

	fprintf(stderr, "GameWebservice Launching...\n");

	GSoapManager::get()->RunOnThread();

	//char operation[64];
	//while (true)
	//{
	//	scanf("%s", operation);
	//	fflush(0);
	//	if (0 == strcmp(operation, "start"))
	//	{
	//		GSoapManager::get()->RunOnThread();
	//	}
	//	else if (0 == strcmp(operation, "stop"))
	//	{
	//		GSoapManager::get()->Close();
	//	}
	//	else if (0 == strcmp(operation, "quit"))
	//	{
	//		GSoapManager::get()->Close();
	//		break;
	//	}

	//	SLEEP(1);
	//}

	//等待主循环结束
	GSoapManager::get()->WaitThread();

	fprintf(stderr, "GameWebservice quit.\n");

	return 0;
}

//Rest don`t use this.
int ns__test(struct soap* soap)
{
	return SOAP_NO_METHOD;
}
