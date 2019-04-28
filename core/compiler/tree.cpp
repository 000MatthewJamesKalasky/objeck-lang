/***************************************************************************
 * Language parse tree.
 *
 * Copyright (c) 2008-2018, Randy Hollines
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
 * - Neither the name of the Objeck team nor the names of its
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

#include "tree.h"

using namespace frontend;

/****************************
 * Expression class
 ****************************/
void Expression::SetMethodCall(MethodCall* call)
{
  if(call) {
    method_call = call;
    call->SetPreviousExpression(this);
  }
}

/****************************
 * CharacterString class
 ****************************/
void CharacterString::AddSegment(const wstring &orig) 
{
  if(!is_processed) {
    wstring escaped_str;
    int skip = 2;
    for(size_t i = 0; i < orig.size(); ++i) {
      wchar_t c = orig[i];
      if(skip > 1 && c == L'\\' && i + 1 < orig.size()) {
  wchar_t cc = orig[i + 1];
  switch(cc) {
  case L'"':
    escaped_str += L'\"';
    skip = 0;
    break;

  case L'\\':
    escaped_str += L'\\';
    skip = 0;
    break;

  case L'n':
    escaped_str += L'\n';
    skip = 0;
    break;

  case L'r':
    escaped_str += L'\r';
    skip = 0;
    break;

  case L't':
    escaped_str += L'\t';
    skip = 0;
    break;

  case L'a':
    escaped_str += L'\a';
    skip = 0;
    break;

  case L'b':
    escaped_str += L'\b';
    skip = 0;
    break;

#ifndef _WIN32
  case L'e':
    escaped_str += L'\e';
    skip = 0;
    break;
#endif

  case L'f':
    escaped_str += L'\f';
    skip = 0;
    break;

  case L'0':
    escaped_str += L'\0';
    skip = 0;
    break;

  default:
    if(skip <= 1) {
      skip++;
    }
    break;
  }
      }

      if(skip > 1) {
  escaped_str += c;
      }
      else {
  skip++;
      }
    }
    // set string
    segments.push_back(new CharacterStringSegment(escaped_str));
  }
}

/****************************
 * TreeFactory class
 ****************************/
TreeFactory* TreeFactory::instance;

TreeFactory* TreeFactory::Instance()
{
  if(!instance) {
    instance = new TreeFactory;
  }

  return instance;
}

/****************************
 * SymbolEntry class
 ****************************/
SymbolEntry* SymbolEntry::Copy() 
{
  return TreeFactory::Instance()->MakeSymbolEntry(file_name, line_num,
              name, type, is_static,
              is_local, is_self);
}

void SymbolEntry::SetId(int i)
{
  if(id < 0) {
    id = i;
    for(size_t j = 0; j < variables.size(); ++j) {
      variables[j]->SetId(i);
    }
  }
}

/****************************
 * Method class
 ****************************/
