# ifndef HTTPPROCESS_H
# define HTTPPROCESS_H

//Post����mime����ص�
extern struct http_post_handlers ghttpPostHndlers[];

int HttpGetHandler(struct soap* soap);

int HttpPostHandler(struct soap* soap);

# endif
