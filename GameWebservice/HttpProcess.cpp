#include "HttpProcess.h"
# include "pub.h"
# include "GSoapManager.h"
# include "DBManager.h"
# include "Common.h"

# include "json\json.h"

enum WebServiceType
{
	Regist,
	LoginFromPwd,
	LoginFromToken,
};

enum StatusCode
{
	Error,
	NoError,
	Sucesss,
	RequestDataFailed,
	JsonParseFailed,
	NameInvalid,
	PwdInvalid
};

//Post根据mime分配回调
struct http_post_handlers ghttpPostHndlers[] =
{
	{ "*/*", HttpPostHandler },
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

	//Connection* ss = DBManager::get()->GetConnection();
	//DBManager::get()->RegistUser(ss, "xiangmu", "123", "abc");
	//DBManager::get()->ReleaseConnection(ss);

	soap_response(soap, SOAP_HTML);
	soap_send(soap, "<html>HttpGetHandler</html>");
	soap_end_send(soap);
	return SOAP_OK;
}

int HttpPostHandler(struct soap* soap)
{
	fprintf(stderr, "HttpPostHandler\n");
	Json::Value ret;
	ret["status"] = StatusCode::NoError;
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
				ret["status"] = StatusCode::RequestDataFailed;
				break;
			}

			if (length < 1)
			{
				ret["status"] = StatusCode::RequestDataFailed;
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
		ret["error"] = e.what();
	}

	if (ret["status"] != StatusCode::Sucesss)
	{//clear
		StatusCode temp = (StatusCode)ret["status"].asInt();
		ret.clear();
		ret["status"] = temp;
	}

	string jsonData = ret.toStyledString();

	soap_response(soap, SOAP_HTML);
	soap_send(soap, jsonData.c_str());
	soap_end_send(soap);
	return SOAP_OK;
}

int HttpPost_Regist(struct soap* soap, char *data, int length)
{
	Json::Value ret;
	ret["status"] = StatusCode::NoError;
	try
	{
		do
		{
			++data;
			--length;

			Json::Reader reader;
			Json::Value root;

			if (!reader.parse(data, root))
			{
				ret["status"] = StatusCode::JsonParseFailed;
				break;
			}

			string name;
			string pwd;
			if (root["name"].isNull() || (name = root["name"].asString()).empty())
			{
				ret["status"] = StatusCode::NameInvalid;
				break;
			}
			if (root["pwd"].isNull() || (pwd = root["pwd"].asString()).empty())
			{
				ret["status"] = StatusCode::PwdInvalid;
				break;
			}

			Connection* conn = DBManager::get()->GetConnection();
			bool bRet = DBManager::get()->RegistUser(conn, name, pwd, TranslateIpToString(soap->ip));
			DBManager::get()->ReleaseConnection(conn);
			if (bRet)
			{
				ret["status"] = StatusCode::Sucesss;
			}
		} while (false);
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "HttpPost_Regist exception:%s\n", e.what());
		ret["status"] = StatusCode::Error;
		ret["error"] = e.what();
	}

	if (ret["status"] != StatusCode::Sucesss)
	{//clear
		StatusCode temp = (StatusCode)ret["status"].asInt();
		ret.clear();
		ret["status"] = temp;
	}

	string jsonData = ret.toStyledString();

	soap_response(soap, SOAP_HTML);
	soap_send(soap, jsonData.c_str());
	soap_end_send(soap);
	return SOAP_OK;
}
