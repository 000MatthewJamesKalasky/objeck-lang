/* 
**  mod_objeck.c -- Apache sample objeck module
**  [Autogenerated via ``apxs -n objeck -g'']
**
**  To play with this sample module first compile it into a
**  DSO file and install it into Apache's modules directory 
**  by running:
**
**    $ apxs -c -i mod_objeck.c
**
**  Then activate it in Apache's apache2.conf file for instance
**  for the URL /objeck in as follows:
**
**    #   apache2.conf
**    LoadModule objeck_module modules/mod_objeck.so
**    <Location /objeck>
**    SetHandler objeck
**    </Location>
**
**  Then after restarting Apache via
**
**    $ apachectl restart
**
**  you immediately can request the URL /objeck and watch for the
**  output of this module. This can be achieved for instance via:
**
**    $ lynx -mime_header http://localhost/objeck 
**
**  The output should be similar to the following one:
**
**    HTTP/1.1 200 OK
**    Date: Tue, 31 Mar 1998 14:42:22 GMT
**    Server: Apache/1.3.4 (Unix)
**    Connection: close
**    Content-Type: text/html
**  
**    The sample page from mod_objeck.c
*/ 

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "ap_config.h"
#include "http_log.h"

#include <dlfcn.h>

typedef void (*vm_init_def)(const char*, const char*, const char*);
typedef void (*vm_call_def)(request_rec*);
typedef void (*vm_exit_def)();

static apr_pool_t* call_pool = NULL;

static apr_status_t destroy_call_pool (void * dummy)
{
  void* data;
  vm_exit_def exit_ptr;
  
  /* call clean up function on library */
  apr_pool_userdata_get(&data, "objeck:exit", call_pool);
  if(!data) {
    return 1;
  }
  exit_ptr = (vm_exit_def)data;
  (*exit_ptr)();
  
  /* release library handle */
  apr_pool_userdata_get(&data, "objeck:lib", call_pool);
  if(!data) {
    return 1;
  }
  dlclose(data);
  
  /* release pool */
  apr_pool_destroy(call_pool);
  call_pool = NULL;
  return APR_SUCCESS;
}

/* child initialization */
static void exipc_child_init(apr_pool_t *p, server_rec *s)
{
  void* dynamic_lib;
  vm_init_def init_ptr;
  vm_call_def call_ptr;
  vm_exit_def exit_ptr;
  char* error;
  
  /* load library */
  dynamic_lib = dlopen("/tmp/obr.so", RTLD_LAZY);
  if(!dynamic_lib) {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
		 ">>> Runtime error loading DLL: %s\n", 
		 dlerror());
    exit(1);
  }
  
  /* load function references */
  init_ptr = (vm_init_def)dlsym(dynamic_lib, "Init");
  if((error = dlerror()) != NULL)  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
		 ">>> Runtime error calling function: %s\n",
		 error);
    exit(1);
  }

  call_ptr = (vm_call_def)dlsym(dynamic_lib, "Call");
  if((error = dlerror()) != NULL)  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
		 ">>> Runtime error calling function: %s\n", 
		 error);
    exit(1);
  }
  
  exit_ptr = (vm_exit_def)dlsym(dynamic_lib, "Exit");
  if((error = dlerror()) != NULL)  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
		 ">>> Runtime error exiting function: %s", 
		 error);
    exit(1);
  }
  
  /* create resource pool */
  if(APR_SUCCESS == apr_pool_create(&call_pool, 0)) {
    apr_pool_cleanup_register(p, 0, destroy_call_pool, destroy_call_pool);
  }
  
  apr_pool_userdata_set(dynamic_lib, "objeck:lib", apr_pool_cleanup_null, call_pool);
  apr_pool_userdata_set(call_ptr, "objeck:call", apr_pool_cleanup_null, call_pool);
  apr_pool_userdata_set(exit_ptr, "objeck:exit", apr_pool_cleanup_null, call_pool);
  
  /* call library initialization function */
  (*init_ptr)("/tmp/a.obe", "ApacheModule", "ApacheModule:Request:o.Apache.Request,");
}

/* content handler */
static int objeck_handler(request_rec *r)
{ 
  void* data = NULL; 

  if(strcmp(r->handler, "objeck")) {
    return DECLINED;
  }

  apr_pool_userdata_get(&data, "objeck:call", call_pool);
  if(!data) {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, 
		 ">>> Unable to load calling funciton <<<");
    return 1;
  }
  vm_call_def call_ptr = (vm_call_def)data;    
  
  /* set content type to html and call VM */
  r->content_type = "text/html";      
  if(!r->header_only) {
    (*call_ptr)(r);
  }

  return OK;
}

static void objeck_register_hooks(apr_pool_t *p)
{
  ap_hook_child_init(exipc_child_init, NULL, NULL, APR_HOOK_MIDDLE);
  ap_hook_handler(objeck_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA objeck_module = {
  STANDARD20_MODULE_STUFF, 
  NULL,                  /* create per-dir    config structures */
  NULL,                  /* merge  per-dir    config structures */
  NULL,                  /* create per-server config structures */
  NULL,                  /* merge  per-server config structures */
  NULL,                  /* table of config file commands       */
  objeck_register_hooks  /* register hooks                      */
};

