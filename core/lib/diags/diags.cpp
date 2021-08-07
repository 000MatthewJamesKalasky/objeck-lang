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

#include "../../../compiler/parser.h"
#include "../../../compiler/context.h"

using namespace std;
using namespace frontend;

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

#ifdef _WIN32
  __declspec(dllexport)
#endif
    void diag_tree_release(VMContext& context)
  {
    ParsedProgram* program = (ParsedProgram*)APITools_GetIntValue(context, 0);
    if(program) {
      delete program;
      program = nullptr;
    }
  }

  //
  // Parse file
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void diag_parse_file(VMContext& context)
  {
    const wstring src_file(APITools_GetStringValue(context, 2));

#ifdef _DEBUG
    wcout << L"Parse file: src_file='" << src_file << L"'" << endl;
#endif

    Parser parser(src_file, false, L"");
    const bool was_parsed = parser.Parse();

    APITools_SetIntValue(context, 0, (size_t)parser.GetProgram());
    APITools_SetIntValue(context, 1, was_parsed ? 1 : 0);
  }
  
  //
  // Parse text
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void diag_parse_text(VMContext& context)
  {
    const wstring src_text(APITools_GetStringValue(context, 2));

#ifdef _DEBUG
    wcout << L"Parse file: text_size=" << src_text.size() << L"'" << endl;
#endif

    Parser parser(L"", false, src_text);
    const bool was_parsed = parser.Parse();

    APITools_SetIntValue(context, 0, (size_t)parser.GetProgram());
    APITools_SetIntValue(context, 1, was_parsed ? 1 : 0);
  }

  //
  // get diagnostics (error and warnings)
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void diag_get_diagnosis(VMContext& context)
  {
    size_t* prgm_obj = APITools_GetObjectValue(context, 0);
    ParsedProgram* program = (ParsedProgram*)prgm_obj[ResultPosition::POS_NAME];

    const wstring sys_path = APITools_GetStringValue(context, 1);

    wstring full_path = L"lang.obl";
    if(!sys_path.empty()) {
      full_path += L',' + sys_path;
    }

    // if parsed
    if(!prgm_obj[ResultPosition::POS_TYPE]) {
      vector<wstring> error_strings = program->GetErrorStrings();
      size_t* diagnostics_array = FormatErrors(context, error_strings);

      // diagnostics
      prgm_obj[ResultPosition::POS_DESC] = (size_t)diagnostics_array;
    }
    else {
      ContextAnalyzer analyzer(program, full_path, false, false);
      if(!analyzer.Analyze()) {
        vector<wstring> error_strings = program->GetErrorStrings();
        size_t* diagnostics_array = FormatErrors(context, error_strings);

        // diagnostics
        prgm_obj[ResultPosition::POS_DESC] = (size_t)diagnostics_array;
      }
    }
  }
    
  //
  // get symbols
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void diag_get_symbols(VMContext& context)
  {
    size_t* prgm_obj = APITools_GetObjectValue(context, 0);
    ParsedProgram* program = (ParsedProgram*)prgm_obj[ResultPosition::POS_NAME];

    const vector<ParsedBundle*> bundles = program->GetBundles();
    size_t* bundle_array = APITools_MakeIntArray(context, (int)bundles.size());
    size_t* bundle_array_ptr = bundle_array + 3;
    
    // bundles
    wstring file_name;
    for(size_t i = 0; i < bundles.size(); ++i) {
      ParsedBundle* bundle = bundles[i];
      file_name = bundle->GetFileName();

      // bundle
      size_t* bundle_symb_obj = APITools_CreateObject(context, L"System.Diagnostics.Result");
      const wstring bundle_name = bundle->GetName();
      bundle_symb_obj[ResultPosition::POS_NAME] = (size_t)APITools_CreateStringValue(context, bundle_name.empty() ? L"Default" : bundle_name);
      bundle_symb_obj[ResultPosition::POS_TYPE] = ResultType::TYPE_NAMESPACE; // namespace type
      bundle_symb_obj[ResultPosition::POS_START_LINE] = bundle->GetLineNumber();
      bundle_symb_obj[ResultPosition::POS_START_POS] = bundle->GetLinePosition();
      bundle_symb_obj[ResultPosition::POS_END_LINE] = bundle->GetEndLineNumber();
      bundle_symb_obj[ResultPosition::POS_END_POS] = bundle->GetEndLinePosition();
      bundle_array_ptr[i] = (size_t)bundle_symb_obj;

      // get classes and enums
      const vector<Class*> klasses = bundle->GetClasses();
      const vector<Enum*> eenums = bundle->GetEnums();

      // classes
      size_t* klass_array = APITools_MakeIntArray(context, (int)(klasses.size() + eenums.size()));
      size_t* klass_array_ptr = klass_array + 3;

      size_t index = 0;
      for(size_t j = 0; j < klasses.size(); ++j) {
        Class* klass = klasses[j];

        size_t* klass_symb_obj = APITools_CreateObject(context, L"System.Diagnostics.Result");
        klass_symb_obj[ResultPosition::POS_NAME] = (size_t)APITools_CreateStringValue(context, klass->GetName());
        klass_symb_obj[ResultPosition::POS_TYPE] = ResultType::TYPE_CLASS; // class type
        klass_symb_obj[ResultPosition::POS_START_LINE] = klass->GetLineNumber();
        klass_symb_obj[ResultPosition::POS_START_POS] = klass->GetLinePosition();
        klass_symb_obj[ResultPosition::POS_END_LINE] = klass->GetEndLineNumber();
        klass_symb_obj[ResultPosition::POS_END_POS] = klass->GetEndLinePosition();
        klass_array_ptr[index++] = (size_t)klass_symb_obj;

        // methods
        vector<Method*> mthds = klass->GetMethods();
        size_t* mthds_array = APITools_MakeIntArray(context, (int)mthds.size());
        size_t* mthds_array_ptr = mthds_array + 3;
        for(size_t k = 0; k < mthds.size(); ++k) {
          Method* mthd = mthds[k];

          size_t* mthd_symb_obj = APITools_CreateObject(context, L"System.Diagnostics.Result");

          wstring mthd_name = mthd->GetName();
          const size_t mthd_name_index = mthd_name.find_last_of(L':');
          if(mthd_name_index != wstring::npos) {
            mthd_name = mthd_name.substr(mthd_name_index + 1, mthd_name.size() - mthd_name_index - 1);
          }
          mthd_symb_obj[ResultPosition::POS_NAME] = (size_t)APITools_CreateStringValue(context, mthd_name);

          mthd_symb_obj[ResultPosition::POS_TYPE] = ResultType::TYPE_METHOD; // method type
          mthd_symb_obj[ResultPosition::POS_START_LINE] = mthd->GetLineNumber();
          mthd_symb_obj[ResultPosition::POS_START_POS] = mthd->GetLinePosition();
          mthd_symb_obj[ResultPosition::POS_END_LINE] = mthd->GetEndLineNumber();
          mthd_symb_obj[ResultPosition::POS_END_POS] = mthd->GetEndLinePosition();
          mthds_array_ptr[k] = (size_t)mthd_symb_obj;
        }
        klass_symb_obj[ResultPosition::POS_CHILDREN] = (size_t)mthds_array;
      }

      // enums
      for(size_t j = 0; j < eenums.size(); ++j) {
        Enum* eenum = eenums[j];

        wstring eenum_name = eenum->GetName();
        const size_t eenum_name_index = eenum_name.find_last_of(L'#');
        if(eenum_name_index != wstring::npos) {
          eenum_name = eenum_name.substr(eenum_name_index + 1, eenum_name.size() - eenum_name_index - 1);
        }

        size_t* eenum_symb_obj = APITools_CreateObject(context, L"System.Diagnostics.Result");
        eenum_symb_obj[ResultPosition::POS_NAME] = (size_t)APITools_CreateStringValue(context, eenum_name);
        eenum_symb_obj[ResultPosition::POS_TYPE] = ResultType::TYPE_ENUM; // enum type
        eenum_symb_obj[ResultPosition::POS_START_LINE] = eenum->GetLineNumber();
        eenum_symb_obj[ResultPosition::POS_START_POS] = eenum->GetLinePosition();
        eenum_symb_obj[ResultPosition::POS_END_LINE] = eenum->GetEndLineNumber();
        eenum_symb_obj[ResultPosition::POS_END_POS] = eenum->GetEndLinePosition();
        klass_array_ptr[index++] = (size_t)eenum_symb_obj;
      }

      bundle_symb_obj[ResultPosition::POS_CHILDREN] = (size_t)klass_array;
    }

    // file root
    size_t* file_symb_obj = APITools_CreateObject(context, L"System.Diagnostics.Result");
    file_symb_obj[ResultPosition::POS_NAME] = (size_t)APITools_CreateStringValue(context, file_name);
    file_symb_obj[ResultPosition::POS_TYPE] = ResultType::TYPE_FILE; // file type
    file_symb_obj[ResultPosition::POS_CHILDREN] = (size_t)bundle_array;
    file_symb_obj[ResultPosition::POS_START_LINE] = file_symb_obj[ResultPosition::POS_START_POS] = file_symb_obj[ResultPosition::POS_END_LINE] = file_symb_obj[ResultPosition::POS_END_POS] = -1;

    prgm_obj[ResultPosition::POS_CHILDREN] = (size_t)file_symb_obj;
  }

  //
  // find declaration
  //
