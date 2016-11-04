#include "HttpProcess.h"
# include "pub.h"
# include "GSoapManager.h"
# include "DBManager.h"
# include "Common.h"
# include "ToolFunc.h"

enum WebServiceType
{
	None,
	Regist,
	LoginFromPwd,
	LoginFromToken
};

enum StatusCode
{
	Sucesss,
	Error,
	Exception,

	RequestDataFailed,
	DBConnectFailed,
	PwdFailed,
	NameFailed,
	TokenFailed,
	JsonParseFailed,

	RegistUserError,
	LoginUpdateError,
};

typedef struct HttpData
{
	WebServiceType type;
	char* configData;
	UINT32 configLength;
	char* binaryData;
	UINT32 binaryLength;
	HttpData()
	{
		type = WebServiceType::None;
		configData = 0;
		configLength = 0;
		binaryData = 0;
		binaryLength = 0;
	}

	string GetConfigData()
	{
		return string(configData, configLength);
	}
}HTTPDATA, *PHTTPDATA;

int HttpGetHandler(struct soap* soap); 
int HttpPostHandler(struct soap* soap);
int HttpPost_Regist(struct soap* soap, HTTPDATA& httpData);
int HttpPost_LoginFromPwd(struct soap* soap, HTTPDATA& httpData);
int HttpPost_LoginFromToken(struct soap* soap, HTTPDATA& httpData);
void ParseData(char* data, size_t length, HTTPDATA& httpData);

//Post根据mime分配回调
struct http_post_handlers ghttpPostHndlers[] =
{
	{ "POST", HttpPostHandler },
	{ 0 }
};

int HttpGetHandler(struct soap* soap)
{
	fprintf(stderr, "HttpGetHandler\n");
	if (GSoapManager::get()->CheckAuthorization(soap))
	{//认证失败
		return 401;
	}

	soap_response(soap, SOAP_HTML);
	soap_send(soap, "<html>HttpGetHandler</html>");
	soap_end_send(soap);
	return SOAP_OK;
}

int HttpPostHandler(struct soap* soap)
{
	fprintf(stderr, "HttpPostHandler\n");
	Json::Value ret;
	StatusCode status = StatusCode::Error;
	try
	{
		do
		{
			if (GSoapManager::get()->CheckAuthorization(soap))
			{//认证失败
				return 401;
			}

			char *data;
			size_t length;

			if (soap_http_body(soap, &data, &length) != SOAP_OK)
			{
				status = StatusCode::RequestDataFailed;
				break;
			}

			if (length < 8)
			{
				status = StatusCode::RequestDataFailed;
				break;
			}

			//解析
			HTTPDATA httpData;
			ParseData(data, length, httpData);

			//
			switch (httpData.type)
			{
			case WebServiceType::Regist:
				return HttpPost_Regist(soap, httpData);
			case WebServiceType::LoginFromPwd:
				return HttpPost_LoginFromPwd(soap, httpData);
			case WebServiceType::LoginFromToken:
				return HttpPost_LoginFromToken(soap, httpData);
			}
		} while (false);
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "HttpPostHandler exception:%s\n", e.what());
		status = StatusCode::Exception;
	}

	if (status != StatusCode::Sucesss)
	{
		ret.clear();
	}

	ret["status"] = status;

	string jsonData = ret.toStyledString();

	soap_response(soap, SOAP_HTML);
	soap_send(soap, jsonData.c_str());
	soap_end_send(soap);
	return SOAP_OK;
}

int HttpPost_Regist(struct soap* soap, HTTPDATA& httpData)
{
	Json::Value ret;
	StatusCode status = StatusCode::Error;

	Connection* conn = 0;

	try
	{
		do
		{
			//获取数据库连接
			conn = DBManager::get()->GetConnection();
			if (conn == 0)
			{
				status = StatusCode::DBConnectFailed;
				break;
			}

			//============================================================================================
			Json::Reader reader;
			Json::Value root;

			if (!reader.parse(httpData.GetConfigData(), root))
			{
				status = StatusCode::JsonParseFailed;
				break;
			}

			string name;
			string pwd;
			if (root["name"].isNull() || (name = root["name"].asString()).empty())
			{
				status = StatusCode::NameFailed;
				break;
			}
			if (root["pwd"].isNull() || (pwd = root["pwd"].asString()).empty())
			{
				status = StatusCode::PwdFailed;
				break;
			}

			UserInfo userInfo;
			string ip = TranslateIpToString(soap->ip);
			string token;
			bool bRet = DBManager::get()->RegistUser(conn, name, pwd, ip);
			if (!bRet)
			{
				status = StatusCode::RegistUserError;
				break;
			}

			bRet = DBManager::get()->QueryUserInfo(conn, name, userInfo);
			if (!bRet)
			{
				status = StatusCode::LoginUpdateError;
				break;
			}

			token = GenericUserToken(userInfo);
			if (token.empty())
			{
				status = StatusCode::LoginUpdateError;
				break;
			}

			bRet = DBManager::get()->LoginUpdate(conn, name, ip, token);
			if (!bRet)
			{
				status = StatusCode::LoginUpdateError;
				break;
			}

			ret["token"] = token;

			//============================================================================================
			status = StatusCode::Sucesss;
		} while (false);
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "HttpPost_Regist exception:%s\n", e.what());
		ret["status"] = StatusCode::Exception;
	}

	DBManager::get()->ReleaseConnection(conn);
	
	if (status != StatusCode::Sucesss)
	{
		ret.clear();
	}

	ret["status"] = status;

	string jsonData = ret.toStyledString();

	soap_response(soap, SOAP_HTML);
	soap_send(soap, jsonData.c_str());
	soap_end_send(soap);
	return SOAP_OK;
}


