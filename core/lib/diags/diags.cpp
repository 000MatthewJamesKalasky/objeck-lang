/***************************************************************************
 * Diagnostics support for Objeck
 *
 * Copyright (c) 2011-2015, Randy Hollines
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
 * - Neither the name of the Objeck Team nor the names of its
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

#include "diags.h"

#include "..\..\..\compiler\parser.h"
#include "..\..\..\compiler\tree.h"
#include "..\..\..\vm\lib_api.h"

using namespace std;

Diagnostic::Diagnostic(int l, int p, wstring m)
{
  line = l;
  pos = p;
  msg = m;
}

Diagnostic::~Diagnostic()
{

}

int Diagnostic::GetLine() 
{
  return line;
}

int Diagnostic::GetPos()
{
  return pos;
}

wstring& Diagnostic::GetMsg()
{
  return msg;
}

extern "C" {

  //
  // initialize diagnostics environment
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void load_lib()
  {
#ifdef _DEBUG
    OpenLogger("debug.log");
#endif
  }

  //
  // release diagnostics resources
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void unload_lib()
  {
#ifdef _DEBUG
    CloseLogger();
#endif
  }

  //
  // parse source file
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void diag_parse_file(VMContext& context)
  {

    const wstring src_file(APITools_GetStringValue(context, 2));
    const wstring sys_path(APITools_GetStringValue(context, 3));

#ifdef _DEBUG
    wcout << L"### connect: " << L"src_file=" << src_file << L", sys_path=" << sys_path << L" ###" << endl;
#endif

    Parser parser(src_file, false, sys_path);
    const bool was_parsed = parser.Parse();

    APITools_SetIntValue(context, 0, (size_t)parser.GetProgram());
    APITools_SetIntValue(context, 1, was_parsed ? 1 : 0);
  }

 #ifdef _WIN32
    __declspec(dllexport)
  #endif
      void diag_tree_get_symbols(VMContext& context)
    {
      ParsedProgram* program = (ParsedProgram*)APITools_GetIntValue(context, 1);
      
      vector<ParsedBundle*> bundles = program->GetBundles();
      size_t* bundle_array = APITools_MakeIntArray(context, (int)bundles.size());
      size_t* bundle_array_ptr = bundle_array + 3;
      for(size_t i = 0; i < bundles.size(); ++i) {
        size_t* symb_obj = APITools_CreateObject(context, L"System.Diagnostics.AnalysisSymbol");
        ParsedBundle* bundle = bundles[i];

        symb_obj[0] = (size_t)APITools_CreateStringValue(context, bundle->GetName());
        symb_obj[1] = 13;
        symb_obj[2] = bundle->GetLineNumber();
        symb_obj[3] = bundle->GetLinePosition();
      
        bundle_array_ptr[i] = (size_t)symb_obj;
      }

      
  }

 #ifdef _WIN32
    __declspec(dllexport)
#endif
    void diag_tree_release(VMContext& context)
    {
      ParsedProgram* program = (ParsedProgram*)APITools_GetIntValue(context, 1);
      if(program) {
        delete program;
        program = nullptr;
      }
    }
}