/***************************************************************************
 * VM common.
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

#include "common.h"
#include "loader.h"
#include "interpreter.h"

#ifdef _WIN32
list<HANDLE> StackProgram::thread_ids;
CRITICAL_SECTION StackProgram::program_cs;
#else
list<pthread_t> StackProgram::thread_ids;
pthread_mutex_t StackProgram::program_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/********************************
 * ObjectSerializer struct
 ********************************/
void ObjectSerializer::CheckObject(long* mem, bool is_obj, long depth) {
  if(mem) {
    // TODO: optimize so this is not a double call.. see below
    StackClass* cls = MemoryManager::Instance()->GetClass(mem);
    if(cls) {
      // write id
      WriteInt(cls->GetId());
      
      if(!WasSerialized(mem)) {
	long mem_size = cls->GetInstanceMemorySize();
	
#ifdef _X64
	mem_size *= 2;
#endif
	
#ifdef _DEBUG
	for(int i = 0; i < depth; i++) {
	  cout << "\t";
	}
	cout << "\t----- SERIALIZING object: cls_id=" << cls->GetId() << ", mem_id=" 
	     << cur_id << ", size=" << mem_size << " byte(s) -----" << endl;
#endif
	CheckMemory(mem, cls->GetDeclarations(), cls->GetNumberDeclarations(), depth + 1);
      } 
    }
    else {
      // NOTE: this happens when we are trying to mark unidentified memory
      // segments. these segments may be parts of that stack or temp for
      // register variables
#ifdef _DEBUG
      for(int i = 0; i < depth; i++) {
        cout << "\t";
      }
      cout <<"$: addr/value=" << mem << endl;
      if(is_obj) {
        assert(cls);
      }
#endif
      // primitive or object array
      if(!WasSerialized(mem)) {
	long* array = (mem);
	const long size = array[0];
	const long dim = array[1];
	long* objects = (long*)(array + 2 + dim);

#ifdef _DEBUG
	for(int i = 0; i < depth; i++) {
	  cout << "\t";
	}
	cout << "\t----- SERIALIZE: size=" << (size * sizeof(INT_VALUE)) << " -----" << endl;	
#endif
	
	for(long k = 0; k < size; k++) {
	  CheckObject((long*)objects[k], false, 2);
	}
      }
    }
  }
}

void ObjectSerializer::CheckMemory(long* mem, StackDclr** dclrs, const long dcls_size, long depth) {
  // check method
  for(long i = 0; i < dcls_size; i++) {
#ifdef _DEBUG
    for(long j = 0; j < depth; j++) {
      cout << "\t";
    }
#endif

    // write type
    WriteInt(dclrs[i]->type);
    
    // update address based upon type
    switch(dclrs[i]->type) {
    case INT_PARM: {
#ifdef _DEBUG
      cout << "\t" << i << ": ----- serializing int: value=" << (*mem) << ", size=" << sizeof(INT_VALUE) << " byte(s) -----" << endl;
#endif
      WriteInt(*mem);
      // update
      mem++;
    }
      break;

    case FLOAT_PARM: {
      FLOAT_VALUE value;
      memcpy(&value, mem, sizeof(FLOAT_VALUE));
#ifdef _DEBUG
      cout << "\t" << i << ": ----- serializing float: value=" << value << ", size=" 
	   << sizeof(FLOAT_VALUE) << " byte(s) -----" << endl;
#endif
      WriteFloat(value);
      // update
      mem += 2;
    }
    break;
    
    case BYTE_ARY_PARM: {
      long* array = (long*)(*mem);
      // mark data
      if(!WasSerialized((long*)(*mem))) {
	const long array_size = array[0];
#ifdef _DEBUG
	cout << "\t" << i << ": ----- serializing byte array: mem_id=" << cur_id << ", size=" 
	     << array_size << " byte(s) -----" << endl;
#endif
	// write metadata
	WriteInt(array[0]);
	WriteInt(array[1]);
	WriteInt(array[2]);
	BYTE_VALUE* array_ptr = (BYTE_VALUE*)(array + 3);
	// values
	WriteBytes(array_ptr, array_size);
      }
      // update
      mem++;
    }
      break;
      
    case INT_ARY_PARM: {
      long* array = (long*)(*mem);
      // mark data
      if(!WasSerialized((long*)(*mem))) {
	const long array_size = array[0];
#ifdef _DEBUG
	cout << "\t" << i << ": ----- serializing int array: mem_id=" << cur_id << ", size=" 
	     << array_size << " byte(s) -----" << endl;
#endif
	// write metadata
	WriteInt(array[0]);
	WriteInt(array[1]);
	WriteInt(array[2]);
	long* array_ptr = array + 3;	
	// values
	for(int i = 0; i < array_size; i++) {
	  WriteInt(array_ptr[i]);
	}
      }
      // update
      mem++;
    }
      break;
      
    case FLOAT_ARY_PARM: {
      long* array = (long*)(*mem);
      // mark data
      if(!WasSerialized((long*)(*mem))) {
	const long array_size = array[0];
#ifdef _DEBUG
	cout << "\t" << i << ": ----- serializing float array: mem_id=" << cur_id << ", size=" 
	     << array_size << " byte(s) -----" << endl;
#endif
	// write metadata
	WriteInt(array[0]);
	WriteInt(array[1]);
	WriteInt(array[2]);
	FLOAT_VALUE* array_ptr = (FLOAT_VALUE*)(array + 3);
	// write values
	WriteBytes(array_ptr, array_size * sizeof(FLOAT_VALUE));
      }
      // update
      mem++;
    }
      break;
      
    case OBJ_PARM: {
#ifdef _DEBUG
      long* array = (long*)(*mem);
      cout << "\t" << i << ": OBJ_PARM: addr=" << array << "("
           << (long)array << "), id=" << array[0] << endl;
#endif
      // check object
      CheckObject((long*)(*mem), true, depth + 1);
      // update
      mem++;
    }
      break;
    }
  }
}

