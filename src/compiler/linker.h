/***************************************************************************
 * Links pre-compiled code into existing program.
 *
 * Copyright (c) 2008-2011, Randy Hollines
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
 * - Neither the name of the VM Team nor the names of its
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

#ifndef __LINKER_H__
#define __LINKER_H__

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "../shared/instrs.h"

using namespace std;

class Library;
class LibraryClass;
class LibraryEnum;

/********************************
 * LibraryInstr class.
 ********************************/
class LibraryInstr {
  instructions::InstructionType type;
  int operand;
  int operand2;
  int operand3;
  double operand4;
  string operand5;
  string operand6;
  int line_num;

public:
  LibraryInstr(int l, instructions::InstructionType t) {
    line_num = l;
    type = t;
    operand = 0;
  }

  LibraryInstr(int l, instructions::InstructionType t, int o) {
    line_num = l;
    type = t;
    operand = o;
  }

  LibraryInstr(int l, instructions::InstructionType t, double fo) {
    line_num = l;
    type = t;
    operand4 = fo;
    operand = 0;
  }

  LibraryInstr(int l, instructions::InstructionType t, int o, int o2) {
    line_num = l;
    type = t;
    operand = o;
    operand2 = o2;
  }

  LibraryInstr(int l, instructions::InstructionType t, int o, int o2, int o3) {
    line_num = l;
    type = t;
    operand = o;
    operand2 = o2;
    operand3 = o3;
  }

  LibraryInstr(int l, instructions::InstructionType t, string o5) {
    line_num = l;
    type = t;
    operand5 = o5;
  }

  LibraryInstr(int l, instructions::InstructionType t, int o3, string o5, string o6) {
    line_num = l;
    type = t;
    operand3 = o3;
    operand5 = o5;
    operand6 = o6;
  }

  ~LibraryInstr() {
  }

  instructions::InstructionType GetType() {
    return type;
  }

  int GetLineNumber() {
    return line_num;
  }
  
  void SetType(instructions::InstructionType t) {
    type = t;
  }

  int GetOperand() {
    return operand;
  }

  int GetOperand2() {
    return operand2;
  }

  int GetOperand3() {
    return operand3;
  }

  void SetOperand(int o) {
    operand = o;
  }

  void SetOperand2(int o2) {
    operand2 = o2;
  }

  void SetOperand3(int o3) {
    operand3 = o3;
  }

  double GetOperand4() {
    return operand4;
  }

  const string& GetOperand5() const {
    return operand5;
  }

  const string& GetOperand6() const {
    return operand6;
  }
};

/******************************
 * LibraryMethod class
 ****************************/
class LibraryMethod {
  int id;
  string name;
  string rtrn_name;
  frontend::Type* rtrn_type;
  vector<LibraryInstr*> instrs;
  LibraryClass* lib_cls;
  frontend::MethodType type;
  bool is_native;
  bool is_static;
  bool is_virtual;
  bool has_and_or;
  int num_params;
  int mem_size;
  backend::IntermediateDeclarations* entries;

public:
  LibraryMethod(int i, const string &n, const string &r, frontend::MethodType t, bool v,  bool h,
                bool nt, bool s, int p, int m, LibraryClass* c, backend::IntermediateDeclarations* e) {
    id = i;
    name = n;
    rtrn_name = r;
    type = t;
    is_virtual = v;
    has_and_or = h;
    is_native = nt;
    is_static = s;
    num_params = p;
    mem_size = m;
    lib_cls = c;
    entries = e;
    rtrn_type = NULL;

    // check return type
    unsigned int index = 0;
    switch(rtrn_name[index]) {
    case 'l':
      rtrn_type = frontend::TypeFactory::Instance()->MakeType(frontend::BOOLEAN_TYPE);
      index++;
      break;

    case 'b':
      rtrn_type = frontend::TypeFactory::Instance()->MakeType(frontend::BYTE_TYPE);
      index++;
      break;

    case 'i':
      rtrn_type = frontend::TypeFactory::Instance()->MakeType(frontend::INT_TYPE);
      index++;
      break;

    case 'f':
      rtrn_type = frontend::TypeFactory::Instance()->MakeType(frontend::FLOAT_TYPE);
      index++;
      break;

    case 'c':
      rtrn_type = frontend::TypeFactory::Instance()->MakeType(frontend::CHAR_TYPE);
      index++;
      break;

    case 'n':
      rtrn_type = frontend::TypeFactory::Instance()->MakeType(frontend::NIL_TYPE);
      index++;
      break;

    case 'm':
      rtrn_type = frontend::TypeFactory::Instance()->MakeType(frontend::FUNC_TYPE);
      index++;
      break;

    case 'o':
      index = 2;
      while(index < rtrn_name.size() && rtrn_name[index] != '*') {
        index++;
      }
      rtrn_type = frontend::TypeFactory::Instance()->MakeType(frontend::CLASS_TYPE,
                  rtrn_name.substr(2, index - 2));
      break;
    }

    // set dimension
    int dimension = 0;
    while(index < rtrn_name.size() && rtrn_name[index] == '*') {
      dimension++;
      index++;
    }
    rtrn_type->SetDimension(dimension);
  }

