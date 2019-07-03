/***************************************************************************
* Defines how the intermediate code is written to output files
*
* Copyright (c) 2008-2019, Randy Hollines
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

#ifndef __FILE_TARGET_H__
#define __FILE_TARGET_H__

#include "types.h"
#include "linker.h"
#include "tree.h"
#include "../shared/instrs.h"
#include "../shared/version.h"

using namespace std;
using namespace instructions;

namespace backend {
  class IntermediateClass;
  
  wstring ReplaceSubstring(wstring s, const wstring &f, const wstring &r);
  
  /****************************
   * Intermediate class
   ****************************/
  class Intermediate {

  public:
    Intermediate() {
    }

    virtual ~Intermediate() {
    }

  protected:
    inline void WriteString(const wstring &in, OutputStream& out_stream) {
      out_stream.WriteString(in);
    }

    inline void WriteByte(char value, OutputStream& out_stream) {
      out_stream.WriteByte(value);
    }

    inline void WriteInt(int32_t value, OutputStream& out_stream) {
      out_stream.WriteInt(value);
    }

    inline void WriteUnsigned(uint32_t value, OutputStream& out_stream) {
      out_stream.WriteUnsigned(value);
    }

    inline void WriteChar(wchar_t value, OutputStream& out_stream) {
      out_stream.WriteChar(value);
    }

    inline void WriteDouble(FLOAT_VALUE value, OutputStream& out_stream) {
      out_stream.WriteDouble(value);
    }
  };

  /****************************
  * IntermediateInstruction class
  ****************************/
  class IntermediateInstruction : public Intermediate {
    friend class IntermediateFactory;
    InstructionType type;
    int operand;
    int operand2;
    int operand3;
    FLOAT_VALUE operand4;
    wstring operand5;
    wstring operand6;
    int line_num;

    IntermediateInstruction(int l, InstructionType t) {
      line_num = l;
      type = t;
    }

    IntermediateInstruction(int l, InstructionType t, int o1) {
      line_num = l;
      type = t;
      operand = o1;
    }

    IntermediateInstruction(int l, InstructionType t, int o1, int o2) {
      line_num = l;
      type = t;
      operand = o1;
      operand2 = o2;
    }

    IntermediateInstruction(int l, InstructionType t, int o1, int o2, int o3) {
      line_num = l;
      type = t;
      operand = o1;
      operand2 = o2;
      operand3 = o3;
    }

    IntermediateInstruction(int l, InstructionType t, FLOAT_VALUE o4) {
      line_num = l;
      type = t;
      operand4 = o4;
    }

    IntermediateInstruction(int l, InstructionType t, wstring o5) {
      line_num = l;
      type = t;
      operand5 = o5;
    }

    IntermediateInstruction(int l, InstructionType t, int o3, wstring o5, wstring o6) {
      line_num = l;
      type = t;
      operand3 = o3;
      operand5 = o5;
      operand6 = o6;
    }

    IntermediateInstruction(LibraryInstr* lib_instr) {
      type = lib_instr->GetType();
      line_num = lib_instr->GetLineNumber();
      operand = lib_instr->GetOperand();
      operand2 = lib_instr->GetOperand2();
      operand3 = lib_instr->GetOperand3();
      operand4 = lib_instr->GetOperand4();
      operand5 = lib_instr->GetOperand5();
      operand6 = lib_instr->GetOperand6();
    }

    ~IntermediateInstruction() {
    }

  public:
    InstructionType GetType() {
      return type;
    }

    int GetOperand() {
      return operand;
    }

    int GetOperand2() {
      return operand2;
    }

    FLOAT_VALUE GetOperand4() {
      return operand4;
    }

    void SetOperand3(int o3) {
      operand3 = o3;
    }

#ifdef _DEBUG
    void Debug();
#endif

    void Write(bool is_debug, OutputStream& out_stream);
  };

  /****************************
   * IntermediateFactory class
   ****************************/
  class IntermediateFactory {
    static IntermediateFactory* instance;
    vector<IntermediateInstruction*> instructions;

  public:
    static IntermediateFactory* Instance();

    void Clear() {
      while(!instructions.empty()) {
        IntermediateInstruction* tmp = instructions.front();
        instructions.erase(instructions.begin());
        // delete
        delete tmp;
        tmp = nullptr;
      }

      delete instance;
      instance = nullptr;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t, int o1) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t, o1);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t, int o1, int o2) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t, o1, o2);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t, int o1, int o2, int o3) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t, o1, o2, o3);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t, FLOAT_VALUE o4) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t, o4);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t, wstring o5) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t, o5);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(int l, InstructionType t, int o3, wstring o5, wstring o6) {
      IntermediateInstruction* tmp = new IntermediateInstruction(l, t, o3, o5, o6);
      instructions.push_back(tmp);
      return tmp;
    }

    IntermediateInstruction* MakeInstruction(LibraryInstr* lib_instr) {
      IntermediateInstruction* tmp = new IntermediateInstruction(lib_instr);
      instructions.push_back(tmp);
      return tmp;
    }
  };

  /****************************
   * Block class
   ****************************/
  class IntermediateBlock : public Intermediate {
    vector<IntermediateInstruction*> instructions;

  public:
    IntermediateBlock() {
    }

    ~IntermediateBlock() {
    }

    void AddInstruction(IntermediateInstruction* i) {
      instructions.push_back(i);
    }

    vector<IntermediateInstruction*> GetInstructions() {
      return instructions;
    }

    int GetSize() {
      return (int)instructions.size();
    }

    void Clear() {
      instructions.clear();
    }

    bool IsEmpty() {
      return instructions.size() == 0;
    }

    void Write(bool is_debug, OutputStream& out_stream);

#ifdef _DEBUG
    void Debug() {
      if(instructions.size() > 0) {
        for(size_t i = 0; i < instructions.size(); ++i) {
          instructions[i]->Debug();
        }
        GetLogger() << L"--" << endl;
      }
    }
#endif
  };

  /****************************
   * Method class
   **************************/
  class IntermediateMethod : public Intermediate {
    int id;
    wstring name;
    wstring rtrn_name;
    int space;
    int params;
    frontend::MethodType type;
    bool is_native;
    bool is_function;
    bool is_lib;
    bool is_virtual;
    bool has_and_or;
    bool is_lambda;
    int instr_count;
    vector<IntermediateBlock*> blocks;
    IntermediateDeclarations* entries;
    IntermediateClass* klass;

  public:
    IntermediateMethod(int i, const wstring &n, bool v, bool h, bool l, const wstring &r,
                       frontend::MethodType t, bool nt, bool f, int c, int p,
                       IntermediateDeclarations* e, IntermediateClass* k) {
      id = i;
      name = n;
      is_virtual = v;
      has_and_or = h;
      is_lambda = l;
      rtrn_name = r;
      type = t;
      is_native = nt;
      is_function = f;
      space = c;
      params = p;
      entries = e;
      is_lib = false;
      klass = k;
      instr_count = 0;
    }

    IntermediateMethod(LibraryMethod* lib_method, IntermediateClass* k) {
      // set attributes
      id = lib_method->GetId();
      name = lib_method->GetName();
      is_virtual = lib_method->IsVirtual();
      has_and_or = lib_method->HasAndOr();
      is_lambda = lib_method->IsLambda();
      rtrn_name = lib_method->GetEncodedReturn();
      type = lib_method->GetMethodType();
      is_native = lib_method->IsNative();
      is_function = lib_method->IsStatic();
      space = lib_method->GetSpace();
      params = lib_method->GetNumParams();
      entries = lib_method->GetEntries();
      is_lib = true;
      instr_count = 0;
      klass = k;
      
      // process instructions
      IntermediateBlock* block = new IntermediateBlock;
      vector<LibraryInstr*> lib_instructions = lib_method->GetInstructions();
      for(size_t i = 0; i < lib_instructions.size(); ++i) {
        block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(lib_instructions[i]));
      }
      AddBlock(block);
    }

    ~IntermediateMethod() {
      // clean up
      while(!blocks.empty()) {
        IntermediateBlock* tmp = blocks.front();
        blocks.erase(blocks.begin());
        // delete
        delete tmp;
        tmp = nullptr;
      }

      if(entries) {
        delete entries;
        entries = nullptr;
      }
    }

    int GetId() {
      return id;
    }
    
    IntermediateDeclarations* GetEntries() {
      return entries;
    }
    
    IntermediateClass* GetClass() {
      return klass;
    }

    int GetSpace() {
      return space;
    }

    void SetSpace(int s) {
      space = s;
    }

    wstring GetName() {
      return name;
    }

    bool IsVirtual() {
      return is_virtual;
    }

    bool IsLibrary() {
      return is_lib;
    }

    void AddBlock(IntermediateBlock* b) {
      instr_count += b->GetSize();
      blocks.push_back(b);
    }

    int GetInstructionCount() {
      return instr_count;
    }

    bool HasAndOr() {
      return has_and_or;
    }

    int GetNumParams() {
      return params;
    }

    vector<IntermediateBlock*> GetBlocks() {
      return blocks;
    }

    void SetBlocks(vector<IntermediateBlock*> b) {
      blocks = b;
    }

    void Write(bool emit_lib, bool is_debug, OutputStream& out_stream);