void ObjectSerializer::Serialize(long* inst) {
  next_id = 0;

  WriteInt(OBJ_PARM);
  CheckObject(inst, true, 0);
}

ObjectSerializer::ObjectSerializer(long* i) {
  Serialize(i);
}

ObjectSerializer::~ObjectSerializer() {
}

/********************************
 * ObjectDeserializer class
 ********************************/
long* ObjectDeserializer::DeserializeObject() {
  ParamType type = (ParamType)ReadInt();
  INT_VALUE obj_id = ReadInt();
  cls = Loader::GetProgram()->GetClass(obj_id);
  if(cls) {
    INT_VALUE mem_id = ReadInt();
    if(mem_id < 0) {
      instance = MemoryManager::AllocateObject(cls->GetId(), (long*)op_stack, *stack_pos);
    }
    else {
      return NULL;
    }
  }
  else {
    return NULL;
  }
  
  while(buffer_offset < buffer_array_size) {
    type = (ParamType)ReadInt();
    
    switch(type) {
    case INT_PARM:
      instance[instance_pos++] = ReadInt();
#ifdef _DEBUG
      cout << "--- deserialization: int value=" << instance[instance_pos - 1] << " ---" << endl;
#endif
      break;

    case FLOAT_PARM: {
      FLOAT_VALUE value = ReadFloat();
      memcpy(&instance[instance_pos], &value, sizeof(value));
#ifdef _DEBUG
      cout << "--- deserialization: float value=" << value << " ---" << endl;
#endif
      instance_pos += 2;
    }
      break;
      
    case BYTE_ARY_PARM: {
      INT_VALUE mem_id = ReadInt();
      if(mem_id < 0) {
	const long byte_array_size = ReadInt();
	const long byte_array_dim = ReadInt();
	const long byte_array_size_dim = ReadInt();
	long* byte_array = (long*)MemoryManager::AllocateArray(byte_array_size +
							       ((byte_array_dim + 2) *
								sizeof(long)),
							       BYTE_ARY_TYPE,
							       op_stack, *stack_pos);
	BYTE_VALUE* byte_array_ptr = (BYTE_VALUE*)(byte_array + 3);
	byte_array[0] = byte_array_size;
	byte_array[1] = byte_array_dim;
	byte_array[2] = byte_array_size_dim;
	
	// copy content
	memcpy(byte_array_ptr, buffer + buffer_offset, byte_array_size);
	buffer_offset += byte_array_size;
	
	// update cache
	mem_cache[mem_id] = byte_array;
	instance[instance_pos++] = (long)byte_array;
      }
      else {
	map<INT_VALUE, long*>::iterator found = mem_cache.find(-mem_id);
	if(found != mem_cache.end()) {
	  return NULL;
	} 
	instance[instance_pos++] = (long)found->second;
      }
    }
      break;
      
    case INT_ARY_PARM: {
      INT_VALUE mem_id = ReadInt();
      if(mem_id < 0) {
	const long array_size = ReadInt();
	const long array_dim = ReadInt();
	const long array_size_dim = ReadInt();	
	long* array = (long*)MemoryManager::AllocateArray(array_size + array_dim + 2, 
							  INT_TYPE, op_stack, *stack_pos);
	array[0] = array_size;
	array[1] = array_dim;
	array[2] = array_size_dim;
	long* array_ptr = array + 3;	
	// copy content
	for(int i = 0; i < array_size; i++) {
	  array_ptr[i] = ReadInt();
	}
	// update cache
	mem_cache[mem_id] = array;
	instance[instance_pos++] = (long)array;
      }
      else {
	map<INT_VALUE, long*>::iterator found = mem_cache.find(-mem_id);
	if(found != mem_cache.end()) {
	  return NULL;
	} 
	instance[instance_pos++] = (long)found->second;
      }
    }
      break;
      
    case FLOAT_ARY_PARM: {
      INT_VALUE mem_id = ReadInt();
      if(mem_id < 0) {
	const long array_size = ReadInt();
	const long array_dim = ReadInt();
	const long array_size_dim = ReadInt();
	long* array = (long*)MemoryManager::AllocateArray(array_size + array_dim + 2, 
							  INT_TYPE, op_stack, *stack_pos);
	
	array[0] = array_size;
	array[1] = array_dim;
	array[2] = array_size_dim;
	FLOAT_VALUE* array_ptr = (FLOAT_VALUE*)(array + 3);
	
	// copy content
	memcpy(array_ptr, buffer + buffer_offset, array_size * sizeof(FLOAT_VALUE));
	buffer_offset += array_size * sizeof(FLOAT_VALUE);
	
	// update cache
	mem_cache[mem_id] = array;
	instance[instance_pos++] = (long)array;
      }
      else {
	map<INT_VALUE, long*>::iterator found = mem_cache.find(-mem_id);
	if(found != mem_cache.end()) {
	  return NULL;
	} 
	instance[instance_pos++] = (long)found->second;
      }
    }
      break;
      
    case OBJ_PARM: {
      ObjectDeserializer deserializer(buffer, buffer_offset, buffer_array_size, op_stack, stack_pos);
      instance[instance_pos++] = (long)deserializer.DeserializeObject();
    }
      break;
    }
  }
  
  return instance;
}

