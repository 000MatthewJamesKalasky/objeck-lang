/***************************************************************************
 * Defines how the intermediate code is written to output files
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

#include "emit.h"

using namespace backend;

wstring backend::ReplaceSubstring(wstring s, const wstring& f, const wstring &r) {
  const size_t index = s.find(f);
  if(index != string::npos) {
    s.replace(index, f.size(), r);
  }
  
  return s;
}

/****************************
 * IntermediateFactory class
 ****************************/
IntermediateFactory* IntermediateFactory::instance;

IntermediateFactory* IntermediateFactory::Instance()
{
  if(!instance) {
    instance = new IntermediateFactory;
  }

  return instance;
}

/****************************
 * Writes target code to an output file.
 ****************************/
void FileEmitter::Emit()
{
#ifdef _DEBUG
  wcout << L"\n--------- Emitting Target Code ---------" << endl;
  program->Debug();
#endif

  // library target
  if(emit_lib) {
    if(!EndsWith(file_name, L".obl")) {
      wcerr << L"Error: Libraries must end in '.obl'" << endl;
      exit(1);
    }
  } 
  // web target
  else if(is_web) {
    if(!EndsWith(file_name, L".obw")) {
      wcerr << L"Error: Web applications must end in '.obw'" << endl;
      exit(1);
    }
  } 
  // application target
  else {
    if(!EndsWith(file_name, L".obe")) {
      wcerr << L"Error: Executables must end in '.obe'" << endl;
      exit(1);
    }
  }
  
  const string open_filename(file_name.begin(), file_name.end());
  ofstream file_out(open_filename.c_str(), ofstream::binary);
  if(file_out && file_out.is_open()) {
    program->Write(emit_lib, is_debug, is_web, file_out);
    file_out.close();
    wcout << L"Wrote target file: '" << file_name << L"'" << endl;
  }
  else {
    wcerr << L"Unable to write file: '" << file_name << L"'" << endl;
  }
}

/****************************
 * IntermediateProgram class
 ****************************/
void IntermediateProgram::Write(bool emit_lib, bool is_debug, bool is_web, ofstream &file_out) {
  // version
  WriteInt(VER_NUM, file_out);

  // magic number
  if(emit_lib) {
    WriteInt(MAGIC_NUM_LIB, file_out);
  }
  else if(is_web) {
    WriteInt(MAGIC_NUM_WEB, file_out);
  }
  else {
    WriteInt(MAGIC_NUM_EXE, file_out);
  }

  // write wstring id
  if(!emit_lib) {
#ifdef _DEBUG
    assert(string_cls_id > 0);
#endif
    WriteInt(string_cls_id, file_out);
  }

  // write float strings
  WriteInt((int)float_strings.size(), file_out);
  for(size_t i = 0; i < float_strings.size(); ++i) {
    frontend::FloatStringHolder* holder = float_strings[i];
    WriteInt(holder->length, file_out);
    for(int j = 0; j < holder->length; ++j) {
      WriteDouble(holder->value[j], file_out);
    }
  }
  
  // write int strings
  WriteInt((int)int_strings.size(), file_out);
  for(size_t i = 0; i < int_strings.size(); ++i) {
    frontend::IntStringHolder* holder = int_strings[i];
    WriteInt(holder->length, file_out);
    for(int j = 0; j < holder->length; ++j) {
      WriteInt(holder->value[j], file_out);
    }
  }
  
  // write char strings
  WriteInt((int)char_strings.size(), file_out);
  for(size_t i = 0; i < char_strings.size(); ++i) {
    WriteString(char_strings[i], file_out);
  }

  // write bundle names
  if(emit_lib) {
    WriteInt((int)bundle_names.size(), file_out);
    for(size_t i = 0; i < bundle_names.size(); ++i) {
      WriteString(bundle_names[i], file_out);
    }
  }

  // program start
  if(!emit_lib) {
    WriteInt(class_id, file_out);
    WriteInt(method_id, file_out);
  }
  
  // program enums
  if(emit_lib) {
    WriteInt((int)enums.size(), file_out);
    for(size_t i = 0; i < enums.size(); ++i) {
      enums[i]->Write(file_out);
    }
  }
  
  // program classes
  WriteInt((int)classes.size(), file_out);
  for(size_t i = 0; i < classes.size(); ++i) {
    if(classes[i]->IsLibrary()) {
      num_lib_classes++;
    }
    else {
      num_src_classes++;
    }
    classes[i]->Write(emit_lib, file_out);
  }
  
  wcout << L"Compiled " << num_src_classes
        << (num_src_classes > 1 ? " source classes" : " source class");
  if(is_debug) {
    wcout << " with debug symbols";
  }
  wcout << L'.' << endl;
  
  wcout << L"Linked " << num_lib_classes
        << (num_lib_classes > 1 ? " library classes." : " library class.") << endl;
}

/****************************
 * Class class
 ****************************/
