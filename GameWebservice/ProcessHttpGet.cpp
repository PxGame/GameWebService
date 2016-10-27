# include "ProcessHttpGet.h"
# include "pub.h"
# include "Common.h"

int HttpGetHandler(struct soap* soap)
{
	fprintf(stderr, "HttpGetHandler\n");
	if (CheckAuthorization(soap))
	{//»œ÷§ ß∞‹
		return 401;
	}

	soap_response(soap, SOAP_HTML);
	soap_send(soap, "<html>HttpGetHandler</html>");
	soap_end_send(soap);
	return SOAP_OK;
}