/********************************
 * SDK functions
 ********************************/
#ifndef _UTILS
void DLLTools_MethodCall(long* op_stack, long *stack_pos, long *instance, 
			 int cls_id, int mthd_id) {
  StackClass* cls = Loader::GetProgram()->GetClass(cls_id);
  if(cls) {
    StackMethod* mthd = cls->GetMethod(mthd_id);
    if(mthd) {
      Runtime::StackInterpreter intpr;
      intpr.Execute((long*)op_stack, (long*)stack_pos, 0, mthd, instance, false);
    }
    else {
      cerr << ">>> DLL call: Unable to locate method; id=" << mthd_id << " <<<" << endl;
      exit(1);
    }
  }
  else {
    cerr << ">>> DLL call: Unable to locate class; id=" << cls_id << " <<<" << endl;
    exit(1);
  }
}

void DLLTools_MethodCall(long* op_stack, long *stack_pos, long *instance, 
			 const char* cls_id, const char* mthd_id) {
  StackClass* cls = Loader::GetProgram()->GetClass(cls_id);
  if(cls) {
    StackMethod* mthd = cls->GetMethod(mthd_id);
    if(mthd) {
      Runtime::StackInterpreter intpr;
      intpr.Execute((long*)op_stack, (long*)stack_pos, 0, mthd, instance, false);
    }
    else {
      cerr << ">>> DLL call: Unable to locate method; name=': " << mthd_id << "' <<<" << endl;
      exit(1);
    }
  }
  else {
    cerr << ">>> DLL call: Unable to locate class; name='" << cls_id << "' <<<" << endl;
    exit(1);
  }
}
#endif
