# include "ProcessHttpPost.h"
# include "pub.h"
# include "Common.h"

int HttpPostHandler(struct soap* soap)
{
	if (CheckAuthorization(soap))
	{//»œ÷§ ß∞‹
		return 401;
	}

	return SOAP_OK;
}