int HttpPost_LoginFromPwd(struct soap* soap, HTTPDATA& httpData)
{
	Json::Value ret;
	StatusCode status = StatusCode::Error;

	Connection* conn = 0;

	try
	{
		do
		{
			//获取数据库连接
			conn = DBManager::get()->GetConnection();
			if (conn == 0)
			{
				status = StatusCode::DBConnectFailed;
				break;
			}

			//============================================================================================
			Json::Reader reader;
			Json::Value root;

			if (!reader.parse(httpData.GetConfigData(), root))
			{
				status = StatusCode::JsonParseFailed;
				break;
			}

			string name;
			string pwd;
			if (root["name"].isNull() || (name = root["name"].asString()).empty())
			{
				status = StatusCode::NameFailed;
				break;
			}
			if (root["pwd"].isNull() || (pwd = root["pwd"].asString()).empty())
			{
				status = StatusCode::PwdFailed;
				break;
			}

			UserInfo userInfo;
			string ip = TranslateIpToString(soap->ip);
			string token;
			bool bRet = DBManager::get()->QueryUserInfo(conn, name, userInfo);
			if (!bRet)
			{
				status = StatusCode::NameFailed;
				break;
			}

			//比较密码
			bRet = pwd.compare(userInfo.pwd) == 0;
			if (!bRet)
			{
				status = StatusCode::PwdFailed;
				break;
			}

			token = GenericUserToken(userInfo);
			if (token.empty())
			{
				status = StatusCode::LoginUpdateError;
				break;
			}

			bRet = DBManager::get()->LoginUpdate(conn, name, ip, token);
			if (!bRet)
			{
				status = StatusCode::LoginUpdateError;
				break;
			}

			ret["token"] = token;
			//============================================================================================
			status = StatusCode::Sucesss;
		} while (false);
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "HttpPost_LoginFromPwd exception:%s\n", e.what());
		ret["status"] = StatusCode::Exception;
	}

	DBManager::get()->ReleaseConnection(conn);

	if (status != StatusCode::Sucesss)
	{
		ret.clear();
	}

	ret["status"] = status;

	string jsonData = ret.toStyledString();

	soap_response(soap, SOAP_HTML);
	soap_send(soap, jsonData.c_str());
	soap_end_send(soap);
	return SOAP_OK;
}

int HttpPost_LoginFromToken(struct soap* soap, HTTPDATA& httpData)
{
	Json::Value ret;
	StatusCode status = StatusCode::Error;

	Connection* conn = 0;

	try
	{
		do
		{
			//获取数据库连接
			conn = DBManager::get()->GetConnection();
			if (conn == 0)
			{
				status = StatusCode::DBConnectFailed;
				break;
			}

			//============================================================================================
			Json::Reader reader;
			Json::Value root;

			if (!reader.parse(httpData.GetConfigData(), root))
			{
				status = StatusCode::JsonParseFailed;
				break;
			}

			string name;
			string token;
			if (root["name"].isNull() || (name = root["name"].asString()).empty())
			{
				status = StatusCode::NameFailed;
				break;
			}
			if (root["token"].isNull() || (token = root["token"].asString()).empty())
			{
				status = StatusCode::TokenFailed;
				break;
			}

			UserInfo userInfo;
			string ip = TranslateIpToString(soap->ip);
			bool bRet = DBManager::get()->QueryUserInfo(conn, name, userInfo);
			if (!bRet)
			{
				status = StatusCode::NameFailed;
				break;
			}

			//比较token
			bRet = token.compare(userInfo.logintoken) == 0;
			if (!bRet)
			{
				status = StatusCode::TokenFailed;
				break;
			}

			bRet = DBManager::get()->LoginUpdate(conn, name, ip, "");
			if (!bRet)
			{
				status = StatusCode::LoginUpdateError;
				break;
			}

			//============================================================================================
			status = StatusCode::Sucesss;
		} while (false);
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "HttpPost_LoginFromToken exception:%s\n", e.what());
		ret["status"] = StatusCode::Exception;
	}

	DBManager::get()->ReleaseConnection(conn);

	if (status != StatusCode::Sucesss)
	{
		ret.clear();
	}

	ret["status"] = status;

	string jsonData = ret.toStyledString();

	soap_response(soap, SOAP_HTML);
	soap_send(soap, jsonData.c_str());
	soap_end_send(soap);
	return SOAP_OK;
}

void ParseData(char* data, size_t length, HTTPDATA& httpData)
{
	//format data
	//type(4byte) | configlength(4byte) | configData(configlength) | binaryData(length - 8byte - configlength)

	//类型
	httpData.type = (WebServiceType)BytesToInt((byte*)data);
	data += 4;

	//配置信息
	httpData.configLength = BytesToInt((byte*)data);
	data += 4;
	httpData.configData = data;

	//剩余的二进制信息
	httpData.binaryLength = length - 8 - httpData.configLength;
	if (httpData.binaryLength > 0)
	{
		httpData.binaryData = data + httpData.configLength;
	}
}