wstring Method::EncodeType(Type* type, Class* klass, ParsedProgram* program, Linker* linker)
{
  wstring name;
  if(type) {
    // type
    switch(type->GetType()) {
    case BOOLEAN_TYPE:
      name = L'l';
      break;

    case BYTE_TYPE:
      name = L'b';
      break;

    case INT_TYPE:
      name = L'i';
      break;

    case FLOAT_TYPE:
      name = L'f';
      break;

    case CHAR_TYPE:
      name = L'c';
      break;

    case NIL_TYPE:
      name = L'n';
      break;

    case VAR_TYPE:
      name = L'v';
      break;

    case CLASS_TYPE: {
      name = L"o.";
      
      // program class check
      const wstring type_klass_name = type->GetClassName();
      Class* prgm_klass = program->GetClass(type_klass_name);
      if(prgm_klass) {
        name += prgm_klass->GetName();
      }
      else {
        // full path resolution
        vector<wstring> uses = program->GetUses();
        for(size_t i = 0; !prgm_klass && i < uses.size(); ++i) {
          prgm_klass = program->GetClass(uses[i] + L"." + type_klass_name);
        }
        
        // program enum check
        if(prgm_klass) {
          name += prgm_klass->GetName();
        }
        else {
          // full path resolution
          Enum* prgm_enum = program->GetEnum(type_klass_name);
          vector<wstring> uses = program->GetUses();
          for(size_t i = 0; !prgm_enum && i < uses.size(); ++i) {
            prgm_enum = program->GetEnum(uses[i] + L"." + type_klass_name);
          }
          
          if(prgm_enum) {
            name += prgm_enum->GetName();
          }
          else {
            const wstring type_klass_name_ext = klass->GetName() + L"#" + type_klass_name;
            prgm_enum = program->GetEnum(type_klass_name_ext);
            if(prgm_enum) {
              name += type_klass_name_ext;
            }
          }
        }
      }
      
      // search libaraires      
      if(name == L"o.") {
        prgm_klass = klass->GetGenericClass(type_klass_name);
        if(prgm_klass) {
          name += prgm_klass->GetName();
        }
      }

      // search libaraires      
      if(name == L"o.") {
        LibraryClass* lib_klass = linker->SearchClassLibraries(type_klass_name, program->GetUses());
        if(lib_klass) {
          name += lib_klass->GetName();
        } 
        else {
          LibraryEnum* lib_enum = linker->SearchEnumLibraries(type_klass_name, program->GetUses());
          if(lib_enum) {
            name += lib_enum->GetName();
          }
          else {
            const wstring type_klass_name_ext = klass->GetName() + L"#" + type_klass_name;
            lib_enum = linker->SearchEnumLibraries(type_klass_name_ext, program->GetUses());
            if(lib_enum) {
              name += type_klass_name_ext;
            }
          }
        }
      } 
#ifdef _DEBUG
//      assert(name != L"o.");
#endif
    }
      break;
      
    case FUNC_TYPE:  {
      name = L"m.";
      if(type->GetClassName().size() == 0) {
        name += EncodeFunctionType(type->GetFunctionParameters(), type->GetFunctionReturn(), klass, program, linker);
      }
      else {
        name += type->GetClassName();
      }
    }
      break;
    }
    // dimension
    for(int i = 0; i < type->GetDimension(); ++i) {
      name += L'*';
    }
  }

  return name;
}

wstring Method::EncodeFunctionType(vector<Type*> func_params, Type* func_rtrn, 
           Class* klass, ParsedProgram* program, Linker* linker) 
{                                     
  wstring encoded_name = L"(";
  for(size_t i = 0; i < func_params.size(); ++i) {
    // encode params
    encoded_name += EncodeType(func_params[i], klass, program, linker);

    // encode dimension   
    for(int j = 0; j < func_params[i]->GetDimension(); ++j) {
      encoded_name += L'*';
    }
    encoded_name += L',';
  }

  // encode return
  encoded_name += L")~";
  encoded_name += EncodeType(func_rtrn, klass, program, linker);

  return encoded_name;
}

wstring Method::EncodeType(Type* type) {
  wstring name;
  if(type) {
    // type
    switch(type->GetType()) {
    case BOOLEAN_TYPE:
      name = L'l';
      break;

    case BYTE_TYPE:
      name = L'b';
      break;

    case INT_TYPE:
      name = L'i';
      break;

    case FLOAT_TYPE:
      name = L'f';
      break;

    case CHAR_TYPE:
      name = L'c';
      break;

    case NIL_TYPE:
      name = L'n';
      break;

    case VAR_TYPE:
      name = L'v';
      break;

    case CLASS_TYPE:
      name = L"o.";
      name += type->GetClassName();
      break;

    case FUNC_TYPE:
      name = L'm';
      break;
    }

    // dimension
    for(int i = 0; i < type->GetDimension(); ++i) {
      name += L'*';
    }
  }

  return name;
}

wstring Method::EncodeUserType(Type* type) {
  wstring name;
  if(type) {
    // type
    switch(type->GetType()) {
    case BOOLEAN_TYPE:
      name = L"Bool";
      break;

    case BYTE_TYPE:
      name = L"Byte";
      break;

    case INT_TYPE:
      name = L"Int";
      break;

    case FLOAT_TYPE:
      name = L"Float";
      break;

    case CHAR_TYPE:
      name = L"Char";
      break;

    case NIL_TYPE:
      name = L"Nil";
      break;

    case VAR_TYPE:
      name = L"Var";
      break;

    case CLASS_TYPE:
      name = type->GetClassName();
      break;

    case FUNC_TYPE: {
      name = L'(';
      vector<Type*> func_params = type->GetFunctionParameters();
      for(size_t i = 0; i < func_params.size(); ++i) {
  name += EncodeUserType(func_params[i]);
      }
      name += L") ~ ";
      name += EncodeUserType(type->GetFunctionReturn());
    }
      break;
    }

    // dimension
    for(int i = 0; i < type->GetDimension(); ++i) {
      name += L"[]";
    }
  }

  return name;
}

