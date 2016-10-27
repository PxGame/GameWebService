# include "ProcessHttpPost.h"
# include "pub.h"
# include "Common.h"

int HttpPostHandler(struct soap* soap)
{
	fprintf(stderr, "HttpPostHandler\n");
	if (CheckAuthorization(soap))
	{//ÈÏÖ¤Ê§°Ü
		return 401;
	}


	//char* buf;
	//size_t len;

	//if (soap_http_body(soap, &buf, &len) != SOAP_OK)
	//	return soap->error;

	//FILE* f = fopen("image.jpg", "wb+");
	//if (f == NULL)
	//{
	//	return 404;
	//}

	//int lens = fwrite(buf, 1, len, f);
	//fclose(f);

	soap_response(soap, SOAP_HTML);
	soap_send(soap, "<html>HttpPostHandler</html>");
	soap_end_send(soap);
	return SOAP_OK;
}