void IntermediateClass::Write(bool emit_lib, ofstream &file_out) {
  // write id and name
  WriteInt(id, file_out);
  WriteString(name, file_out);
  WriteInt(pid, file_out);
  WriteString(parent_name, file_out);

  // interface ids
  WriteInt(interface_ids.size(), file_out);
  for(size_t i = 0; i < interface_ids.size(); ++i) {
    WriteInt(interface_ids[i], file_out);
  }

  // interface names
  if(emit_lib) {
    WriteInt(interface_names.size(), file_out);
    for(size_t i = 0; i < interface_names.size(); ++i) {
      WriteString(interface_names[i], file_out);
    }
    WriteInt(is_interface, file_out);
  }
  
  WriteInt(is_virtual, file_out);
  WriteInt(is_debug, file_out);
  if(is_debug) {
    WriteString(file_name, file_out);
  }

  // write local space size
  WriteInt(cls_space, file_out);
  WriteInt(inst_space, file_out);
  cls_entries->Write(is_debug, file_out);
  inst_entries->Write(is_debug, file_out);

  // write methods
  WriteInt((int)methods.size(), file_out);
  for(size_t i = 0; i < methods.size(); ++i) {
    methods[i]->Write(emit_lib, is_debug, file_out);
  }
}

/****************************
 * Method class
 **************************/
void IntermediateMethod::Write(bool emit_lib, bool is_debug, ofstream &file_out) {
  // write attributes
  WriteInt(id, file_out);
  
  if(emit_lib) {
    WriteInt(type, file_out);
  }
  
  WriteInt(is_virtual, file_out);
  WriteInt(has_and_or, file_out);
  
  if(emit_lib) {
    WriteInt(is_native, file_out);
    WriteInt(is_function, file_out);
  }
  
  WriteString(name, file_out);
  WriteString(rtrn_name, file_out);

  // write local space size
  WriteInt(params, file_out);
  WriteInt(space, file_out);
  entries->Write(is_debug, file_out);

  // write statements
  uint32_t num_instrs = 0;
  for(size_t i = 0; i < blocks.size(); ++i) {
    num_instrs += blocks[i]->GetInstructions().size();
  }
  WriteUnsigned(num_instrs, file_out);

  for(size_t i = 0; i < blocks.size(); ++i) {
    blocks[i]->Write(is_debug, file_out);
  }
}

/****************************
* IntermediateBlock class
****************************/
void IntermediateBlock::Write(bool is_debug, ofstream &file_out) {
  for(size_t i = 0; i < instructions.size(); ++i) {
    instructions[i]->Write(is_debug, file_out);
  }
}

/****************************
 * Instruction class
 ****************************/
void IntermediateInstruction::Write(bool is_debug, ofstream &file_out) {
  WriteByte((int)type, file_out);
  if(is_debug) {
    WriteInt(line_num, file_out);
  }
  switch(type) {
  case LOAD_INT_LIT:

  case NEW_FLOAT_ARY:
  case NEW_INT_ARY:
  case NEW_BYTE_ARY:
  case NEW_CHAR_ARY:
  case NEW_OBJ_INST:
  case OBJ_INST_CAST:
  case OBJ_TYPE_OF:
  case TRAP:
  case TRAP_RTRN:
    WriteInt(operand, file_out);
    break;

  case LOAD_CHAR_LIT:
    WriteChar(operand, file_out);
    break;

  case instructions::ASYNC_MTHD_CALL:
  case MTHD_CALL:
    WriteInt(operand, file_out);
    WriteInt(operand2, file_out);
    WriteInt(operand3, file_out);
    break;

  case LIB_NEW_OBJ_INST:
  case LIB_OBJ_INST_CAST:
  case LIB_OBJ_TYPE_OF:
    WriteString(operand5, file_out);
    break;

  case LIB_MTHD_CALL:
    WriteInt(operand3, file_out);
    WriteString(operand5, file_out);
    WriteString(operand6, file_out);
    break;

  case LIB_FUNC_DEF:
    WriteString(operand5, file_out);
    WriteString(operand6, file_out);
    break;

  case JMP:
  case DYN_MTHD_CALL:
  case LOAD_INT_VAR:
  case LOAD_FLOAT_VAR:
  case LOAD_FUNC_VAR:
  case STOR_INT_VAR:
  case STOR_FLOAT_VAR:
  case STOR_FUNC_VAR:
  case COPY_INT_VAR:
  case COPY_FLOAT_VAR:
  case COPY_FUNC_VAR:
  case LOAD_BYTE_ARY_ELM:
  case LOAD_CHAR_ARY_ELM:
  case LOAD_INT_ARY_ELM:
  case LOAD_FLOAT_ARY_ELM:
  case STOR_BYTE_ARY_ELM:
  case STOR_CHAR_ARY_ELM:
  case STOR_INT_ARY_ELM:
  case STOR_FLOAT_ARY_ELM:
    WriteInt(operand, file_out);
    WriteInt(operand2, file_out);
    break;

  case LOAD_FLOAT_LIT:
    WriteDouble(operand4, file_out);
    break;

  case LBL:
    WriteInt(operand, file_out);
    break;

  default:
    break;
  }
}

/****************************
 * IntermediateEnumItem class
 ****************************/
void IntermediateEnumItem::Write(ofstream &file_out) {
  WriteString(name, file_out);
  WriteInt(id, file_out);
}

/****************************
 * Enum class
 ****************************/
void IntermediateEnum::Write(ofstream &file_out) {
  WriteString(name, file_out);
  WriteInt(offset, file_out);
  // write items
  WriteInt((int)items.size(), file_out);
  for(size_t i = 0; i < items.size(); ++i) {
    items[i]->Write(file_out);
  }
}