void Method::EncodeUserName() {
  bool is_new_private = false;
  if(is_static) {
    user_name = L"function : ";
  }
  else {
    switch(method_type) {
    case NEW_PUBLIC_METHOD:
      break;

    case NEW_PRIVATE_METHOD:
      is_new_private = true;
      break;

    case PUBLIC_METHOD:
      user_name = L"method : public : ";
      break;

    case PRIVATE_METHOD:
      user_name = L"method : private : ";
      break;
    }
  }

  if(is_native) {
    user_name += L"native : ";
  }

  // name
  user_name += ReplaceSubstring(name, L":", L"->");

  // private new
  if(is_new_private) {
    user_name += L" : private ";
  }

  // params
  user_name += L'(';

  vector<Declaration*> declaration_list = declarations->GetDeclarations();
  for(size_t i = 0; i < declaration_list.size(); ++i) {
    SymbolEntry* entry = declaration_list[i]->GetEntry();
    if(entry) {
      user_name += EncodeUserType(entry->GetType());
      if(i + 1 < declaration_list.size()) {
  user_name += L", ";
      }
    }
  }
  user_name += L") ~ ";

  user_name += EncodeUserType(return_type);
}

/****************************
 * StaticArray class
 ****************************/
void StaticArray::Validate(StaticArray* array) {
  vector<Expression*> static_array = array->GetElements()->GetExpressions();
  for(size_t i = 0; i < static_array.size(); ++i) { 
    if(static_array[i]) {
      if(static_array[i]->GetExpressionType() == STAT_ARY_EXPR) {
  if(i == 0) {
    dim++;
  }
  Validate(static_cast<StaticArray*>(static_array[i]));
      }
      else {
  // check lengths
  if(cur_width == -1) {
    cur_width = (int)static_array.size();
  }
  if(cur_width != (int)static_array.size()) {
    matching_lengths = false;
  }      
  // check types
  if(cur_type == VAR_EXPR) {
    cur_type = static_array[i]->GetExpressionType();
  }
  else if(cur_type != static_array[i]->GetExpressionType()) {
    matching_types = false;
  }
      }
    }
  }
}

ExpressionList* StaticArray::GetAllElements() {
  if(!all_elements) {
    all_elements = TreeFactory::Instance()->MakeExpressionList();  
    GetAllElements(this, all_elements);
    
    // change row/column order    
    if(dim == 2) {
      ExpressionList* temp = TreeFactory::Instance()->MakeExpressionList();
      vector<Expression*> elements = all_elements->GetExpressions();
      // update indices
      for(int i = 0; i < cur_width; ++i) {
  for(int j = 0; j < cur_height; ++j) {
    const int index = j * cur_width + i;
    temp->AddExpression(elements[index]);
  }
      }      
      all_elements = temp;
    }
  }
  
  return all_elements;
}

void StaticArray::GetAllElements(StaticArray* array, ExpressionList* elems)
{
  vector<Expression*> static_array = array->GetElements()->GetExpressions();
  for(size_t i = 0; i < static_array.size(); ++i) {
    if(static_array[i]) {
      if(static_array[i]->GetExpressionType() == STAT_ARY_EXPR) {
        GetAllElements(static_cast<StaticArray*>(static_array[i]), all_elements);
        cur_height++;
      }
      else {
        elems->AddExpression(static_array[i]);
      }
    }
  }
}

void StaticArray::GetSizes(StaticArray* array, int& count)
{
  vector<Expression*> static_array = array->GetElements()->GetExpressions();
  for(size_t i = 0; i < static_array.size(); ++i) {
    if(static_array[i]) {
      if(static_array[i]->GetExpressionType() == STAT_ARY_EXPR) {
        count++;
        GetSizes(static_cast<StaticArray*>(static_array[i]), count);
      }
    }
  }
}

vector<int> StaticArray::GetSizes() {
  if(!sizes.size()) {
    int count = 0;
    GetSizes(this, count);
    
    sizes.push_back(cur_width);
    if(count) {
      sizes.push_back(count);
    }
  }
  
  return sizes;
}

/****************************
 * Variable class
 ****************************/
Variable* Variable::Copy() {
  Variable* v = TreeFactory::Instance()->MakeVariable(file_name, line_num, name);
  v->indices = indices;
  return v;
}

/****************************
 * Declaration class
 ****************************/
Declaration* Declaration::Copy() {
  if(assignment) {
    return TreeFactory::Instance()->MakeDeclaration(file_name, line_num, entry->Copy(), child, assignment);
  }

  return TreeFactory::Instance()->MakeDeclaration(file_name, line_num, entry->Copy(), child);
}

