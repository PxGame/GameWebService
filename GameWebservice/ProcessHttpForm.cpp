# include "ProcessHttpForm.h"
# include "pub.h"
# include "Common.h"

int HttpFormHandler(struct soap* soap)
{
	if (CheckAuthorization(soap))
	{//��֤ʧ��
		return 401;
	}

	return SOAP_OK;
}