# include "Common.h"
# include "pub.h"

//�˿�
int gPort = 8080;

//��֤�û�
int CheckAuthorization(struct soap* soap)
{
	if (!soap->userid || !soap->passwd ||
		strcmp(soap->userid, AUTH_USERID) ||
		strcmp(soap->passwd, AUTH_PWD))
	{
		//��֤ʧ�� 401
		return 401;
	}

	return 0;
}