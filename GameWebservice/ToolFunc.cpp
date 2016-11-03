# include "ToolFunc.h"
# include "pub.h"

string TranslateIpToString(unsigned long ip)
{
	char chIp[64] = { 0 };
	sprintf(chIp, "%d.%d.%d.%d", (int)(ip >> 24) & 0xFF, (int)(ip >> 16) & 0xFF, (int)(ip >> 8) & 0xFF, (int)ip & 0xFF);
	return string(chIp);
}

string GenericRandomUUID()
{
	static boost::uuids::random_generator gen;
	string u = boost::lexical_cast<string>(gen()); 
	string::iterator new_end = remove_if(u.begin(), u.end(), bind2nd(equal_to<char>(), '-'));
	u.erase(new_end, u.end());
	return u;
}
string GenericNameUUID(string data)
{
	static boost::uuids::name_generator gen(boost::uuids::nil_uuid());
	string u = boost::lexical_cast<string>(gen(data));
	string::iterator new_end = remove_if(u.begin(), u.end(), bind2nd(equal_to<char>(), '-'));
	u.erase(new_end, u.end());
	return u;
}

string GenericUserToken(UserInfo & userInfo)
{
	ostringstream o;
	o << userInfo.name << userInfo.pwd << userInfo.logincount;
	return GenericNameUUID(o.str());
}

string WStringToUtf8(const wstring & str)
{
	wstring_convert<codecvt_utf8<wchar_t>> conv;
	return conv.to_bytes(str);
}