#ifdef _WIN32
  __declspec(dllexport)
#endif
  void diag_find_declaration(VMContext& context)
  {
    size_t* prgm_obj = APITools_GetObjectValue(context, 1);
    ParsedProgram* program = (ParsedProgram*)prgm_obj[ResultPosition::POS_NAME];

    const int line_num = (int)APITools_GetIntValue(context, 2);
    const int line_pos = (int)APITools_GetIntValue(context, 3);
    const wstring sys_path = APITools_GetStringValue(context, 4);

    SymbolTable* table = nullptr;
    Method* method = program->FindMethod(line_num, table);
    if(method) {
      wstring full_path = L"lang.obl";
      if(!sys_path.empty()) {
        full_path += L',' + sys_path;
      }

      ContextAnalyzer analyzer(program, full_path, false, false);
      if(analyzer.Analyze()) {
        SymbolEntry* entry = analyzer.GetDeclaration(method, line_num, line_pos);
        if(entry) {
          // deceleration result
          wstring dclr_name = entry->GetName();
          size_t var_name_pos = dclr_name.find_last_of(L':');
          if(var_name_pos != wstring::npos) {
            dclr_name = dclr_name.substr(var_name_pos + 1, dclr_name.size() - var_name_pos - 1);
          }

          size_t* dcrl_obj = APITools_CreateObject(context, L"System.Diagnostics.Result");
          dcrl_obj[ResultPosition::POS_NAME] = (size_t)APITools_CreateStringValue(context, dclr_name);
          dcrl_obj[ResultPosition::POS_TYPE] = ResultType::TYPE_NAMESPACE; // namespace type
          dcrl_obj[ResultPosition::POS_START_LINE] = dcrl_obj[ResultPosition::POS_END_LINE] = entry->GetLineNumber() - 1;
          dcrl_obj[ResultPosition::POS_START_POS] = entry->GetLinePosition() - 2; // 17
          dcrl_obj[ResultPosition::POS_END_POS] = entry->GetLinePosition() + (int)dclr_name.size() - 2; // = 32

          APITools_SetObjectValue(context, 0, dcrl_obj);
        }
      }
    }
  }
  
  //
  // find references
  //