  ~LibraryMethod() {
    // clean up
    while(!instrs.empty()) {
      LibraryInstr* tmp = instrs.front();
      instrs.erase(instrs.begin());
      // delete
      delete tmp;
      tmp = NULL;
    }
  }

  int GetId() {
    return id;
  }

  bool IsVirtual() {
    return is_virtual;
  }

  bool HasAndOr() {
    return has_and_or;
  }

  const string& GetName() const {
    return name;
  }

  const string& GetEncodedReturn() const {
    return rtrn_name;
  }

  frontend::MethodType GetMethodType() {
    return type;
  }

  int GetSpace() {
    return mem_size;
  }

  int GetNumParams() {
    return num_params;
  }

  backend::IntermediateDeclarations* GetEntries() {
    return entries;
  }

  bool IsNative() {
    return is_native;
  }

  bool IsStatic() {
    return is_static;
  }

  LibraryClass* GetLibraryClass() {
    return lib_cls;
  }

  void AddInstructions(vector<LibraryInstr*> is) {
    instrs = is;
  }

  frontend::Type* GetReturn() {
    return rtrn_type;
  }

  vector<LibraryInstr*> GetInstructions() {
    return instrs;
  }
};

/****************************
 * LibraryEnumItem class
 ****************************/
class LibraryEnumItem {
  string name;
  int id;
  LibraryEnum* lib_eenum;

public:
  LibraryEnumItem(const string &n, int i, LibraryEnum* e) {
    name = n;
    id = i;
    lib_eenum = e;
  }

  ~LibraryEnumItem() {
  }

  const string& GetName() const {
    return name;
  }

  int GetId() {
    return id;
  }

  LibraryEnum* GetEnum() {
    return lib_eenum;
  }
};

/****************************
 * LibraryEnum class
 ****************************/
class LibraryEnum {
  string name;
  int offset;
  map<const string, LibraryEnumItem*> items;

public:
  LibraryEnum(const string &n, const int o) {
    name = n;
    offset = o;
  }

  ~LibraryEnum() {
    // clean up
    map<const string, LibraryEnumItem*>::iterator iter;
    for(iter = items.begin(); iter != items.end(); iter++) {
      LibraryEnumItem* tmp = iter->second;
      delete tmp;
      tmp = NULL;
    }
    items.clear();
  }

  const string& GetName() const {
    return name;
  }

  int GetOffset() {
    return offset;
  }

  void AddItem(LibraryEnumItem* i) {
    items.insert(pair<string, LibraryEnumItem*>(i->GetName(), i));
  }

  LibraryEnumItem* GetItem(const string &n) {
    map<const string, LibraryEnumItem*>::iterator result = items.find(n);
    if(result != items.end()) {
      return result->second;
    }

    return NULL;
  }

  map<const string, LibraryEnumItem*> GetItems() {
    return items;
  }
};

/****************************
 * LibraryClass class
 ****************************/
class LibraryClass {
  int id;
  string name;
  string parent_name;
  vector<string>interface_names;
  int cls_space;
  int inst_space;
  map<const string, LibraryMethod*> methods;
  backend::IntermediateDeclarations* entries;
  bool is_virtual;
  Library* library;
  vector<LibraryClass*> lib_children;
  vector<frontend::ParseNode*> children;
  bool was_called;
  bool is_debug;
  string file_name;
  
public:
  LibraryClass(const string &n, const string &p, vector<string> in, bool v, int cs, int is, 
	       backend::IntermediateDeclarations* e, Library* l, const string &fn, bool d) {
    name = n;
    parent_name = p;
    interface_names = in;
    is_virtual = v;
    cls_space = cs;
    inst_space = is;
    entries = e;
    library = l;
    if(name == "System.Class" || name == "System.Method" || name == "System.DataType") {
      was_called = true;
    }
    else {
      was_called = false;
    }
    is_debug = d;
    file_name = fn;
  }

