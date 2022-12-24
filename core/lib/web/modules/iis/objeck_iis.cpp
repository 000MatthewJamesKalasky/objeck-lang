// <Snippet1>
#define _WINSOCKAPI_
#include <windows.h>
#include <sal.h>
#include <httpserv.h>
#include <psapi.h>

#include <iostream>
#include "sys.h"
#include "loader.h"
#include "interpreter.h"

// Create the module class.
class ObjeckIIS : public CHttpModule
{
public:
  std::string m_html;
  bool is_ok;
  Runtime::StackInterpreter* intpr;
  size_t* op_stack; long* stack_pos;

  ObjeckIIS() {
    is_ok = false;

    HMODULE module = GetModuleHandle("objeck_iis");
    if(module) {
      const size_t max_size = 512;
      char buffer[max_size] = {0};
      if(!GetModuleFileName(module, (LPSTR)buffer, max_size)) {
        std::string buffer_str(buffer);
        size_t find_pos = buffer_str.find_last_of('\\');
        if(find_pos != std::wstring::npos) {
          std::string base_path = buffer_str.substr(0, find_pos); 
          base_path += "\\config.ini";
          // read ini entry
          GetPrivateProfileString("objeck", "program", "(none)", (LPSTR)&buffer, max_size, base_path.c_str());
          
          // load program
          Loader loader(BytesToUnicode(buffer).c_str());
          loader.Load();
          m_html = "--- Loaded ---";


          // ignore non-web applications
          if(!loader.IsWeb()) {
            std::wcout << L"Please recompile the code to be a web application." << std::endl;
            exit(1);
          }

          intpr = new Runtime::StackInterpreter(Loader::GetProgram());
          Runtime::StackInterpreter::AddThread(intpr);

          op_stack = new size_t[OP_STACK_SIZE];
          stack_pos = new long;
        }

        is_ok = true;
      }
    }
  }

  ~ObjeckIIS() {
    delete stack_pos;
    stack_pos = nullptr;

    delete[] op_stack;
    op_stack = nullptr;

    delete intpr;
    intpr = nullptr;
  }

  void LoadProgam(const std::wstring &name) {
    Loader loader(name.c_str());
    loader.Load();
  }

  REQUEST_NOTIFICATION_STATUS OnBeginRequest(IN IHttpContext* pHttpContext, IN IHttpEventProvider* pProvider)
  {
    UNREFERENCED_PARAMETER(pProvider);

    // Retrieve a pointer to the response.
    IHttpRequest* request = pHttpContext->GetRequest();
    IHttpResponse* response = pHttpContext->GetResponse();

    // Test for an error.
    if(request && response) {
      // Clear the existing response.
      response->Clear();
      
      // Set the MIME type to plain text.
      const std::string header = "text/plain";
      response->SetHeader(HttpHeaderContentType, header.c_str(), (USHORT)header.size(), TRUE);

      // Create and set the data chunk.
      HTTP_DATA_CHUNK data_chunk;
      data_chunk.DataChunkType = HttpDataChunkFromMemory;
      data_chunk.FromMemory.pBuffer = (PVOID)m_html.c_str();
      data_chunk.FromMemory.BufferLength = (USHORT)m_html.size();

      // Insert the data chunk into the response.
      DWORD sent;
      const HRESULT result = response->WriteEntityChunks(&data_chunk, 1, FALSE, TRUE, &sent);

      // Test for an error.
      if(FAILED(result)) {
        // Set the HTTP status.
        response->SetStatus(500, "Server Error", 0, result);
      }

      // End additional processing.
      return RQ_NOTIFICATION_FINISH_REQUEST;
    }

    // Return processing to the pipeline.
    return RQ_NOTIFICATION_CONTINUE;
  }
};

// Create the module's class factory.
class ObjeckIISFactory : public IHttpModuleFactory
{
public:
  HRESULT
    GetHttpModule(OUT CHttpModule** ppModule, IN IModuleAllocator* pAllocator)
  {
    UNREFERENCED_PARAMETER(pAllocator);

    // Create a new instance.
    ObjeckIIS* pModule = new ObjeckIIS;

    // Test for an error.
    if(!pModule) {
      // Return an error if the factory cannot create the instance.
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
      // Return a pointer to the module.
      *ppModule = pModule;
      pModule = nullptr;
      // Return a success status.
      return S_OK;
    }
  }

  void Terminate()
  {
    // Remove the class from memory.
    delete this;
  }
};

// Create the module's exported registration function.
HRESULT
__stdcall
RegisterModule(DWORD dwServerVersion, IHttpModuleRegistrationInfo* pModuleInfo, IHttpServer* pGlobalInfo)
{
  UNREFERENCED_PARAMETER(dwServerVersion);
  UNREFERENCED_PARAMETER(pGlobalInfo);

  // Set the request notifications and exit.
  return pModuleInfo->SetRequestNotifications(new ObjeckIISFactory, RQ_BEGIN_REQUEST, 0);
}
// </Snippet1>