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

//int转byte
void  IntToByte(INT32 i, byte *bytes)
{
	bytes[0] = (byte)(0xff & i);
	bytes[1] = (byte)((0xff00 & i) >> 8);
	bytes[2] = (byte)((0xff0000 & i) >> 16);
	bytes[3] = (byte)((0xff000000 & i) >> 24);
	return;
}

//byte转int
INT32 BytesToInt(byte* bytes)
{
	INT32 addr = bytes[0] & 0xFF;
	addr |= ((bytes[1] << 8) & 0xFF00);
	addr |= ((bytes[2] << 16) & 0xFF0000);
	addr |= ((bytes[3] << 24) & 0xFF000000);
	return addr;
}