  ~LibraryClass() {
    // clean up
    map<const string, LibraryMethod*>::iterator iter;
    for(iter = methods.begin(); iter != methods.end(); iter++) {
      LibraryMethod* tmp = iter->second;
      delete tmp;
      tmp = NULL;
    }
    methods.clear();

    /* deleted elsewhere
    if(entries) {
      delete entries;
      entries = NULL;
    }
    */
    
    lib_children.clear();
  }

#ifdef _UTILS
  void List() {
    map<const string, LibraryMethod*>::iterator iter;
    for(iter = methods.begin(); iter != methods.end(); iter++) {
      cout << "  method='" << iter->second->GetName() << "'" << endl;
    }
  }
#endif
  
  void SetId(int i) {
    id = i;
  }

  void SetCalled(bool c) {
    was_called = c;
  }

  bool GetCalled() {
    return was_called;
  }

  bool IsDebug() {
    return is_debug;
  }

  const string& GetFileName() const {
    return file_name;
  }
  
  int GetId() {
    return id;
  }

  const string& GetName() const {
    return name;
  }
  
  vector<string> GetInterfaceNames() {
    return interface_names;
  }

  bool IsInterface() {
    return interface_names.size() == 0;
  }

  const string& GetParentName() const {
    return parent_name;
  }

  vector<frontend::ParseNode*> GetChildren() {
    return children;
  }

  void AddChild(frontend::ParseNode* c) {
    children.push_back(c);
  }

  vector<LibraryClass*> GetLibraryChildren();

  bool IsVirtual() {
    return is_virtual;
  }

  int GetClassSpace() {
    return cls_space;
  }

  int GetInstanceSpace() {
    return inst_space;
  }

  backend::IntermediateDeclarations* GetEntries() {
    return entries;
  }

  LibraryMethod* GetMethod(const string &name) {
    map<const string, LibraryMethod*>::iterator result = methods.find(name);
    if(result != methods.end()) {
      return result->second;
    }

    return NULL;
  }

  map<const string, LibraryMethod*> GetMethods() {
    return methods;
  }

  void AddMethod(string name, LibraryMethod* mthd) {
    methods.insert(pair<const string, LibraryMethod*>(name, mthd));
  }
};

/******************************
 * Library class
 ****************************/
typedef struct _CharStringInstruction {
  string value;
  vector<LibraryInstr*> instrs;
} CharStringInstruction;

typedef struct _IntStringInstruction {
  frontend::IntStringHolder* value;
  vector<LibraryInstr*> instrs;
} IntStringInstruction;

typedef struct _FloatStringInstruction {
  frontend::FloatStringHolder* value;
  vector<LibraryInstr*> instrs;
} FloatStringInstruction;

class Library {
  string lib_path;
  char* buffer;
  char* alloc_buffer;
  std::streamoff buffer_size;
  long buffer_pos;
  map<const string, LibraryEnum*> enums;
  vector<LibraryEnum*> enum_list;
  map<const string, LibraryClass*> named_classes;
  vector<LibraryClass*> class_list;
  map<const string, const string> hierarchies;
  vector<CharStringInstruction*> char_strings;
  vector<IntStringInstruction*> int_strings;
  vector<FloatStringInstruction*> float_strings;
  vector<string> bundle_names;
  
  int ReadInt() {
    int value;
    memcpy(&value, buffer, sizeof(value));
    buffer += sizeof(value);
    return value;
  }

  int ReadByte() {
    char value;
    memcpy(&value, buffer, sizeof(value));
    buffer += sizeof(value);
    return value;
  }
  
  string ReadString() {
    int size = ReadInt();
    string value(buffer, size);
    buffer += size;
    return value;
  }

  double ReadDouble() {
    double value;
    memcpy(&value, buffer, sizeof(value));
    buffer += sizeof(value);
    return value;
  }

  // loads a file into memory
  char* LoadFileBuffer(string filename, std::streamoff& buffer_size) {
    char* buffer = NULL;
    // open file
    ifstream in(filename.c_str(), ifstream::binary);
    if(in) {
      // get file size
      in.seekg(0, ios::end);
      buffer_size = in.tellg();
      in.seekg(0, ios::beg);
      buffer = new char[buffer_size];
      in.read(buffer, buffer_size);
      // close file
      in.close();
    } else {
      cout << "Unable to open file: " << filename << endl;
      exit(1);
    }

    return buffer;
  }