#ifdef _DEBUG
    void Debug() {
      GetLogger() << L"---------------------------------------------------------" << endl;
      GetLogger() << L"Method: id=" << id << L"; name='" << name << L"'; return='" << rtrn_name
        << L"';\n  blocks=" << blocks.size() << L"; is_function=" << is_function << L"; num_params="
        << params << L"; mem_size=" << space << endl;
      GetLogger() << L"---------------------------------------------------------" << endl;
      entries->Debug(has_and_or);
      GetLogger() << L"---------------------------------------------------------" << endl;
      for(size_t i = 0; i < blocks.size(); ++i) {
        blocks[i]->Debug();
      }
    }
#endif
  };

  /****************************
   * Class class
   ****************************/
  class IntermediateClass : public Intermediate {
    int id;
    wstring name;
    int pid;
    vector<int> interface_ids;
    wstring parent_name;
    vector<wstring> interface_names;
    vector<wstring> generic_classes;
    int cls_space;
    int inst_space;
    vector<IntermediateBlock*> blocks;
    vector<IntermediateMethod*> methods;
    map<int, IntermediateMethod*> method_map;
    IntermediateDeclarations* cls_entries;
    IntermediateDeclarations* inst_entries;
    map<IntermediateDeclarations*, pair<wstring, int> > closure_entries;
    bool is_lib;
    bool is_interface;
    bool is_virtual;
    bool is_debug;
    wstring file_name;
    
  public:
    IntermediateClass(int i, const wstring &n, int pi, const wstring &p, 
                      vector<int> infs, vector<wstring> in, bool is_inf,
                      vector<wstring> gen, bool is_vrtl, int cs, int is,
                      IntermediateDeclarations* ce, IntermediateDeclarations* ie, 
                      const wstring &fn, bool d) {
      id = i;
      name = n;
      pid = pi;
      parent_name = p;
      interface_ids = infs;
      interface_names = in;
      generic_classes = gen;
      is_interface = is_inf;
      is_virtual = is_vrtl;
      cls_space = cs;
      inst_space = is;
      cls_entries = ce;
      inst_entries = ie;
      is_lib = false;
      is_debug = d;
      file_name = fn;
    }
    
    IntermediateClass(LibraryClass* lib_klass, int pi) {
      // set attributes
      id = lib_klass->GetId();
      name = lib_klass->GetName();
      pid = pi;
      interface_ids = lib_klass->GetInterfaceIds();
      parent_name = lib_klass->GetParentName();
      interface_names = lib_klass->GetInterfaceNames();
      is_interface = lib_klass->IsInterface();
      is_virtual = lib_klass->IsVirtual();
      is_debug = lib_klass->IsDebug();
      cls_space = lib_klass->GetClassSpace();
      inst_space = lib_klass->GetInstanceSpace();
      cls_entries = lib_klass->GetClassEntries();
      inst_entries = lib_klass->GetInstanceEntries();
      closure_entries = lib_klass->GetLambaEntries();

      // process methods
      map<const wstring, LibraryMethod*> lib_methods = lib_klass->GetMethods();
      map<const wstring, LibraryMethod*>::iterator mthd_iter;
      for(mthd_iter = lib_methods.begin(); mthd_iter != lib_methods.end(); ++mthd_iter) {
        LibraryMethod* lib_method = mthd_iter->second;
        IntermediateMethod* imm_method = new IntermediateMethod(lib_method, this);
        AddMethod(imm_method);
      }

      file_name = lib_klass->GetFileName();
      is_lib = true;
    }

    ~IntermediateClass() {
      // clean up
      while(!blocks.empty()) {
        IntermediateBlock* tmp = blocks.front();
        blocks.erase(blocks.begin());
        // delete
        delete tmp;
        tmp = nullptr;
      }
      
      // clean up
      while(!methods.empty()) {
        IntermediateMethod* tmp = methods.front();
        methods.erase(methods.begin());
        // delete
        delete tmp;
        tmp = nullptr;
      }

      // clean up
      if(cls_entries) {
        delete cls_entries;
        cls_entries = nullptr;
      }

      if(inst_entries) {
        delete inst_entries;
        inst_entries = nullptr;
      }

      map<IntermediateDeclarations*, pair<wstring, int> >::iterator entries_iter;
      for(entries_iter = closure_entries.begin(); entries_iter != closure_entries.end(); ++entries_iter) {
        IntermediateDeclarations* tmp = entries_iter->first;
        delete tmp;
        tmp = nullptr;
      }
      closure_entries.clear();
    }

    int GetId() {
      return id;
    }

    const wstring &GetName() {
      return name;
    }

    bool IsLibrary() {
      return is_lib;
    }

    int GetInstanceSpace() {
      return inst_space;
    }

    void SetInstanceSpace(int s) {
      inst_space = s;
    }

    int GetClassSpace() {
      return cls_space;
    }

    void SetClassSpace(int s) {
      cls_space = s;
    }
    
    void AddMethod(IntermediateMethod* m) {
      methods.push_back(m);
      method_map.insert(pair<int, IntermediateMethod*>(m->GetId(), m));
    }

    void AddBlock(IntermediateBlock* b) {
      blocks.push_back(b);
    }

    IntermediateMethod* GetMethod(int id) {
      map<int, IntermediateMethod*>::iterator result = method_map.find(id);
#ifdef _DEBUG
      assert(result != method_map.end());
#endif
      return result->second;
    }

    vector<IntermediateMethod*> GetMethods() {
      return methods;
    }

    void AddClosureDeclarations(const wstring mthd_name, const int mthd_id, IntermediateDeclarations* dclrs) {
      closure_entries[dclrs] = pair<wstring, int>(mthd_name, mthd_id);
    }

    void Write(bool emit_lib, OutputStream& out_stream);
    
#ifdef _DEBUG
    void Debug() {
      GetLogger() << L"=========================================================" << endl;
      GetLogger() << L"Class: id=" << id << L"; name='" << name << L"'; parent='" << parent_name
            << L"'; pid=" << pid << L";\n interface=" << (is_interface ? L"true" : L"false") 
            << L"; virtual=" << is_virtual << L"; num_generics=" << generic_classes.size() 
            << L"; num_methods=" << methods.size() << L"; class_mem_size=" << cls_space 
            << L";\n instance_mem_size=" << inst_space << L"; is_debug=" 
            << (is_debug  ? L"true" : L"false") << endl;      
      GetLogger() << endl << "Interfaces:" << endl;
      for(size_t i = 0; i < interface_names.size(); ++i) {
        GetLogger() << L"\t" << interface_names[i] << endl;
      }      
      GetLogger() << L"=========================================================" << endl;
      cls_entries->Debug(false);
      GetLogger() << L"---------------------------------------------------------" << endl;
      inst_entries->Debug(false);
      GetLogger() << L"=========================================================" << endl;
      for(size_t i = 0; i < blocks.size(); ++i) {
        blocks[i]->Debug();
      }

      for(size_t i = 0; i < methods.size(); ++i) {
        methods[i]->Debug();
      }
    }
#endif
  };

  /****************************
   * EnumItem class
   ****************************/
  class IntermediateEnumItem : public Intermediate {
    wstring name;
    INT_VALUE id;

  public:
    IntermediateEnumItem(const wstring &n, const INT_VALUE i) {
      name = n;
      id = i;
    }

    IntermediateEnumItem(LibraryEnumItem* i) {
      name = i->GetName();
      id = i->GetId();
    }

    void Write(OutputStream& out_stream);

#ifdef _DEBUG
    void Debug() {
      GetLogger() << L"Item: name='" << name << L"'; id='" << id << endl;
    }
#endif
  };

  /****************************
   * Enum class
   ****************************/
  class IntermediateEnum : public Intermediate {
    wstring name;
    INT_VALUE offset;
    vector<IntermediateEnumItem*> items;

  public:
    IntermediateEnum(const wstring &n, const INT_VALUE o) {
      name = n;
      offset = o;
    }

    IntermediateEnum(LibraryEnum* e) {
      name = e->GetName();
      offset = e->GetOffset();
      // write items
      map<const wstring, LibraryEnumItem*> items = e->GetItems();
      map<const wstring, LibraryEnumItem*>::iterator iter;
      for(iter = items.begin(); iter != items.end(); ++iter) {
        LibraryEnumItem* lib_enum_item = iter->second;
        IntermediateEnumItem* imm_enum_item = new IntermediateEnumItem(lib_enum_item);
        AddItem(imm_enum_item);
      }
    }

    ~IntermediateEnum() {
      while(!items.empty()) {
        IntermediateEnumItem* tmp = items.front();
        items.erase(items.begin());
        // delete
        delete tmp;
        tmp = nullptr;
      }
    }

    void AddItem(IntermediateEnumItem* i) {
      items.push_back(i);
    }

    void Write(OutputStream& out_stream);

#ifdef _DEBUG
    void Debug() {
      GetLogger() << L"=========================================================" << endl;
      GetLogger() << L"Enum: name='" << name << L"'; items=" << items.size() << endl;
      GetLogger() << L"=========================================================" << endl;

      for(size_t i = 0; i < items.size(); ++i) {
        items[i]->Debug();
      }
    }
#endif
  };

  /****************************
   * Program class
   ****************************/
  class IntermediateProgram : public Intermediate {
    int class_id;
    int method_id;
    vector<wstring> alias_encodings;
    vector<IntermediateEnum*> enums;
    vector<IntermediateClass*> classes;
    map<int, IntermediateClass*> class_map;
    vector<wstring> char_strings;
    vector<frontend::IntStringHolder*> int_strings;
    vector<frontend::FloatStringHolder*> float_strings;
    vector<wstring> bundle_names;
    wstring aliases_str;
    int num_src_classes;
    int num_lib_classes;
    int string_cls_id;

  public:
    IntermediateProgram() {
      num_src_classes = num_lib_classes = 0;
      string_cls_id = -1;
    }

    ~IntermediateProgram() {
      // clean up
      while(!enums.empty()) {
        IntermediateEnum* tmp = enums.front();
        enums.erase(enums.begin());
        // delete
        delete tmp;
        tmp = nullptr;
      }

      while(!classes.empty()) {
        IntermediateClass* tmp = classes.front();
        classes.erase(classes.begin());
        // delete
        delete tmp;
        tmp = nullptr;
      }

      while(!int_strings.empty()) {
        frontend::IntStringHolder* tmp = int_strings.front();
        delete[] tmp->value;
        tmp->value = nullptr;
        int_strings.erase(int_strings.begin());
        // delete
        delete tmp;
        tmp = nullptr;
      }

      while(!float_strings.empty()) {
        frontend::FloatStringHolder* tmp = float_strings.front();
        delete[] tmp->value;
        tmp->value = nullptr;
        float_strings.erase(float_strings.begin());
        // delete
        delete tmp;
        tmp = nullptr;
      }

      IntermediateFactory::Instance()->Clear();
    }

    void AddClass(IntermediateClass* c) {
      classes.push_back(c);
      class_map.insert(pair<int, IntermediateClass*>(c->GetId(), c));
    }

    IntermediateClass* GetClass(int id) {
      map<int, IntermediateClass*>::iterator result = class_map.find(id);
#ifdef _DEBUG
      assert(result != class_map.end());
#endif
      return result->second;
    }

    void AddAliasEncoding(const wstring &a) {
      alias_encodings.push_back(a);
    }

    void AddEnum(IntermediateEnum* e) {
      enums.push_back(e);
    }

    vector<IntermediateClass*> GetClasses() {
      return classes;
    }

    void SetCharStrings(vector<wstring> s) {
      char_strings = s;
    }

    void SetIntStrings(vector<frontend::IntStringHolder*> s) {
      int_strings = s;
    }

    void SetFloatStrings(vector<frontend::FloatStringHolder*> s) {
      float_strings = s;
    }

    void SetStartIds(int c, int m) {
      class_id = c;
      method_id = m;
    }

    int GetStartClassId() {
      return class_id;
    }

    int GetStartMethodId() {
      return method_id;
    }

    void SetBundleNames(vector<wstring> &n) {
      bundle_names = n;
    }

    void SetStringClassId(int i) {
      string_cls_id = i;
    }

    void SetAliasesString(const wstring &a) {
      aliases_str = a;
    }

    const wstring GetAliasesString() {
      return aliases_str;
    }

    void Write(bool emit_lib, bool is_debug, bool is_web, OutputStream& out_stream);

#ifdef _DEBUG
    void Debug() {
      GetLogger() << L"Strings:" << endl;
      for(size_t i = 0; i < char_strings.size(); ++i) {
        GetLogger() << L"wstring id=" << i << L", size='" << ToString((int)char_strings[i].size())
              << L"': '" << char_strings[i] << L"'" << endl;
      }
      GetLogger() << endl;

      GetLogger() << L"Program: enums=" << enums.size() << L", classes="
            << classes.size() << L"; start=" << class_id << L"," << method_id << endl;
      // enums
      for(size_t i = 0; i < enums.size(); ++i) {
        enums[i]->Debug();
      }
      // classes
      for(size_t i = 0; i < classes.size(); ++i) {
        classes[i]->Debug();
      }
    }
#endif

    inline wstring ToString(int v) {
      wostringstream str;
      str << v;
      return str.str();
    }
  };

  /****************************
   * FileEmitter class
   ****************************/
  class FileEmitter {
    IntermediateProgram* program;
    wstring file_name;
    bool emit_lib;
    bool is_debug;
    bool is_web;

    bool EndsWith(wstring const &str, wstring const &ending) {
      if(str.length() >= ending.length()) {
        return str.compare(str.length() - ending.length(), ending.length(), ending) == 0;
      } 

      return false;
    }

  public:
    FileEmitter(IntermediateProgram* p, bool l, bool d, bool w, const wstring &n) {
      program = p;
      emit_lib = l;
      is_debug = d;
      is_web = w;
      file_name = n;
    }

    ~FileEmitter() {
      delete program;
      program = nullptr;
    }

    void Emit();
  };
}

#endif
