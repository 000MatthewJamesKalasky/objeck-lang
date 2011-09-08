#include "odbc.h"

using namespace std;

extern "C" {
  //
  // initialize odbc environment
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void load_lib() {
    if(!env) {
      SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
      SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    }
  }
  
  //
  // release odbc resources
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void unload_lib() {
    if(env) {
      SQLFreeHandle(SQL_HANDLE_ENV, env);
    }
  }
  
  //
  // connects to an ODBC data source
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_connect(VMContext& context) {
    SQLHDBC conn;

    const char* ds = APITools_GetStringValue(context, 1);
    const char* username = APITools_GetStringValue(context, 2);
    const char* password = APITools_GetStringValue(context, 3);
    
#ifdef _DEBUG
    cout << "### connect: " << "ds=" << ds << ", username=" 
	 << username << ", password=" << password << " ###" << endl;
#endif

    SQLRETURN status = SQLAllocHandle(SQL_HANDLE_DBC, env, &conn);
    if(SQL_OK) {
      status = SQLConnect(conn, (SQLCHAR*)ds, SQL_NTS, 
			  (SQLCHAR*)username, SQL_NTS, 
			  (SQLCHAR*)password, SQL_NTS);      
      if(SQL_FAIL) {	
	conn = NULL;
      }
    }
    else {
      conn = NULL;
    }
    
    APITools_SetIntValue(context, 0, (long)conn);
  }
  
  //
  // disconnects from an ODBC data source
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_disconnect(VMContext& context) {
    SQLHDBC conn = (SQLHDBC)APITools_GetIntValue(context, 0);    
    if(conn) {
      SQLDisconnect(conn);
      SQLFreeHandle(SQL_HANDLE_DBC, conn);
      
#ifdef _DEBUG
      cout << "### disconnect ###" << endl;
#endif
    }
  }
  
  //
  // executes and update statement
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_update_statement(VMContext& context) {
    SQLHDBC conn = (SQLHDBC)APITools_GetIntValue(context, 1);
    const char* sql = APITools_GetStringValue(context, 2);

#ifdef _DEBUG
    cout << "### update: conn=" << conn << ", stmt=" << sql << "  ###" << endl;
#endif    
    
    if(!conn || !sql) {
      APITools_SetIntValue(context, 0, -1);
      return;
    }
    
    SQLHSTMT stmt = NULL;
    SQLRETURN status = SQLAllocStmt(conn, &stmt);
    if(SQL_OK) {
      status = SQLExecDirect(stmt, (SQLCHAR*)sql, SQL_NTS);
      if(SQL_OK) {
	SQLLEN count;
	status = SQLRowCount(stmt, &count);
	if(SQL_OK) {
	  SQLFreeStmt(stmt, SQL_CLOSE);
	  APITools_SetIntValue(context, 0, count);
	  return;
	}
      }
    }
    
    if(stmt) {
      SQLFreeStmt(stmt, SQL_CLOSE);
    }
    APITools_SetIntValue(context, 0, -1);
  }

  //
  // executes a select statement
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_select_statement(VMContext& context) {
    SQLHDBC conn = (SQLHDBC)APITools_GetIntValue(context, 2);
    const char* sql = APITools_GetStringValue(context, 3);
    
#ifdef _DEBUG
    cout << "### select: conn=" << conn << ", stmt=" << sql << "  ###" << endl;
#endif    
    
    if(!conn || !sql) {
      APITools_SetIntValue(context, 0, 0);
      return;
    }
    
    SQLHSTMT stmt = NULL;
    SQLRETURN status = SQLAllocStmt(conn, &stmt);
    if(SQL_OK) {
      status = SQLPrepare(stmt, (SQLCHAR*)sql, SQL_NTS);
      if(SQL_OK) {
	SQLSMALLINT columns;
	status = SQLNumResultCols(stmt, &columns);
	if(SQL_OK) {
	  vector<ColumnDescription> descriptions;
	  // get column information
	  vector<const char*>* column_names = new vector<const char*>;
	  if(columns > 0) {
	    for(SQLSMALLINT i = 1; i <= columns; i++) {
	      ColumnDescription description;
	      status = SQLDescribeCol(stmt, i, (SQLCHAR*)&description.column_name, COL_NAME_MAX, 
				      &description.column_name_size, &description.type, 
				      &description.column_size, &description.decimal_length, 
				      &description.nullable);
	      if(SQL_FAIL) {
		SQLFreeStmt(stmt, SQL_CLOSE);
		APITools_SetIntValue(context, 0, 0);
		return;
	      }
	      column_names->push_back((const char*)description.column_name);
#ifdef _DEBUG
	      cout << "  name=" << description.column_name << ", type=" << description.type << endl;
#endif
	    }
	  }
	  // execute query
	  status = SQLExecute(stmt); 
	  if(SQL_OK) {
	    APITools_SetIntValue(context, 0, (long)stmt);
	    APITools_SetIntValue(context, 1, (long)column_names);
#ifdef _DEBUG
	    cout << "### select OK: stmt=" << stmt << " ###" << endl;
#endif  
	    return;
	  }
	} 
      }
    }
      
    if(stmt) {
      SQLFreeStmt(stmt, SQL_CLOSE);
    }      
    APITools_SetIntValue(context, 0, 0);
  }

  //
  // executes a select statement
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_pepare_statement(VMContext& context) {
    SQLHDBC conn = (SQLHDBC)APITools_GetIntValue(context, 2);
    const char* sql = APITools_GetStringValue(context, 3);
    
#ifdef _DEBUG
    cout << "### select: conn=" << conn << ", stmt=" << sql << "  ###" << endl;
#endif    
    
    if(!conn || !sql) {
      APITools_SetIntValue(context, 0, 0);
      return;
    }
    
    SQLHSTMT stmt = NULL;
    SQLRETURN status = SQLAllocStmt(conn, &stmt);
    if(SQL_OK) {
      status = SQLPrepare(stmt, (SQLCHAR*)sql, SQL_NTS);
      if(SQL_OK) {
	SQLSMALLINT columns;
	status = SQLNumResultCols(stmt, &columns);
	if(SQL_OK) {
	  vector<ColumnDescription> descriptions;
	  // get column information
	  vector<const char*>* column_names = new vector<const char*>;
	  if(columns > 0) {
	    for(SQLSMALLINT i = 1; i <= columns; i++) {
	      ColumnDescription description;
	      status = SQLDescribeCol(stmt, i, (SQLCHAR*)&description.column_name, COL_NAME_MAX, 
				      &description.column_name_size, &description.type, 
				      &description.column_size, &description.decimal_length, 
				      &description.nullable);
	      if(SQL_FAIL) {
		SQLFreeStmt(stmt, SQL_CLOSE);
		APITools_SetIntValue(context, 0, 0);
		return;
	      }
	      column_names->push_back((const char*)description.column_name);
#ifdef _DEBUG
	      cout << "  name=" << description.column_name << ", type=" << description.type << endl;
#endif
	    }
	  }
	  // return statement
	  APITools_SetIntValue(context, 0, (long)stmt);
	  APITools_SetIntValue(context, 1, (long)column_names);
	  return;
	} 
      }
    }
      
    if(stmt) {
      SQLFreeStmt(stmt, SQL_CLOSE);
    }      
    APITools_SetIntValue(context, 0, 0);
  }
  
  //
  // fetches the next row in a resultset
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_result_next(VMContext& context) {
    SQLHSTMT stmt = (SQLHDBC)APITools_GetIntValue(context, 1);
    if(!stmt) {
      APITools_SetIntValue(context, 0, 0);
      return;
    }
      
    SQLRETURN status = SQLFetch(stmt);
    if(SQL_OK) {
      APITools_SetIntValue(context, 0, 1);
      return;
    }

    APITools_SetIntValue(context, 0, 0);
  }

  //
  // updates a prepared statement
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_stmt_update(VMContext& context) {
    SQLHSTMT stmt = (SQLHDBC)APITools_GetIntValue(context, 1);

#ifdef _DEBUG
    cout << "### stmt_update: stmt=" << stmt << " ###" << endl;
#endif
    
    SQLRETURN status = SQLExecute(stmt);
    if(SQL_OK) {
      SQLLEN count;
      status = SQLRowCount(stmt, &count);
      if(SQL_OK) {
	APITools_SetIntValue(context, 0, count);
	return;
      }
    }
    
    APITools_SetIntValue(context, 0, -1);
  }
  
  //
  // set an int from a prepared statement
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_stmt_set_int(VMContext& context) {
    long* value = APITools_GetIntAddress(context, 1);
    long i = APITools_GetIntValue(context, 2);
    SQLHSTMT stmt = (SQLHDBC)APITools_GetIntValue(context, 3);
    
#ifdef _DEBUG
    cout << "### set_int: stmt=" << stmt << ", column=" << i 
	 << ", value=" << *value << " ###" << endl;
#endif  
    
    SQLRETURN status = SQLBindParameter(stmt, i, SQL_PARAM_INPUT, SQL_C_LONG, 
					SQL_INTEGER, 0, 0, value, 0, NULL);
    if(SQL_OK) { 
      APITools_SetIntValue(context, 0, 1);
      return;
    }
    
    APITools_SetIntValue(context, 0, 0);
  }
  
  //
  // gets an int from a result set
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_result_get_int(VMContext& context) {
    SQLUSMALLINT i = APITools_GetIntValue(context, 2);
    SQLHSTMT stmt = (SQLHDBC)APITools_GetIntValue(context, 3);
    vector<const char*>* names = (vector<const char*>*)APITools_GetIntValue(context, 4);
    
#ifdef _DEBUG
    cout << "### get_int: stmt=" << stmt << ", column=" << i << ", max=" << (long)names->size() << " ###" << endl;
#endif  
    
    if(!stmt || !names || i < 1 || i > (long)names->size()) {
      APITools_SetIntValue(context, 0, 0);
      APITools_SetIntValue(context, 1, 0);
      return;
    }
    
    SQLLEN is_null;
    int value;
    SQLRETURN status = SQLGetData(stmt, i, SQL_C_LONG, &value, 0, &is_null);
    if(SQL_OK) {
      APITools_SetIntValue(context, 0, is_null == SQL_NULL_DATA);
      APITools_SetIntValue(context, 1, value);
#ifdef _DEBUG
      cout << "  " << value << endl;
#endif
      return;
    }
    
    APITools_SetIntValue(context, 0, 0);
    APITools_SetIntValue(context, 1, 0);
  }
  
  //
  // gets an int from a result set
  //
#ifdef _WIN32
  __declspec(dllexport)  
#endif
  void odbc_result_get_double(VMContext& context) {
    SQLUSMALLINT i = APITools_GetIntValue(context, 2);
    SQLHSTMT stmt = (SQLHDBC)APITools_GetIntValue(context, 3);
    vector<const char*>* names = (vector<const char*>*)APITools_GetIntValue(context, 4);
    
#ifdef _DEBUG
    cout << "### get_double: stmt=" << stmt << ", column=" << i << ", max=" 
	 << (long)names->size() << " ###" << endl;
#endif  
    
    if(!stmt || !names || i < 1 || i > (long)names->size()) {
      APITools_SetIntValue(context, 0, 0);
      APITools_SetFloatValue(context, 1, 0.0);
      return;
    }
    
    SQLLEN is_null;
    double value;
    SQLRETURN status = SQLGetData(stmt, i, SQL_C_DOUBLE, &value, 100, &is_null);
    if(SQL_OK) {
      APITools_SetIntValue(context, 0, is_null == SQL_NULL_DATA);
      APITools_SetFloatValue(context, 1, value);
#ifdef _DEBUG
      cout << "  " << value << endl;
#endif
      return;
    }
    
    APITools_SetIntValue(context, 0, 0);
    APITools_SetFloatValue(context, 1, 0.0);
  }
  
  //
  // set a string for a prepared statement
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_stmt_set_string(VMContext& context) {
    char* value = APITools_GetStringValue(context, 1);
    long i = APITools_GetIntValue(context, 2);
    SQLHSTMT stmt = (SQLHDBC)APITools_GetIntValue(context, 3);
    
#ifdef _DEBUG
    cout << "### set_string: stmt=" << stmt << ", column=" << i 
	 << ", value=" << value << " ###" << endl;
#endif  
    
    SQLRETURN status = SQLBindParameter(stmt, i, SQL_PARAM_INPUT, SQL_C_CHAR, 
					SQL_CHAR, 0, 0, value, strlen(value), NULL);
    if(SQL_OK) { 
      APITools_SetIntValue(context, 0, 1);
      return;
    }
    
    APITools_SetIntValue(context, 0, 0);
  }
  
  //
  // gets a string from a result set
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_result_get_string(VMContext& context) {
    long i = APITools_GetIntValue(context, 2);
    SQLHSTMT stmt = (SQLHDBC)APITools_GetIntValue(context, 3);
    vector<const char*>* names = (vector<const char*>*)APITools_GetIntValue(context, 4);
    
#ifdef _DEBUG
    cout << "### get_string: stmt=" << stmt << ", column=" << i 
	 << ", max=" << (long)names->size() << " ###" << endl;
#endif  
    
    if(!stmt || !names || i < 1 || i > (long)names->size()) {
      APITools_SetIntValue(context, 0, 0);
      APITools_SetObjectValue(context, 1, NULL);
      return;
    }

    SQLLEN is_null;
    char value[VARCHAR_MAX];
    SQLRETURN status = SQLGetData(stmt, i, SQL_C_CHAR, &value, 
				  VARCHAR_MAX, &is_null);
    if(SQL_OK) {
      APITools_SetIntValue(context, 0, is_null == SQL_NULL_DATA);
      APITools_SetStringValue(context, 1, value);
#ifdef _DEBUG
      cout << "  " << value << endl;
#endif
      return;
    }
    
    APITools_SetIntValue(context, 0, 0);
    APITools_SetObjectValue(context, 1, NULL);
  }

  //
  // gets a timestampe from a result set
  //  
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_result_get_timestamp(VMContext& context) {
    long i = APITools_GetIntValue(context, 2);
    SQLHSTMT stmt = (SQLHDBC)APITools_GetIntValue(context, 3);
    vector<const char*>* names = (vector<const char*>*)APITools_GetIntValue(context, 4);
    
#ifdef _DEBUG
    cout << "### get_timestamp: stmt=" << stmt << ", column=" << i << ", max=" << (long)names->size() << " ###" << endl;
#endif  
    
    if(!stmt || !names || i < 1 || i > (long)names->size()) {
      APITools_SetIntValue(context, 0, 0);
      APITools_SetObjectValue(context, 1, NULL);
      return;
    }

    SQLLEN is_null;
    TIMESTAMP_STRUCT value;
    SQLRETURN status = SQLGetData(stmt, i, SQL_C_TYPE_TIMESTAMP, &value, 
				  sizeof(TIMESTAMP_STRUCT), &is_null);
    if(SQL_OK) {
      APITools_SetIntValue(context, 0, is_null == SQL_NULL_DATA);
      long* ts_obj = context.alloc_obj("ODBC.Timestamp", (long*)context.op_stack, *context.stack_pos);
      ts_obj[0] = value.year;
      ts_obj[1] = value.month;
      ts_obj[2] = value.day;
      ts_obj[3] = value.hour;
      ts_obj[4] = value.minute;
      ts_obj[5] = value.second;
      ts_obj[6] = value.fraction;
      
#ifdef _DEBUG
      cout << "  " << value.year << endl;
      cout << "  " << value.month << endl;
      cout << "  " << value.day << endl;
      cout << "  " << value.hour << endl;
      cout << "  " << value.minute << endl;
      cout << "  " << value.second << endl;
      cout << "  " << value.fraction << endl;
#endif
      
      // set values
      APITools_SetIntValue(context, 0, 0);
      APITools_SetObjectValue(context, 1, ts_obj);
      return;
    }
    
    APITools_SetIntValue(context, 0, 0);
    APITools_SetObjectValue(context, 1, NULL);
  }
  
  //
  // closes a result set
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void odbc_result_close(VMContext& context) {
    SQLHSTMT stmt = (SQLHDBC)APITools_GetIntValue(context, 0);
    if(stmt) {
      SQLFreeStmt(stmt, SQL_CLOSE);
    }
    
    vector<const char*>* names = (vector<const char*>*)APITools_GetIntValue(context, 1);
    if(names) {
      delete names;
      names = NULL;
    }
    
#ifdef _DEBUG
    cout << "### closed statement ###" << endl;
#endif
  }
}
