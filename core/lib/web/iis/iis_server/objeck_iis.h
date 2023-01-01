/***************************************************************************
 * IIS ISAPI server module
 * 
 * Copyright (c) 2023, Randy Hollines
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 * - Neither the name of the StackVM Team nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#ifndef __OBJECK_IIS_H__
#define __OBJECK_IIS_H__

#include <windows.h>
#include <sal.h>
#include <httpserv.h>

#include "memory.h"
#include "loader.h"
#include "interpreter.h"
#include "sys.h"
#include "logger.h"

//
// IIS server
//
class ObjeckIIS : public CHttpModule {
  static Loader* loader;
  static Runtime::StackInterpreter* intpr;
  static StackMethod* method;
  
  static size_t* op_stack;
  static long* stack_pos;
#ifdef _DEBUG
  static std::wstreambuf* tmp_wcout;
  static std::wstreambuf* tmp_werr;
#endif

  void LogSetupEnvironment(const std::string& progam_path, const std::string& install_path, const std::string& lib_name);
  std::map<std::string, std::string> LoadConfiguration();

public:
  ObjeckIIS();
  ~ObjeckIIS();

  void StartInterpreter();
  static void StopInterpreter();

  bool WriteResponseString(const std::string data, IHttpResponse* response);
  void SetContentType(const std::string header, IHttpResponse* response);

  REQUEST_NOTIFICATION_STATUS OnBeginRequest(IN IHttpContext* pHttpContext, IN IHttpEventProvider* pProvider);
};

//
// Server module factory
//
class ObjeckIISFactory : public IHttpModuleFactory {

public:
  HRESULT GetHttpModule(OUT CHttpModule** ppModule, IN IModuleAllocator* pAllocator);
  void Terminate();
};

//
// Entrypoint
//
HRESULT __stdcall RegisterModule(DWORD dwServerVersion, IHttpModuleRegistrationInfo* pModuleInfo, IHttpServer* pGlobalInfo);

#endif