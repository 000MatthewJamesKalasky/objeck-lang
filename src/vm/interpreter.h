/***************************************************************************
 * VM stack machine.
 *
 * Copyright (c) 2008-2010 Randy Hollines
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

#ifndef __STACK_INTPR_H__
#define __STACK_INTPR_H__

#include "common.h"
#include <string.h>

#ifdef _WIN32
#include "os/windows/memory.h"
#else
#include "os/posix/memory.h"
#endif

using namespace std;

namespace Runtime {

#define STACK_SIZE 128

/********************************
 * PDA stack interpreter
 ********************************/
struct AsyncMethodCallParams {
  StackMethod* called;
  long* instance;
  long value;
};

class StackInterpreter {
  // program
  static StackProgram* program;
  // calculation stack
  long* op_stack;
  long* stack_pos;
  // call stack
  StackFrame* call_stack[STACK_SIZE];
  long call_stack_pos;
  // current frame
  StackFrame* frame;
  long ip;
  // halt
  bool halt;
  
  // JIT compiler thread handles
#ifdef _WIN32
  static DWORD WINAPI CompileMethod(LPVOID arg);
  static DWORD WINAPI AsyncMethodCall(LPVOID arg);
#else
  static void* CompileMethod(void* arg);
  static void* AsyncMethodCall(void* arg);
#endif
  
  inline void PushFrame(StackFrame* f) {
    call_stack[call_stack_pos++] = f;
  }

  inline StackFrame* PopFrame() {
    return call_stack[--call_stack_pos];
  }

  inline void StackErrorUnwind() {
    long pos = call_stack_pos;
    cerr << "Unwinding local stack (" << this << "):" << endl;
    cerr << "  method: pos=" << pos << ", name="
         << frame->GetMethod()->GetName() << endl;
    while(pos--) {
      cerr << "  method: pos=" << pos << ", name="
           << call_stack[pos]->GetMethod()->GetName() << endl;
    }
    cerr << "  ..." << endl;
  }

  inline bool StackEmpty() {
    return call_stack_pos == 0;
  }

  inline void PushInt(long v) {
#ifdef _DEBUG
    cout << "  [push_i: stack_pos=" << (*stack_pos) << "; value=" << v << "("
         << (void*)v << ")]; frame=" << frame << "; frame_pos=" << call_stack_pos << endl;
#endif
    op_stack[(*stack_pos)++] = v;
  }

  inline void PushFloat(FLOAT_VALUE v) {
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

  inline long PopInt() {
    long v = op_stack[--(*stack_pos)];
#ifdef _DEBUG
    cout << "  [pop_i: stack_pos=" << (*stack_pos) << "; value=" << v << "("
         << (void*)v << ")]; frame=" << frame << "; frame_pos=" << call_stack_pos << endl;
#endif
    return v;
  }

  inline FLOAT_VALUE PopFloat() {
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

  inline long TopInt() {
    long v = op_stack[(*stack_pos) - 1];
#ifdef _DEBUG
    cout << "  [top_i: stack_pos=" << (*stack_pos) << "; value=" << v << "(" << (void*)v
         << ")]; frame=" << frame << "; frame_pos=" << call_stack_pos << endl;
#endif
    return v;
  }

  inline FLOAT_VALUE TopFloat() {
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

  inline long ArrayIndex(StackInstr* instr, long* array, const long size) {
    // generate index
    long index = PopInt();
    const long dim = instr->GetOperand();

    for(long i = 1; i < dim; i++) {
      index *= array[i];
      index += PopInt();
    }

#ifdef _DEBUG
    cout << "  [raw index=" << index << ", raw size=" << size << "]" << endl;
#endif

    // bounds check
    if(instr->GetType() == LOAD_FLOAT_ARY_ELM || instr->GetType() == STOR_FLOAT_ARY_ELM) {
      index *= 2;
      if(index < 0 || index >= size * 2) {
        cerr << ">>> Index out of bounds: " << index << "," << (size * 2) << " <<<" << endl;
        StackErrorUnwind();
        exit(1);
      }
    } 
    else {
      if(index < 0 || index >= size) {
        cerr << ">>> Index out of bounds: " << index << "," << size << " <<<" << endl;
        StackErrorUnwind();
        exit(1);
      }
    }
    
    return index;
  }

  static void* AsyncCall(void* arg);
  static void* AsyncJitCall(void* arg);
  
  inline void ProcessNewArray(StackInstr* instr, bool is_float = false);
  inline void ProcessNewByteArray(StackInstr* instr);
  inline void ProcessNewObjectInstance(StackInstr* instr);
  inline void ProcessReturn();

  inline void ProcessAsyncMethodCall(StackInstr* instr);
  inline void ProcessInterpretedAsyncMethodCall(StackMethod* called, long instance);

  inline void ProcessMethodCall(StackInstr* instr);
  inline void ProcessJitMethodCall(StackMethod* called, long instance);
  inline void ProcessInterpretedMethodCall(StackMethod* called, long instance);
  inline void ProcessLoadIntArrayElement(StackInstr* instr);
  inline void ProcessStoreIntArrayElement(StackInstr* instr);
  inline void ProcessLoadFloatArrayElement(StackInstr* instr);
  inline void ProcessStoreFloatArrayElement(StackInstr* instr);
  inline void ProcessLoadByteArrayElement(StackInstr* instr);
  inline void ProcessStoreByteArrayElement(StackInstr* instr);
  inline void ProcessStoreInt(StackInstr* instr);
  inline void ProcessLoadInt(StackInstr* instr);
  inline void ProcessStoreFloat(StackInstr* instr);
  inline void ProcessLoadFloat(StackInstr* instr);
  inline void ProcessCopyInt(StackInstr* instr);
  inline void ProcessCopyFloat(StackInstr* instr);
  inline void ProcessCurrentTime();
  inline void ProcessTrap(StackInstr* instr);

public:
  static void Initialize(StackProgram* p);

  StackInterpreter() {}

  StackInterpreter(StackProgram* p) {
    Initialize(p);
  }

  ~StackInterpreter() {
    if(frame) {
      delete frame;
      frame = NULL;
    }
  }

  // execute method
  void Execute(long* stack, long* pos, long i, StackMethod* method, 
	       long* self, bool jit_called);
  void Execute();
};
}
#endif