  void ReadFile(const string &file) {
    buffer_pos = 0;
    alloc_buffer = buffer = LoadFileBuffer(file, buffer_size);
  }

  void AddEnum(LibraryEnum* e) {
    enums.insert(pair<string, LibraryEnum*>(e->GetName(), e));
    enum_list.push_back(e);
  }

  void AddClass(LibraryClass* cls) {
    named_classes.insert(pair<string, LibraryClass*>(cls->GetName(), cls));
    class_list.push_back(cls);
  }

  // loading functions
  void LoadFile(const string &file_name);
  void LoadEnums();
  void LoadClasses();
  void LoadMethods(LibraryClass* cls, bool is_debug);
  void LoadStatements(LibraryMethod* mthd, bool is_debug);

public:
  Library(const string &p) {
    lib_path = p;
    alloc_buffer = NULL;
  }

  ~Library() {
    // clean up
    map<const string, LibraryEnum*>::iterator enum_iter;
    for(enum_iter = enums.begin(); enum_iter != enums.end(); enum_iter++) {
      LibraryEnum* tmp = enum_iter->second;
      delete tmp;
      tmp = NULL;
    }
    enums.clear();
    enum_list.clear();

    map<const string, LibraryClass*>::iterator cls_iter;
    for(cls_iter = named_classes.begin(); cls_iter != named_classes.end(); cls_iter++) {
      LibraryClass* tmp = cls_iter->second;
      delete tmp;
      tmp = NULL;
    }
    named_classes.clear();
    class_list.clear();

    while(!char_strings.empty()) {
      CharStringInstruction* tmp = char_strings.front();
      char_strings.erase(char_strings.begin());
      // delete
      delete tmp;
      tmp = NULL;
    }

    while(!int_strings.empty()) {
      IntStringInstruction* tmp = int_strings.front();
      int_strings.erase(int_strings.begin());
      // delete
      delete tmp;
      tmp = NULL;
    }

    while(!float_strings.empty()) {
      FloatStringInstruction* tmp = float_strings.front();
      float_strings.erase(float_strings.begin());
      // delete
      delete tmp;
      tmp = NULL;
    }

    if(alloc_buffer) {
      delete[] alloc_buffer;
      alloc_buffer = NULL;
    }
  }

#ifdef _UTILS
  void List() {
    map<const string, LibraryClass*>::iterator cls_iter;
    for(cls_iter = named_classes.begin(); cls_iter != named_classes.end(); cls_iter++) {
      cout << "==================================" << endl;
      cout << "class='" << cls_iter->second->GetName() << "'" << endl;
      cout << "==================================" << endl;
      cls_iter->second->List();
    }
  }
#endif
  
  bool HasBundleName(const string& name) {
    vector<string>::iterator found = find(bundle_names.begin(), bundle_names.end(), name);
    return found != bundle_names.end();
  }

  LibraryClass* GetClass(const string &name) {
    map<const string, LibraryClass*>::iterator result = named_classes.find(name);
    if(result != named_classes.end()) {
      return result->second;
    }

    return NULL;
  }

  LibraryEnum* GetEnum(const string &name) {
    map<const string, LibraryEnum*>::iterator result = enums.find(name);
    if(result != enums.end()) {
      return result->second;
    }

    return NULL;
  }

  vector<LibraryEnum*> GetEnums() {
    return enum_list;
  }

  vector<LibraryClass*> GetClasses() {
    return class_list;
  }

  map<const string, const string> GetHierarchies() {
    return hierarchies;
  }

  vector<CharStringInstruction*> GetCharStringInstructions() {
    return char_strings;
  }

  vector<IntStringInstruction*> GetIntStringInstructions() {
    return int_strings;
  }
  
  vector<FloatStringInstruction*> GetFloatStringInstructions() {
    return float_strings;
  }

  void Load();
};

/********************************
 * Manages shared libraries
 ********************************/
class Linker {
  map<const string, Library*> libraries;
  string master_path;
  vector<string> paths;

public:
  static void Show(const string &msg, const int line_num, int depth) {
    cout << setw(4) << line_num << ": ";
    for(int i = 0; i < depth; i++) {
      cout << "  ";
    }
    cout << msg << endl;
  }

  static string ToString(int v) {
    ostringstream str;
    str << v;
    return str.str();
  }

public:
  Linker(const string& p) {
    master_path = p;
  }

  ~Linker() {
    // clean up
    map<const string, Library*>::iterator iter;
    for(iter = libraries.begin(); iter != libraries.end(); iter++) {
      Library* tmp = iter->second;
      delete tmp;
      tmp = NULL;
    }
    libraries.clear();

    paths.clear();
  }