#ifdef _WIN32
    __declspec(dllexport)
#endif
  void diag_find_references(VMContext& context)
  {
    size_t* prgm_obj = APITools_GetObjectValue(context, 0);
    ParsedProgram* program = (ParsedProgram*)prgm_obj[ResultPosition::POS_NAME];

    const int line_num = (int)APITools_GetIntValue(context, 1);
    const int line_pos = (int)APITools_GetIntValue(context, 2);
    const wstring sys_path = APITools_GetStringValue(context, 3);

    SymbolTable* table = nullptr;
    Method* method = program->FindMethod(line_num, table);
    if(method) {
      wstring full_path = L"lang.obl";
      if(!sys_path.empty()) {
        full_path += L',' + sys_path;
      }

      ContextAnalyzer analyzer(program, full_path, false, false);
      if(analyzer.Analyze()) {
        vector<Expression*> expressions = analyzer.GetExpressions(method, line_num, line_pos);
        size_t* refs_array = APITools_MakeIntArray(context, (int)expressions.size());
        size_t* refs_array_ptr = refs_array + 3;

        for(size_t i = 0; i < expressions.size(); ++i) {
          Expression* expression = expressions[i];

          size_t* reference_obj = APITools_CreateObject(context, L"System.Diagnostics.Result");
          int start_pos = expression->GetLinePosition() - 1;
          int end_pos = start_pos;
          
          switch(expression->GetExpressionType()) {
          case VAR_EXPR: {
            Variable* variable = static_cast<Variable*>(expression);
            end_pos += (int)variable->GetName().size();
            reference_obj[ResultPosition::POS_NAME] = (size_t)APITools_CreateStringValue(context, variable->GetName());
          }
            break;

          case METHOD_CALL_EXPR: {
            MethodCall* method_call = static_cast<MethodCall*>(expression);
            start_pos++; end_pos++;
            end_pos += (int)method_call->GetVariableName().size();
            reference_obj[ResultPosition::POS_NAME] = (size_t)APITools_CreateStringValue(context, method_call->GetMethodName());
          }
            break;
          }
          
          reference_obj[ResultPosition::POS_TYPE] = ResultType::TYPE_VARIABLE; // variable type
          reference_obj[ResultPosition::POS_START_LINE] = reference_obj[ResultPosition::POS_END_LINE] = expression->GetLineNumber() - 1;
          reference_obj[ResultPosition::POS_START_POS] = start_pos - 1;
          reference_obj[ResultPosition::POS_END_POS] = end_pos - 1;          
          refs_array_ptr[i] = (size_t)reference_obj;
        }

        prgm_obj[ResultPosition::POS_START_LINE] = (size_t)refs_array;
      }
    }
  }

  //
  // Supporting functions
  //
  
  size_t* FormatErrors(VMContext& context, vector<wstring> error_strings)
  {
    size_t* diagnostics_array = APITools_MakeIntArray(context, (int)error_strings.size());
    size_t* diagnostics_array_ptr = diagnostics_array + 3;

    for(size_t i = 0; i < error_strings.size(); ++i) {
      const wstring error_string = error_strings[i];

      // parse error string
      const size_t file_mid = error_string.find(L":(");
      const wstring file_str = error_string.substr(0, file_mid);

      const size_t msg_mid = error_string.find(L"):");
      const wstring msg_str = error_string.substr(msg_mid + 3, error_string.size() - msg_mid - 3);

      const wstring line_pos_str = error_string.substr(file_mid + 2, msg_mid - file_mid - 2);
      const size_t line_pos_mid = line_pos_str.find(L',');
      const wstring line_str = line_pos_str.substr(0, line_pos_mid);
      const wstring pos_str = line_pos_str.substr(line_pos_mid + 1, line_pos_str.size() - line_pos_mid - 1);

      // create objects
      size_t* diag_obj = APITools_CreateObject(context, L"System.Diagnostics.Result");
      diag_obj[ResultPosition::POS_NAME] = (size_t)APITools_CreateStringValue(context, msg_str);
      diag_obj[ResultPosition::POS_TYPE] = ResultType::TYPE_ERROR; // error type
      diag_obj[ResultPosition::POS_DESC] = (size_t)APITools_CreateStringValue(context, file_str);
      diag_obj[ResultPosition::POS_START_LINE] = _wtoi(line_str.c_str());
      diag_obj[ResultPosition::POS_START_POS] = _wtoi(pos_str.c_str());
      diag_obj[ResultPosition::POS_END_LINE] = diag_obj[ResultPosition::POS_END_POS] = -1;
      diagnostics_array_ptr[i] = (size_t)diag_obj;
    }

    return diagnostics_array;
  }
}