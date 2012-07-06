/***************************************************************************
 * VM stack machine.
 *
 * Copyright (c) 2008-2012, Randy Hollines
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

#ifndef __STACK_INTPR_H__
#define __STACK_INTPR_H__

#include "common.h"
#include <string.h>

#ifdef _WIN32
#include "os/windows/memory.h"
#else
#include "os/posix/memory.h"
#endif

#ifdef _DEBUGGER
#include "debugger/debugger.h"
#endif

using namespace std;

namespace Runtime {
#ifdef _DEBUGGER
  class Debugger;
#endif
  
#define CALC_STACK_SIZE 1048576 * 4
#define CALL_STACK_SIZE 1024
  
  enum TimeInterval {
    DAY_TIME,
    HOUR_TIME,
    MIN_TIME,
    SEC_TIME
  };
  
  struct ThreadHolder {
    StackMethod* called;
    long* param;
  };

  class StackInterpreter {
    // program
    static StackProgram* program;
    // call stack and current frame pointer
    StackFrame* call_stack[CALL_STACK_SIZE];
    long call_stack_pos;
    StackFrame* frame;
    // halt
    bool halt;
#ifdef _DEBUGGER
    Debugger* debugger;
#endif
  
    //
    // push call frame
    //
    inline void PushFrame(StackFrame* f) {
      if(call_stack_pos >= CALL_STACK_SIZE) {
        cerr << ">>> call stack bounds have been exceeded! <<<" << endl;
        exit(1);
      }
      
      call_stack[call_stack_pos++] = f;
    }

    //
    // pop call frame
    //
    inline StackFrame* PopFrame() {
      if(call_stack_pos <= 0) {
        cerr << ">>> call stack bounds have been exceeded! <<<" << endl;
        exit(1);
      }
      
      return call_stack[--call_stack_pos];
    }
    
    //
    // generates a stack dump if an error occurs
    //
    inline void StackErrorUnwind() {
      long pos = call_stack_pos;
#ifdef _DEBUGGER
      cerr << "Unwinding local stack (" << this << "):" << endl;
      StackMethod* method =  frame->GetMethod();
      cerr << "  method: pos=" << pos << ", file="
	   << frame->GetMethod()->GetClass()->GetFileName() << ", line=" 
	   << method->GetInstruction(frame->GetIp() - 1)->GetLineNumber() << endl;
      while(--pos) {
	StackMethod* method =  call_stack[pos]->GetMethod();
	cerr << "  method: pos=" << pos << ", file=" 
	     << call_stack[pos]->GetMethod()->GetClass()->GetFileName() << ", line=" 
	     << method->GetInstruction(call_stack[pos]->GetIp() - 1)->GetLineNumber() << endl;
      }
      cerr << "  ..." << endl;
#else
      cerr << "Unwinding local stack (" << this << "):" << endl;
      cerr << "  method: pos=" << pos << ", name="
	   << frame->GetMethod()->GetName() << endl;
      while(--pos) {
	if(pos > - 1) {
	  cerr << "  method: pos=" << pos << ", name="
	       << call_stack[pos]->GetMethod()->GetName() << endl;
	}
      }
      cerr << "  ..." << endl;
#endif
    }
    
    //
    // generates a stack dump if an error occurs
    //
    inline void StackErrorUnwind(StackMethod* method) {
      long pos = call_stack_pos;
      cerr << "Unwinding local stack (" << this << "):" << endl;
      cerr << "  method: pos=" << pos << ", name="
	   << method->GetName() << endl;
      while(--pos) {
	if(pos > - 1) {
	  cerr << "  method: pos=" << pos << ", name="
	       << call_stack[pos]->GetMethod()->GetName() << endl;
	}
      }
      cerr << "  ..." << endl;
    }
    
    //
    // is call stack empty?
    //
    inline bool StackEmpty() {
      return call_stack_pos == 0;
    }

    //
    // pops an integer from the calculation stack.  this code
    // in normally inlined and there's a macro version available.
    //
    inline long PopInt(long* op_stack, long* stack_pos) {    
#ifdef _DEBUG
      long v = op_stack[--(*stack_pos)];
      cout << "  [pop_i: stack_pos=" << (*stack_pos) << "; value=" << v << "("
	   << (void*)v << ")]; frame=" << frame << "; frame_pos=" << call_stack_pos << endl;
      return v;
#else
      return op_stack[--(*stack_pos)];
#endif
    }
    
    //
    // pushes an integer onto the calculation stack.  this code
    // in normally inlined and there's a macro version available.
    //
    inline void PushInt(long v, long* op_stack, long* stack_pos) {
#ifdef _DEBUG
      cout << "  [push_i: stack_pos=" << (*stack_pos) << "; value=" << v << "("
	   << (void*)v << ")]; frame=" << frame << "; frame_pos=" << call_stack_pos << endl;
#endif
      op_stack[(*stack_pos)++] = v;
    }

    //
    // pushes an double onto the calculation stack.
    //
    inline void PushFloat(FLOAT_VALUE v, long* op_stack, long* stack_pos) {
#ifdef _DEBUG
      cout << "  [push_f: stack_pos=" << (*stack_pos) << "; value=" << v
	   << "]; frame=" << frame << "; frame_pos=" << call_stack_pos << endl;
#endif
      memcpy(&op_stack[(*stack_pos)], &v, sizeof(FLOAT_VALUE));

#ifdef _X64
      (*stack_pos)++;
#else
      (*stack_pos) += 2;
#endif
    }
  
    //
    // swaps two integers on the calculation stack
    //
    inline void SwapInt(long* op_stack, long* stack_pos) {
      long v = op_stack[(*stack_pos) - 2];
      op_stack[(*stack_pos) - 2] = op_stack[(*stack_pos) - 1];
      op_stack[(*stack_pos) - 1] = v;
    }
    
    //
    // pops a double from the calculation stack
    //
    inline FLOAT_VALUE PopFloat(long* op_stack, long* stack_pos) {
      FLOAT_VALUE v;

#ifdef _X64
      (*stack_pos)--;
#else
      (*stack_pos) -= 2;
#endif

      memcpy(&v, &op_stack[(*stack_pos)], sizeof(FLOAT_VALUE));
#ifdef _DEBUG
      cout << "  [pop_f: stack_pos=" << (*stack_pos) << "; value=" << v
	   << "]; frame=" << frame << "; frame_pos=" << call_stack_pos << endl;
#endif

      return v;
    }
    
    //
    // peeks at the integer on the top of the
    // execution stack.
    //
    inline long TopInt(long* op_stack, long* stack_pos) {
#ifdef _DEBUG
      long v = op_stack[(*stack_pos) - 1];
      cout << "  [top_i: stack_pos=" << (*stack_pos) << "; value=" << v << "(" << (void*)v
	   << ")]; frame=" << frame << "; frame_pos=" << call_stack_pos << endl;
      return v;
#else
      return op_stack[(*stack_pos) - 1];
#endif
    }
    
    //
    // peeks at the double on the top of the
    // execution stack.
    //
    inline FLOAT_VALUE TopFloat(long* op_stack, long* stack_pos) {
      FLOAT_VALUE v;

#ifdef _X64
      long index = (*stack_pos) - 1;
#else
      long index = (*stack_pos) - 2;
#endif

      memcpy(&v, &op_stack[index], sizeof(FLOAT_VALUE));
#ifdef _DEBUG
      cout << "  [top_f: stack_pos=" << (*stack_pos) << "; value=" << v
	   << "]; frame=" << frame << "; frame_pos=" << call_stack_pos << endl;
#endif

      return v;
    }

    //
    // calculates an array offset
    //
    inline long ArrayIndex(StackInstr* instr, long* array, const long size, long* &op_stack, long* &stack_pos) {
      // generate index
      long index = PopInt(op_stack, stack_pos);
      const long dim = instr->GetOperand();

      for(long i = 1; i < dim; i++) {
	index *= array[i];
	index += PopInt(op_stack, stack_pos);
      }

#ifdef _DEBUG
      cout << "  [raw index=" << index << ", raw size=" << size << "]" << endl;
#endif

      // 64-bit bounds check
#ifdef _X64
      if(index < 0 || index >= size) {
	cerr << ">>> Index out of bounds: " << index << "," << size << " <<<" << endl;
	StackErrorUnwind();
	exit(1);
      }
#else
      // 32-bit bounds check
      if(instr->GetType() == LOAD_FLOAT_ARY_ELM || instr->GetType() == STOR_FLOAT_ARY_ELM) {
	// float array
	index *= 2;
	if(index < 0 || index >= size * 2) {
	  cerr << ">>> Index out of bounds: " << index << "," << (size * 2) << " <<<" << endl;
	  StackErrorUnwind();
	  exit(1);
	}
      } 
      else {
	// interger array
	if(index < 0 || index >= size) {
	  cerr << ">>> Index out of bounds: " << index << "," << size << " <<<" << endl;
	  StackErrorUnwind();
	  exit(1);
	}
      }
#endif
      
      return index;
    }
    
    //
    // creates a new class instance
    //
    void CreateClassObject(StackClass* cls, long* cls_obj, long* &op_stack, long* &stack_pos) {
      // create and set methods
      const long mthd_obj_array_size = cls->GetMethodCount();
      const long mthd_obj_array_dim = 1;
      long* mthd_obj_array = (long*)MemoryManager::Instance()->AllocateArray(mthd_obj_array_size +
									     mthd_obj_array_dim + 2,
									     INT_TYPE, op_stack,
									     *stack_pos, false);
      
      mthd_obj_array[0] = mthd_obj_array_size;
      mthd_obj_array[1] = mthd_obj_array_dim;
      mthd_obj_array[2] = mthd_obj_array_size;
      
      StackMethod** methods = cls->GetMethods();
      long* mthd_obj_array_ptr = mthd_obj_array + 3;
      for(int i = 0; i < mthd_obj_array_size; i++) {
	long* mthd_obj = CreateMethodObject(cls_obj, methods[i], op_stack, stack_pos);
	mthd_obj_array_ptr[i] = (long)mthd_obj;
      }
      cls_obj[1] = (long)mthd_obj_array;
    }
    
    //
    // creates an instance of the 'Method' class
    //
    long* CreateMethodObject(long* cls_obj, StackMethod* mthd, long* &op_stack, long* &stack_pos) {
      long* mthd_obj = MemoryManager::Instance()->AllocateObject(program->GetMethodObjectId(),
								 (long*)op_stack, *stack_pos,
								 false);
      // method and class object
      mthd_obj[0] = (long)mthd;
      mthd_obj[1] = (long)cls_obj;
      
      // set method name
      const string &qual_mthd_name = mthd->GetName();
      const size_t semi_qual_mthd_index = qual_mthd_name.find(':');
      if(semi_qual_mthd_index == string::npos) {
	cerr << ">>> Internal error: invalid method name <<<" << endl;
	StackErrorUnwind();
	exit(1);
      }
      
      const string &semi_qual_mthd_string = qual_mthd_name.substr(semi_qual_mthd_index + 1);
      const size_t mthd_index = semi_qual_mthd_string.find(':');
      if(mthd_index == string::npos) {
	cerr << ">>> Internal error: invalid method name <<<" << endl;
	StackErrorUnwind();
	exit(1);
      }
      const string &mthd_string = semi_qual_mthd_string.substr(0, mthd_index);
      mthd_obj[2] = (long)CreateStringObject(mthd_string, op_stack, stack_pos);

      // parse parameter string      
      int index = 0;
      const string &params_string = semi_qual_mthd_string.substr(mthd_index + 1);
      vector<long*> data_type_obj_holder;
      while(index < (int)params_string.size()) {
	long* data_type_obj = MemoryManager::Instance()->AllocateObject(program->GetDataTypeObjectId(),
									(long*)op_stack, *stack_pos,
									false);
	data_type_obj_holder.push_back(data_type_obj);
        
	switch(params_string[index]) {
        case 'l':
	  data_type_obj[0] = -1000;
	  index++;
          break;
	  
        case 'b':
	  data_type_obj[0] = -999;
	  index++;
          break;

        case 'i':
	  data_type_obj[0] = -997;
	  index++;
          break;

        case 'f':
	  data_type_obj[0] = -996;
	  index++;
          break;

        case 'c':
	  data_type_obj[0] = -998;
	  index++;
          break;
	  
        case 'o': {
	  data_type_obj[0] = -995;
          index++;
	  const int start_index = index + 1;
          while(index < (int)params_string.size() && params_string[index] != ',') {
            index++;
          }
	  data_type_obj[1] = (long)CreateStringObject(params_string.substr(start_index, index - 2), op_stack, stack_pos);
	}
          break;
	  
	case 'm':
	  data_type_obj[0] = -994;
          index++;
          while(index < (int)params_string.size() && params_string[index] != '~') {
            index++;
          }
	  while(index < (int)params_string.size() && params_string[index] != ',') {
            index++;
          }
          break;
	  
	default:
#ifdef _DEBUG
	  assert(false);
#endif
	  break;
        }
	
        // check array dimension
        int dimension = 0;
        while(index < (int)params_string.size() && params_string[index] == '*') {
          dimension++;
          index++;
        }
	data_type_obj[2] = dimension;
	
	// match ','
        index++;
      }
      
      // create type array
      const long type_obj_array_size = (long)data_type_obj_holder.size();
      const long type_obj_array_dim = 1;
      long* type_obj_array = (long*)MemoryManager::Instance()->AllocateArray(type_obj_array_size +
									     type_obj_array_dim + 2,
									     INT_TYPE, op_stack,
									     *stack_pos, false);
      type_obj_array[0] = type_obj_array_size;
      type_obj_array[1] = type_obj_array_dim;
      type_obj_array[2] = type_obj_array_size;
      long* type_obj_array_ptr = type_obj_array + 3;
      // copy types objects
      for(int i = 0; i < type_obj_array_size; i++) {
	type_obj_array_ptr[i] = (long)data_type_obj_holder[i];
      }
      // set type array
      mthd_obj[3] = (long)type_obj_array;
      
      return mthd_obj;
    }
    
    //
    // creates a string object instance
    // 
    inline long* CreateStringObject(const string &value_str, long* &op_stack, long* &stack_pos) {
      // create character array
      const long char_array_size = value_str.size();
      const long char_array_dim = 1;
      long* char_array = (long*)MemoryManager::Instance()->AllocateArray(char_array_size + 1 +
									 ((char_array_dim + 2) *
									  sizeof(long)),
									 BYTE_ARY_TYPE,
									 op_stack, *stack_pos,
									 false);
      char_array[0] = char_array_size + 1;
      char_array[1] = char_array_dim;
      char_array[2] = char_array_size;

      // copy string
      char* char_array_ptr = (char*)(char_array + 3);
      strcpy(char_array_ptr, value_str.c_str());
      
      // create 'System.String' object instance
      long* str_obj = MemoryManager::Instance()->AllocateObject(program->GetStringObjectId(),
								(long*)op_stack, *stack_pos,
								false);
      str_obj[0] = (long)char_array;
      str_obj[1] = char_array_size;
      str_obj[2] = char_array_size;
      
      return str_obj;
    }

    //
    // creates new object and call default constructor
    // 
    inline void CreateNewObject(const string &cls_id, long* &op_stack, long* &stack_pos) {
      long* obj = MemoryManager::Instance()->AllocateObject(cls_id.c_str(), 
							    (long*)op_stack, *stack_pos, false);
      if(obj) {
	// instance will be put on stack by method call
        string mthd_name = cls_id + ":New:";
        APITools_MethodCall((long*)op_stack, stack_pos, obj, cls_id.c_str(), mthd_name.c_str());
      }
      else {
	PushInt(0, op_stack, stack_pos);
      }
    }
    
    //
    // writes out serialized objects
    // 
    inline void WriteSerializedBytes(const BYTE_VALUE* array, const long src_buffer_size, long* &op_stack, long* &stack_pos) {
      long* inst = (long*)frame->GetMemory()[0];
      long* dest_buffer = (long*)inst[0];
      const long dest_pos = inst[1];
  
      // expand buffer, if needed
      dest_buffer = ExpandSerialBuffer(src_buffer_size, dest_buffer, inst, op_stack, stack_pos);
      inst[0] = (long)dest_buffer;
  
      // copy content
      BYTE_VALUE* dest_buffer_ptr = (BYTE_VALUE*)(dest_buffer + 3);
      memcpy(dest_buffer_ptr + dest_pos, array, src_buffer_size);
      inst[1] = dest_pos + src_buffer_size;
    }
    
    //
    // serializes an array
    // 
    inline void SerializeArray(const long* array, ParamType type, long* &op_stack, long* &stack_pos) {
      if(array) {
	SerializeByte(1, op_stack, stack_pos);
	const long array_size = array[0];
	// write metadata
	SerializeInt(array[0], op_stack, stack_pos);
	SerializeInt(array[1], op_stack, stack_pos);
	SerializeInt(array[2], op_stack, stack_pos);
	BYTE_VALUE* array_ptr = (BYTE_VALUE*)(array + 3);
      
	// write values
	switch(type) {
	case BYTE_ARY_PARM:
	  WriteSerializedBytes(array_ptr, array_size, op_stack, stack_pos);
	  break;
	    
	case INT_ARY_PARM:
	  WriteSerializedBytes(array_ptr, array_size * sizeof(INT_VALUE), op_stack, stack_pos);
	  break;
	  
	case FLOAT_ARY_PARM:
	  WriteSerializedBytes(array_ptr, array_size * sizeof(FLOAT_VALUE), op_stack, stack_pos);
	  break;

	default:
	  break;
	}
      }
      else {
	SerializeByte(0, op_stack, stack_pos);
      }
    }

    //
    // reads a serialized array
    // 
    inline void ReadSerializedBytes(const long* dest_array, const long* src_array, ParamType type) {
      long* inst = (long*)frame->GetMemory()[0];
      const long dest_pos = inst[1];
      const long src_array_size = src_array[0];
      long dest_array_size = dest_array[0];
      
      if(dest_pos < src_array_size) {
	const BYTE_VALUE* src_array_ptr = (BYTE_VALUE*)(src_array + 3);	
	BYTE_VALUE* dest_array_ptr = (BYTE_VALUE*)(dest_array + 3);

	switch(type) {
	case BYTE_ARY_PARM:
	  break;
	  
	case INT_ARY_PARM:
	  dest_array_size *= sizeof(INT_VALUE);
	  break;
	  
	case FLOAT_ARY_PARM:
	  dest_array_size *= sizeof(FLOAT_VALUE);
	  break;
	  
	default:
	  break;
	}
	
	memcpy(dest_array_ptr, src_array_ptr + dest_pos, dest_array_size);
	inst[1] = dest_pos + dest_array_size;
      }
    }
    
    //
    // deserializes an array of objects
    // 
    inline long* DeserializeArray(ParamType type, long* &op_stack, long* &stack_pos) {
      if(!DeserializeByte()) {
	return NULL;
      }
      
      long* inst = (long*)frame->GetMemory()[0];
      long* src_array = (long*)inst[0];
      long dest_pos = inst[1];
      
      if(dest_pos < src_array[0]) {
	// TOOD: detect bad read?
	const long dest_array_size = DeserializeInt();
	const long dest_array_dim = DeserializeInt();
	const long dest_array_dim_size = DeserializeInt();

	long* dest_array;
	if(type == BYTE_ARY_PARM) {
	  dest_array = (long*)MemoryManager::Instance()->AllocateArray(dest_array_size +
								       ((dest_array_dim + 2) *
									sizeof(long)),
								       BYTE_ARY_TYPE,
								       op_stack, *stack_pos,
								       false);
	}
	else if(type == INT_ARY_PARM) {
	  dest_array = (long*)MemoryManager::AllocateArray(dest_array_size + dest_array_dim + 2, 
							   INT_TYPE, op_stack, *stack_pos,
							   false);
	}
	else {
	  dest_array = (long*)MemoryManager::AllocateArray(dest_array_size * 2 + dest_array_dim + 2, 
							   INT_TYPE, op_stack, *stack_pos, false);
	}
	
	dest_array[0] = dest_array_size;
	dest_array[1] = dest_array_dim;
	dest_array[2] = dest_array_dim_size;	
	
	ReadSerializedBytes(dest_array, src_array, type);	
	return dest_array;
      }
      
      return NULL;
    }
    
    //
    // expand buffer
    //
    long* ExpandSerialBuffer(const long src_buffer_size, long* dest_buffer, long* inst, long* &op_stack, long* &stack_pos) {
      long dest_buffer_size = dest_buffer[2];
      const long dest_pos = inst[1];      
      const long calc_offset = src_buffer_size + dest_pos;
      
      if(calc_offset >= dest_buffer_size) {
	const long dest_pos = inst[1];
	while(calc_offset >= dest_buffer_size) {
	  dest_buffer_size += calc_offset / 2;
	}
	// create byte array
	const long byte_array_size = dest_buffer_size;
	const long byte_array_dim = 1;
	long* byte_array = (long*)MemoryManager::Instance()->AllocateArray(byte_array_size + 1 +
									   ((byte_array_dim + 2) *
									    sizeof(long)),
									   BYTE_ARY_TYPE,
									   op_stack, *stack_pos,
									   false);
	byte_array[0] = byte_array_size + 1;
	byte_array[1] = byte_array_dim;
	byte_array[2] = byte_array_size;
	
	// copy content
	BYTE_VALUE* byte_array_ptr = (BYTE_VALUE*)(byte_array + 3);
	const BYTE_VALUE* dest_buffer_ptr = (BYTE_VALUE*)(dest_buffer + 3);	
	memcpy(byte_array_ptr, dest_buffer_ptr, dest_pos);
	
	return byte_array;
      }

      return dest_buffer;
    }
    
    // 
    // serializes a byte
    // 
    void SerializeByte(BYTE_VALUE value, long* &op_stack, long* &stack_pos) {
      const long src_buffer_size = sizeof(value);
      long* inst = (long*)frame->GetMemory()[0];
      long* dest_buffer = (long*)inst[0];
      const long dest_pos = inst[1];
  
      // expand buffer, if needed
      dest_buffer = ExpandSerialBuffer(src_buffer_size, dest_buffer, inst, op_stack, stack_pos);
      inst[0] = (long)dest_buffer;
  
      // copy content
      BYTE_VALUE* dest_buffer_ptr = (BYTE_VALUE*)(dest_buffer + 3);
      memcpy(dest_buffer_ptr + dest_pos, &value, src_buffer_size);
      inst[1] = dest_pos + src_buffer_size;
    }

    // 
    // deserializes a byte
    // 
    BYTE_VALUE DeserializeByte() {
      long* inst = (long*)frame->GetMemory()[0];
      long* byte_array = (long*)inst[0];
      const long dest_pos = inst[1];
      
      if(dest_pos < byte_array[0]) {
	const BYTE_VALUE* byte_array_ptr = (BYTE_VALUE*)(byte_array + 3);	
	BYTE_VALUE value;
	memcpy(&value, byte_array_ptr + dest_pos, sizeof(value));
	inst[1] = dest_pos + sizeof(value);
      
	return value;
      }
      
      return 0;
    }
    
    // 
    // serializes an int
    // 
    void SerializeInt(INT_VALUE value, long* &op_stack, long* &stack_pos) {
      const long src_buffer_size = sizeof(value);
      long* inst = (long*)frame->GetMemory()[0];
      long* dest_buffer = (long*)inst[0];
      const long dest_pos = inst[1];
  
      // expand buffer, if needed
      dest_buffer = ExpandSerialBuffer(src_buffer_size, dest_buffer, inst, op_stack, stack_pos);
      inst[0] = (long)dest_buffer;
  
      // copy content
      BYTE_VALUE* dest_buffer_ptr = (BYTE_VALUE*)(dest_buffer + 3);
      memcpy(dest_buffer_ptr + dest_pos, &value, src_buffer_size);
      inst[1] = dest_pos + src_buffer_size;
    }

    // 
    // deserializes an int
    // 
    INT_VALUE DeserializeInt() {
      long* inst = (long*)frame->GetMemory()[0];
      long* byte_array = (long*)inst[0];
      const long dest_pos = inst[1];
      
      if(dest_pos < byte_array[0]) {
	const BYTE_VALUE* byte_array_ptr = (BYTE_VALUE*)(byte_array + 3);	
	INT_VALUE value;
	memcpy(&value, byte_array_ptr + dest_pos, sizeof(value));
	inst[1] = dest_pos + sizeof(value);
      
	return value;
      }
      
      return 0;
    }
    
    // 
    // serializes a float
    // 
    void SerializeFloat(FLOAT_VALUE value, long* &op_stack, long* &stack_pos) {
      const long src_buffer_size = sizeof(value);
      long* inst = (long*)frame->GetMemory()[0];
      long* dest_buffer = (long*)inst[0];
      const long dest_pos = inst[1];
  
      // expand buffer, if needed
      dest_buffer = ExpandSerialBuffer(src_buffer_size, dest_buffer, inst, op_stack, stack_pos);
      inst[0] = (long)dest_buffer;
  
      // copy content
      BYTE_VALUE* dest_buffer_ptr = (BYTE_VALUE*)(dest_buffer + 3);
      memcpy(dest_buffer_ptr + dest_pos, &value, src_buffer_size);
      inst[1] = dest_pos + src_buffer_size;
    }
    
    // 
    // deserializes a float
    // 
    FLOAT_VALUE DeserializeFloat() {
      long* inst = (long*)frame->GetMemory()[0];
      long* byte_array = (long*)inst[0];
      const long dest_pos = inst[1];

      if(dest_pos < byte_array[0]) {
	const BYTE_VALUE* byte_array_ptr = (BYTE_VALUE*)(byte_array + 3);
	FLOAT_VALUE value;
	memcpy(&value, byte_array_ptr + dest_pos, sizeof(value));
	inst[1] = dest_pos + sizeof(value);      
	return value;
      }

      return 0.0;
    }

    inline void ProcessNewArray(StackInstr* instr, long* &op_stack, long* &stack_pos, bool is_float = false);
    inline void ProcessNewByteArray(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessNewObjectInstance(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessReturn(StackInstr** &instrs, long &ip);

    inline void ProcessMethodCall(StackInstr* instr, StackInstr** &instrs, long &ip, long* &op_stack, long* &stack_pos);
    inline void ProcessDynamicMethodCall(StackInstr* instr, StackInstr** &instrs, long &ip, long* &op_stack, long* &stack_pos);
    inline void ProcessJitMethodCall(StackMethod* called, long* instance, StackInstr** &instrs, long &ip, long* &op_stack, long* &stack_pos);
    inline void ProcessAsyncMethodCall(StackMethod* called, long* param);

    inline void ProcessInterpretedMethodCall(StackMethod* called, long* instance, StackInstr** &instrs, long &ip);
    inline void ProcessLoadIntArrayElement(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessStoreIntArrayElement(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessLoadFloatArrayElement(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessStoreFloatArrayElement(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessLoadByteArrayElement(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessStoreByteArrayElement(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessStoreFunction(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessLoadFunction(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessStoreFloat(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessLoadFloat(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessCopyFloat(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void ProcessCurrentTime(bool is_gmt);
    inline void ProcessSetTime1(long* &op_stack, long* &stack_pos);
    inline void ProcessSetTime2(long* &op_stack, long* &stack_pos);
    inline void ProcessSetTime3(long* &op_stack, long* &stack_pos);
    inline void ProcessAddTime(TimeInterval t, long* &op_stack, long* &stack_pos);
    inline void ProcessPlatform(long* &op_stack, long* &stack_pos);
    inline void ProcessTrap(StackInstr* instr, long* &op_stack, long* &stack_pos);
    inline void SerializeObject(long* &op_stack, long* &stack_pos);
    inline void DeserializeObject(long* &op_stack, long* &stack_pos);
    inline void ProcessDllLoad(StackInstr* instr);
    inline void ProcessDllUnload(StackInstr* instr);
    inline void ProcessDllCall(StackInstr* instr, long* &op_stack, long* &stack_pos);
    
  public:
    static void Initialize(StackProgram* p);

#ifdef _WIN32
    static uintptr_t WINAPI AsyncMethodCall(LPVOID arg);
#else
    static void* AsyncMethodCall(void* arg);
#endif

    StackInterpreter() {
    }
  
    StackInterpreter(StackProgram* p) {
      Initialize(p);
    }
  
#ifdef _DEBUGGER
    StackInterpreter(StackProgram* p, Debugger* d) {
      debugger = d;
      Initialize(p);
    }
#endif
  
    ~StackInterpreter() {
      if(frame) {
	delete frame;
	frame = NULL;
      }
    }

    // execute method
    void Execute(long* op_stack, long* stack_pos, long i, StackMethod* method, long* instance, bool jit_called);
  };
}
#endif
