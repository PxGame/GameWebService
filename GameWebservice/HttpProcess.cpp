#include "HttpProcess.h"
# include "pub.h"
# include "GSoapManager.h"
# include "DBManager.h"
# include "Common.h"
# include "ToolFunc.h"


enum WebServiceType
{
	Regist,
	LoginFromPwd,
	LoginFromToken,
};

enum StatusCode
{
	Exception,
	Error,
	NoError,
	Sucesss,
	RequestDataFailed,
	JsonParseFailed,
	NameInvalid,
	PwdInvalid,
	RegistFaild,
	QueryUserFailed,
	LoginUpdateFailed,
	DBConnectIsNull
};

//Post根据mime分配回调
struct http_post_handlers ghttpPostHndlers[] =
{
	{ "POST", HttpPostHandler },
	{ 0 }
};

int HttpPost_Regist(struct soap* soap, char *data, int length);

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
	StatusCode status = StatusCode::NoError;
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

			if (length < 1)
			{
				status = StatusCode::RequestDataFailed;
				break;
			}

			WebServiceType type = (WebServiceType)data[0];
			switch (type)
			{
			case WebServiceType::Regist:
				return HttpPost_Regist(soap, data, length);
			case WebServiceType::LoginFromPwd:
				break;
			case WebServiceType::LoginFromToken:
				break;
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

int HttpPost_Regist(struct soap* soap, char *data, int length)
{
	Json::Value ret;
	StatusCode status = StatusCode::NoError;

	Connection* conn = 0;

	try
	{
		do
		{
			conn = DBManager::get()->GetConnection();
			if (conn == 0)
			{
				status = StatusCode::DBConnectIsNull;
				break;
			}

			++data;
			--length;

			Json::Reader reader;
			Json::Value root;

			if (!reader.parse(data, root))
			{
				status = StatusCode::JsonParseFailed;
				break;
			}

			string name;
			string pwd;
			if (root["name"].isNull() || (name = root["name"].asString()).empty())
			{
				status = StatusCode::NameInvalid;
				break;
			}
			if (root["pwd"].isNull() || (pwd = root["pwd"].asString()).empty())
			{
				status = StatusCode::PwdInvalid;
				break;
			}

			UserInfo userInfo;
			string ip = TranslateIpToString(soap->ip);
			string token;
			bool bRet = DBManager::get()->RegistUser(conn, name, pwd, ip);
			if (!bRet)
			{
				status = StatusCode::RegistFaild;
				break;
			}

			bRet = DBManager::get()->QueryUserInfo(conn, name, userInfo);
			if (!bRet)
			{
				status = StatusCode::QueryUserFailed;
				break;
			}

			token = GenericUserToken(userInfo);

			bRet = DBManager::get()->LoginUpdate(conn, name, ip, token);
			if (!bRet)
			{
				status = StatusCode::LoginUpdateFailed;
				break;
			}

			ret["token"] = token;

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
