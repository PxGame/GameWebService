# include "ProcessHttpForm.h"
# include "pub.h"
# include "Common.h"

int HttpFormHandler(struct soap* soap)
{
	fprintf(stderr, "HttpFormHandler\n");
	if (CheckAuthorization(soap))
	{//»œ÷§ ß∞‹
		return 401;
	}
	
	soap_response(soap, SOAP_HTML);
	soap_send(soap, "<html>HttpFormHandler</html>");
	soap_end_send(soap);
	return SOAP_OK;
}