// <Snippet1>
#define _WINSOCKAPI_
#include <windows.h>
#include <sal.h>
#include <httpserv.h>
#include <string>

// Create the module class.
class ObjeckIIS : public CHttpModule
{
public:
  std::string m_html;

  ObjeckIIS() {
    m_html = "<html><h2>Guess Who's Back?</h2></ html>";
  }

  ~ObjeckIIS() {

  }

  REQUEST_NOTIFICATION_STATUS OnBeginRequest(IN IHttpContext* pHttpContext, IN IHttpEventProvider* pProvider)
  {
    UNREFERENCED_PARAMETER(pProvider);

    // Retrieve a pointer to the response.
    IHttpRequest* pHttpRequest = pHttpContext->GetRequest();
    IHttpResponse* pHttpResponse = pHttpContext->GetResponse();

    // Test for an error.
    if(pHttpRequest && pHttpResponse) {
      // Clear the existing response.
      pHttpResponse->Clear();
      
      // Set the MIME type to plain text.
      pHttpResponse->SetHeader(HttpHeaderContentType, "text/html", (USHORT)strlen("text/html"), TRUE);

      // Create a data chunk.
      HTTP_DATA_CHUNK dataChunk;
      
      // Set the chunk to a chunk in memory.
      dataChunk.DataChunkType = HttpDataChunkFromMemory;
      
      // Buffer for bytes written of data chunk.
      DWORD cbSent;

      // Set the chunk to the buffer.
      dataChunk.FromMemory.pBuffer = (PVOID)m_html.c_str();
      
      // Set the chunk size to the buffer size.
      dataChunk.FromMemory.BufferLength = (USHORT)m_html.size();

      // Insert the data chunk into the response.
      HRESULT hr = pHttpResponse->WriteEntityChunks(&dataChunk, 1, FALSE, TRUE, &cbSent);

      // Test for an error.
      if(FAILED(hr)) {
        // Set the HTTP status.
        pHttpResponse->SetStatus(500, "Server Error", 0, hr);
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
      pModule = NULL;
      // Return a success status.
      return S_OK;
    }
  }

  void
    Terminate()
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