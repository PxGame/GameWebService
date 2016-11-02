# ifndef HTTPPROCESS_H
# define HTTPPROCESS_H

//Post根据mime分配回调
extern struct http_post_handlers ghttpPostHndlers[];

int HttpGetHandler(struct soap* soap);

int HttpPostHandler(struct soap* soap);

# endif
