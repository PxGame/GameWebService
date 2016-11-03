#ifndef PUB_H
#define PUB_H

# include <iostream>
# include <queue>
# include <vector>
# include <string>
# include <codecvt>

using namespace std;

//json
# include "json\json.h"

//boost
# include "boost\uuid\uuid.hpp"
# include "boost\uuid\uuid_generators.hpp"
# include "boost\uuid\uuid_io.hpp"
# include "boost\lexical_cast.hpp"

//gsoap
# include "soapH.h"
# include "threads.h"
# include "httpget.h"
# include "httppost.h"
# include "httpform.h"

//mysql
# include "mysql_connection.h"
# include "mysql_driver.h"
# include "cppconn\driver.h"
# include "cppconn\statement.h"
# include "cppconn\prepared_statement.h"
# include "cppconn\metadata.h"
# include "cppconn\exception.h"

using namespace sql;

#endif