/****************************
 * MethodCall class
 ****************************/
MethodCall::MethodCall(const wstring &f, const int l, MethodCallType t,
           const wstring &v, ExpressionList* e) :
  Statement(f, l), Expression(f, l) {
  variable_name = v;
  call_type = t;
  method_name = L"New";
  expressions = e;
  entry = dyn_func_entry = NULL;
  method = NULL;
  array_type = NULL;
  variable = NULL;
  enum_item = NULL;
  method = NULL;
  lib_method = NULL;
  lib_enum_item = NULL;
  original_klass = NULL;
  original_lib_klass = NULL;
  is_enum_call = is_func_def = is_dyn_func_call = false;
  func_rtrn = NULL;
  anonymous_klass = NULL;

  if(variable_name == BOOL_CLASS_ID) {
    array_type = TypeFactory::Instance()->MakeType(BOOLEAN_TYPE);
  }
  else if(variable_name == BYTE_CLASS_ID) {
    array_type = TypeFactory::Instance()->MakeType(BYTE_TYPE);
  }
  else if(variable_name == INT_CLASS_ID) {
    array_type = TypeFactory::Instance()->MakeType(INT_TYPE);
  }
  else if(variable_name == FLOAT_CLASS_ID) {
    array_type = TypeFactory::Instance()->MakeType(FLOAT_TYPE);
  }
  else if(variable_name == CHAR_CLASS_ID) {
    array_type = TypeFactory::Instance()->MakeType(CHAR_TYPE);
  }
  else if(variable_name == NIL_CLASS_ID) {
    array_type = TypeFactory::Instance()->MakeType(NIL_TYPE);
  }
  else if(variable_name == VAR_CLASS_ID) {
    array_type = TypeFactory::Instance()->MakeType(VAR_TYPE);
  }
  else {
    array_type = TypeFactory::Instance()->MakeType(CLASS_TYPE, variable_name);
  }
  array_type->SetDimension((int)expressions->GetExpressions().size());
  SetEvalType(array_type, false);
}

/****************************
 * ScopeTable class
 ****************************/
std::vector<SymbolEntry*> ScopeTable::GetEntries()
{
  vector<SymbolEntry*> entries_list;
  map<const wstring, SymbolEntry*>::iterator iter;
  for(iter = entries.begin(); iter != entries.end(); ++iter) {
    SymbolEntry* entry = iter->second;
    entries_list.push_back(entry);
  }

  return entries_list;
}

SymbolEntry* ScopeTable::GetEntry(const wstring& name)
{
  map<const wstring, SymbolEntry*>::iterator result = entries.find(name);
  if(result != entries.end()) {
    return result->second;
  }

  return NULL;
}

/****************************
 * SymbolTable class
 ****************************/
SymbolEntry* SymbolTable::GetEntry(const wstring& name)
{
  ScopeTable* tmp = iter_ptr;
  while(tmp) {
    SymbolEntry* entry = tmp->GetEntry(name);
    if(entry) {
      return entry;
    }
    tmp = tmp->GetParent();
  }

  return NULL;
}

bool SymbolTable::AddEntry(SymbolEntry* e, bool is_var /*= false*/)
{
  // see of we have this entry
  ScopeTable* tmp;
  if(is_var) {
    tmp = iter_ptr;
  }
  else {
    tmp = parse_ptr;
  }

  while(tmp) {
    SymbolEntry* entry = tmp->GetEntry(e->GetName());
    if(entry) {
      return false;
    }
    tmp = tmp->GetParent();
  }

  // add new entry
  if(is_var) {
    iter_ptr->AddEntry(e);
  }
  else {
    parse_ptr->AddEntry(e);
  }
  entries.push_back(e);
  return true;
}

/****************************
 * class Class
 ****************************/
void Class::AssociateMethods()
{
  for(size_t i = 0; i < method_list.size(); ++i) {
    Method* method = method_list[i];
    methods.insert(pair<wstring, Method*>(method->GetEncodedName(), method));

    // add to unqualified names to list
    const wstring& encoded_name = method->GetEncodedName();
    const size_t start = encoded_name.find(':');
    if(start != wstring::npos) {
      const size_t end = encoded_name.find(':', start + 1);
      if(end != wstring::npos) {
        const wstring& unqualified_name = encoded_name.substr(start + 1, end - start - 1);
        unqualified_methods.insert(pair<wstring, Method*>(unqualified_name, method));
      }
    }
  }
}