  void ResloveExternalClass(LibraryClass* klass);
  void ResloveExternalClasses();
  void ResloveExternalMethodCalls();

  vector<string> GetLibraryPaths() {
    return paths;
  }

  Library* GetLibrary(const string &name) {
    map<const string, Library*>::iterator result = libraries.find(name);
    if(result != libraries.end()) {
      return result->second;
    }

    return NULL;
  }

  // get all libaries
  map<const string, Library*> GetAllLibraries() {
    return libraries;
  }

  // returns all classes including duplicates
  vector<LibraryClass*> GetAllClasses() {
    vector<LibraryClass*> all_libraries;

    map<const string, Library*>::iterator iter;
    for(iter = libraries.begin(); iter != libraries.end(); iter++) {
      vector<LibraryClass*> classes = iter->second->GetClasses();
      for(unsigned int i = 0; i < classes.size(); i++) {
        all_libraries.push_back(classes[i]);
      }
    }

    return all_libraries;
  }

  // returns all enums including duplicates
  vector<LibraryEnum*> GetAllEnums() {
    vector<LibraryEnum*> all_libraries;

    map<const string, Library*>::iterator iter;
    for(iter = libraries.begin(); iter != libraries.end(); iter++) {
      vector<LibraryEnum*> enums = iter->second->GetEnums();
      for(unsigned int i = 0; i < enums.size(); i++) {
        all_libraries.push_back(enums[i]);
      }
    }

    return all_libraries;
  }

  // TODO: finds the first class match; note multiple matches may exist
  LibraryClass* SearchClassLibraries(const string &name) {
    vector<LibraryClass*> classes = GetAllClasses();
    for(unsigned int i = 0; i < classes.size(); i++) {
      if(classes[i]->GetName() == name) {
        return classes[i];
      }
    }

    return NULL;
  }

  bool HasBundleName(const string& name) {
    map<const string, Library*>::iterator iter;
    for(iter = libraries.begin(); iter != libraries.end(); iter++) {
      if(iter->second->HasBundleName(name)) {
        return true;
      }
    }

    return false;
  }

  // TODO: finds the first class match; note multiple matches may exist
  LibraryClass* SearchClassLibraries(const string &name, vector<string> uses) {
    vector<LibraryClass*> classes = GetAllClasses();
    for(unsigned int i = 0; i < classes.size(); i++) {
      if(classes[i]->GetName() == name) {
        return classes[i];
      }
    }

    for(unsigned int i = 0; i < classes.size(); i++) {
      for(unsigned int j = 0; j < uses.size(); j++) {
        if(classes[i]->GetName() == uses[j] + "." + name) {
          return classes[i];
        }
      }
    }

    return NULL;
  }

  // TODO: finds the first enum match; note multiple matches may exist
  LibraryEnum* SearchEnumLibraries(const string &name, vector<string> uses) {
    vector<LibraryEnum*> enums = GetAllEnums();
    for(unsigned int i = 0; i < enums.size(); i++) {
      if(enums[i]->GetName() == name) {
        return enums[i];
      }
    }

    for(unsigned int i = 0; i < enums.size(); i++) {
      for(unsigned int j = 0; j < uses.size(); j++) {
        if(enums[i]->GetName() == uses[j] + "." + name) {
          return enums[i];
        }
      }
    }

    return NULL;
  }

  void Load() {
#ifdef _DEBUG
    cout << "--------- Linking Libraries ---------" << endl;
#endif
    // parses library path
    if(master_path.size() > 0) {
      size_t offset = 0;
      size_t index = master_path.find(',');
      while(index != string::npos) {
        // load library
        const string &file_name = master_path.substr(offset, index - offset);
        Library* library = new Library(file_name);
        library->Load();
        // insert library
        libraries.insert(pair<string, Library*>(file_name, library));
        vector<string>::iterator found = find(paths.begin(), paths.end(), file_name);
        if(found == paths.end()) {
          paths.push_back(file_name);
        }
        // update
        offset = index + 1;
        index = master_path.find(',', offset);
      }
      // insert library
      const string &file_name = master_path.substr(offset, master_path.size());
      Library* library = new Library(file_name);
      library->Load();
      libraries.insert(pair<string, Library*>(file_name, library));
      paths.push_back(file_name);
#ifdef _DEBUG
      cout << "--------- End Linking ---------" << endl;
#endif
    }

  }
};

#endif
