/***************************************************************************
 * VM stack machine.
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
#include <random>
#include <string.h>

#ifdef _WIN32
#include "arch/memory.h"
#else
#include "arch/memory.h"
#endif

#ifdef _DEBUGGER
#include "../debugger/debugger.h"
#endif

using namespace std;
#undef max

namespace Runtime {
#ifdef _DEBUGGER
  class Debugger;
#endif
  
#define FRAME_CACHE_SIZE 24
#define CALL_STACK_SIZE 1024
#define OP_STACK_SIZE 128

  // holds the calling context for async
  // method calls
  struct ThreadHolder {
    StackMethod* called;
    size_t* self;
    size_t* param;
  };
  
  class StackInterpreter {
    // program
    static StackProgram* program;
    static set<StackInterpreter*> intpr_threads;
    static stack<StackFrame*> cached_frames;
#ifdef _WIN32
    static CRITICAL_SECTION cached_frames_cs;
    static CRITICAL_SECTION intpr_threads_cs;
#else
    static pthread_mutex_t cached_frames_mutex;
    static pthread_mutex_t intpr_threads_mutex;
#endif

    // call stack and current frame pointer
    StackFrame** call_stack;
    long* call_stack_pos;
    
    StackFrame** frame;
    StackFrameMonitor* monitor;

    // halt
    bool halt;
#ifdef _DEBUGGER
    Debugger* debugger;
#endif
    
    //
    // get stack frame
    //
    static StackFrame* GetStackFrame(StackMethod* method, size_t* instance);
    
    //
    // release stack frame
    //
    static void ReleaseStackFrame(StackFrame* frame);
    
    //
    // push call frame
    //
    inline void PushFrame(StackFrame* f) {
      if((*call_stack_pos) >= CALL_STACK_SIZE) {
        wcerr << L">>> call stack bounds have been exceeded! <<<" << endl;
        exit(1);
      }
      
      call_stack[(*call_stack_pos)++] = f;
    }

    //
    // pop call frame
    //
    inline StackFrame* PopFrame() {
      if((*call_stack_pos) <= 0) {
        wcerr << L">>> call stack bounds have been exceeded! <<<" << endl;
        exit(1);
      }
      
      return call_stack[--(*call_stack_pos)];
    }
    
    //
    // generates a stack dump if an error occurs
    //
    void StackErrorUnwind();
    
    //
    // generates a stack dump if an error occurs
    //
    void StackErrorUnwind(StackMethod* method);
    
    //
    // is call stack empty?
    //
    inline bool StackEmpty() {
      return (*call_stack_pos) == 0;
    }

    //
    // pops an integer from the calculation stack.  this code
    // in normally inlined and there's a macro version available.
    //
    inline size_t PopInt(size_t* op_stack, long* stack_pos) {
#ifdef _DEBUG
      size_t v = op_stack[--(*stack_pos)];
      wcout << L"  [pop_i: stack_pos=" << (*stack_pos) << L"; value=" << v << L"("
            << (size_t*)v << L")]; frame=" << (*frame) << L"; call_pos=" << (*call_stack_pos) << endl;
      return v;
#else
      return op_stack[--(*stack_pos)];
#endif
    }
    
    //
    // pushes an integer onto the calculation stack.  this code
    // in normally inlined and there's a macro version available.
    //
    inline void PushInt(const size_t v, size_t* op_stack, long* stack_pos) {
#ifdef _DEBUG
      wcout << L"  [push_i: stack_pos=" << (*stack_pos) << L"; value=" << v << L"("
            << (size_t*)v << L")]; frame=" << (*frame) << L"; call_pos=" << (*call_stack_pos) << endl;
#endif
      op_stack[(*stack_pos)++] = v;
    }

    //
    // pushes an double onto the calculation stack.
    //
    inline void PushFloat(const FLOAT_VALUE v, size_t* op_stack, long* stack_pos) {
#ifdef _DEBUG
      wcout << L"  [push_f: stack_pos=" << (*stack_pos) << L"; value=" << v
            << L"]; frame=" << (*frame) << L"; call_pos=" << (*call_stack_pos) << endl;
#endif
      *((FLOAT_VALUE*)(&op_stack[(*stack_pos)])) = v;
#if defined(_WIN64) || defined(_X64)
      (*stack_pos)++;
#else
      (*stack_pos) += 2;
#endif
    }
  
    //
    // swaps two integers on the calculation stack
    //
    inline void SwapInt(size_t* op_stack, long* stack_pos) {
      const size_t v = op_stack[(*stack_pos) - 2];
      op_stack[(*stack_pos) - 2] = op_stack[(*stack_pos) - 1];
      op_stack[(*stack_pos) - 1] = v;
    }
    
    //
    // pops a double from the calculation stack
    //
    inline FLOAT_VALUE PopFloat(size_t* op_stack, long* stack_pos) {
#if defined(_WIN64) || defined(_X64)
      (*stack_pos)--;
#else
      (*stack_pos) -= 2;
#endif
      
#ifdef _DEBUG
      FLOAT_VALUE v = *((FLOAT_VALUE*)(&op_stack[(*stack_pos)]));
      wcout << L"  [pop_f: stack_pos=" << (*stack_pos) << L"; value=" << v
            << L"]; frame=" << (*frame) << L"; call_pos=" << (*call_stack_pos) << endl;
      return v;
#endif

      return *((FLOAT_VALUE*)(&op_stack[(*stack_pos)]));
    }
    
    //
    // peeks at the integer on the top of the
    // execution stack.
    //
    inline size_t TopInt(size_t* op_stack, long* stack_pos) {
#ifdef _DEBUG
      size_t v = op_stack[(*stack_pos) - 1];
      wcout << L"  [top_i: stack_pos=" << (*stack_pos) << L"; value=" << v << L"(" << (void*)v
            << L")]; frame=" << (*frame) << L"; call_pos=" << (*call_stack_pos) << endl;
      return v;
#else
      return op_stack[(*stack_pos) - 1];
#endif
    }
    
    //
    // peeks at the double on the top of the
    // execution stack.
    //
    inline FLOAT_VALUE TopFloat(size_t* op_stack, long* stack_pos) {
#if defined(_WIN64) || defined(_X64)
      long index = (*stack_pos) - 1;
#else
      long index = (*stack_pos) - 2;
#endif
      
#ifdef _DEBUG
      FLOAT_VALUE v = *((FLOAT_VALUE*)(&op_stack[index]));
      wcout << L"  [top_f: stack_pos=" << (*stack_pos) << L"; value=" << v
            << L"]; frame=" << (*frame) << L"; call_pos=" << (*call_stack_pos) << endl;
      return v;
#endif
      
      return *((FLOAT_VALUE*)(&op_stack[index]));
    }

    //
    // calculates an array offset
    //
    long ArrayIndex(StackInstr* instr, size_t* array, const long size, size_t* &op_stack, long* &stack_pos);    
        
    //
    // creates a string object instance
    // 
    size_t* CreateStringObject(const wstring &value_str, size_t* &op_stack, long* &stack_pos);

    inline FLOAT_VALUE GetRandomValue() {
      random_device gen;
      return (FLOAT_VALUE)gen() / (FLOAT_VALUE)gen.max();
    }
    
    void inline StorLoclIntVar(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    void inline StorClsInstIntVar(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    void inline CopyLoclIntVar(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    void inline CopyClsInstIntVar(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    void inline LoadLoclIntVar(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    void inline LoadClsInstIntVar(StackInstr* instr, size_t* &op_stack, long* &stack_pos);

    void inline Str2Int(size_t* &op_stack, long* &stack_pos);
    void inline Str2Float(size_t* &op_stack, long* &stack_pos);
    void inline Int2Str(size_t* &op_stack, long* &stack_pos);
    void inline Float2Str(size_t* &op_stack, long* &stack_pos);
    void inline ShlInt(size_t* &op_stack, long* &stack_pos);
    void inline ShrInt(size_t* &op_stack, long* &stack_pos);
    void inline AndInt(size_t* &op_stack, long* &stack_pos);
    void inline OrInt(size_t* &op_stack, long* &stack_pos);
    void inline AddInt(size_t* &op_stack, long* &stack_pos);
    void inline AddFloat(size_t* &op_stack, long* &stack_pos);
    void inline SubInt(size_t* &op_stack, long* &stack_pos);
    void inline SubFloat(size_t* &op_stack, long* &stack_pos);
    void inline MulInt(size_t* &op_stack, long* &stack_pos);
    void inline DivInt(size_t* &op_stack, long* &stack_pos);
    void inline MulFloat(size_t* &op_stack, long* &stack_pos);
    void inline DivFloat(size_t* &op_stack, long* &stack_pos);
    void inline ModInt(size_t* &op_stack, long* &stack_pos);
    void inline BitAndInt(size_t* &op_stack, long* &stack_pos);
    void inline BitOrInt(size_t* &op_stack, long* &stack_pos);
    void inline BitXorInt(size_t* &op_stack, long* &stack_pos);
    void inline LesEqlInt(size_t* &op_stack, long* &stack_pos);
    void inline GtrEqlInt(size_t* &op_stack, long* &stack_pos);
    void inline LesEqlFloat(size_t* &op_stack, long* &stack_pos);
    void inline GtrEqlFloat(size_t* &op_stack, long* &stack_pos);
    void inline EqlInt(size_t* &op_stack, long* &stack_pos);
    void inline NeqlInt(size_t* &op_stack, long* &stack_pos);
    void inline LesInt(size_t* &op_stack, long* &stack_pos);
    void inline GtrInt(size_t* &op_stack, long* &stack_pos);
    void inline EqlFloat(size_t* &op_stack, long* &stack_pos);
    void inline NeqlFloat(size_t* &op_stack, long* &stack_pos);
    void inline LesFloat(size_t* &op_stack, long* &stack_pos);
    void inline GtrFloat(size_t* &op_stack, long* &stack_pos);
    void inline LoadArySize(size_t* &op_stack, long* &stack_pos);
    void inline CpyByteAry(size_t* &op_stack, long* &stack_pos);
    void inline CpyCharAry(size_t* &op_stack, long* &stack_pos);
    void inline CpyIntAry(size_t* &op_stack, long* &stack_pos);
    void inline CpyFloatAry(size_t* &op_stack, long* &stack_pos);
    void inline ObjTypeOf(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    void inline ObjInstCast(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    void inline AsyncMthdCall(size_t* &op_stack, long* &stack_pos);
    void inline ThreadJoin(size_t* &op_stack, long* &stack_pos);
    void inline ThreadMutex(size_t* &op_stack, long* &stack_pos);
    void inline CriticalStart(size_t* &op_stack, long* &stack_pos);
    void inline CriticalEnd(size_t* &op_stack, long* &stack_pos);

    inline void ProcessNewArray(StackInstr* instr, size_t* &op_stack, long* &stack_pos, bool is_float = false);
    inline void ProcessNewByteArray(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessNewCharArray(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessNewObjectInstance(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessNewFunctionInstance(StackInstr* instr, size_t*& op_stack, long*& stack_pos);
    inline void ProcessReturn(StackInstr** &instrs, long &ip);

    inline void ProcessMethodCall(StackInstr* instr, StackInstr** &instrs, long &ip, size_t* &op_stack, long* &stack_pos);
    inline void ProcessDynamicMethodCall(StackInstr* instr, StackInstr** &instrs, long &ip, size_t* &op_stack, long* &stack_pos);
    inline void ProcessJitMethodCall(StackMethod* called, size_t* instance, StackInstr** &instrs, long &ip, size_t* &op_stack, long* &stack_pos);
    inline void ProcessAsyncMethodCall(StackMethod* called, size_t* param);

    inline void ProcessInterpretedMethodCall(StackMethod* called, size_t* instance, StackInstr** &instrs, long &ip);
    inline void ProcessLoadIntArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessStoreIntArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessLoadFloatArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessStoreFloatArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessLoadByteArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessStoreByteArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos);    
    inline void ProcessStoreCharArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessLoadCharArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessStoreFunction(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessLoadFunction(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessStoreFloat(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessLoadFloat(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessCopyFloat(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    inline void ProcessDllLoad(StackInstr* instr);
    inline void ProcessDllUnload(StackInstr* instr);
    inline void ProcessDllCall(StackInstr* instr, size_t* &op_stack, long* &stack_pos);
    
  public:
    // initialize the runtime system
    static void Initialize(StackProgram* p);

    static void AddThread(StackInterpreter* i) {
#ifdef _WIN32
      EnterCriticalSection(&intpr_threads_cs);
#else
      pthread_mutex_lock(&intpr_threads_mutex);
#endif
      
      intpr_threads.insert(i);
      
#ifdef _WIN32
      LeaveCriticalSection(&intpr_threads_cs);
#else
      pthread_mutex_unlock(&intpr_threads_mutex);
#endif
    }

    static void RemoveThread(StackInterpreter* i) {
#ifdef _WIN32
      EnterCriticalSection(&intpr_threads_cs);
#else
      pthread_mutex_lock(&intpr_threads_mutex);
#endif
      
      intpr_threads.erase(i);
      
#ifdef _WIN32
      LeaveCriticalSection(&intpr_threads_cs);
#else
      pthread_mutex_unlock(&intpr_threads_mutex);
#endif
    }
    
    static void HaltAll() {
#ifdef _WIN32
      EnterCriticalSection(&intpr_threads_cs);
#else
      pthread_mutex_lock(&intpr_threads_mutex);
#endif
      
      set<StackInterpreter*>::iterator iter;
      for(iter = intpr_threads.begin(); iter != intpr_threads.end(); ++iter) {
        (*iter)->Halt();
      }

#ifdef _WIN32
      LeaveCriticalSection(&intpr_threads_cs);
#else
      pthread_mutex_unlock(&intpr_threads_mutex);
#endif
    }

    //
    void Halt() {
      halt = true;
    }
    
    // free static resources
    static void Clear() {
      while(!cached_frames.empty()) {
        StackFrame* frame = cached_frames.top();
        cached_frames.pop();
        free(frame->mem);
        delete frame;
      }
    }

#ifdef _WIN32
    static unsigned int WINAPI AsyncMethodCall(LPVOID arg);
#else
    static void* AsyncMethodCall(void* arg);
#endif

    StackInterpreter(StackFrame** c, long* cp) {
      // setup frame
      call_stack = c;
      call_stack_pos = cp;
      frame = new StackFrame*;
      monitor = nullptr;
      
      MemoryManager::AddPdaMethodRoot(frame);
    }

    StackInterpreter() {
      // setup frame
      call_stack = new StackFrame*[CALL_STACK_SIZE];
      call_stack_pos = new long;
      *call_stack_pos = -1;
      frame = new StackFrame*;

      // register monitor
      monitor = new StackFrameMonitor;
      monitor->call_stack = call_stack;
      monitor->call_stack_pos = call_stack_pos;
      monitor->cur_frame = frame;
      MemoryManager::AddPdaMethodRoot(monitor);
    }
  
    StackInterpreter(StackProgram* p) {
      Initialize(p);
      
      // setup frame
      call_stack = new StackFrame*[CALL_STACK_SIZE];
      call_stack_pos = new long;
      *call_stack_pos = -1;
      frame = new StackFrame*;

      // register monitor
      monitor = new StackFrameMonitor;
      monitor->call_stack = call_stack;
      monitor->call_stack_pos = call_stack_pos;
      monitor->cur_frame = frame;      
      MemoryManager::AddPdaMethodRoot(monitor);
    }
  
#ifdef _DEBUGGER
    StackInterpreter(StackProgram* p, Debugger* d) {
      Initialize(p);
      
      debugger = d;
      
      // setup frame
      call_stack = new StackFrame*[CALL_STACK_SIZE];
      call_stack_pos = new long;
      *call_stack_pos = -1;
      frame = new StackFrame*;

      // register monitor
      monitor = new StackFrameMonitor;
      monitor->call_stack = call_stack;
      monitor->call_stack_pos = call_stack_pos;
      monitor->cur_frame = frame;
      MemoryManager::AddPdaMethodRoot(monitor);
    }
#endif
    
    ~StackInterpreter() {
      if(monitor) {
        MemoryManager::RemovePdaMethodRoot(monitor);

        delete[] call_stack;
        call_stack = nullptr;

        delete call_stack_pos;
        call_stack_pos = nullptr;

        delete monitor;
        monitor = nullptr;
      }
      else {
        MemoryManager::RemovePdaMethodRoot(frame);
      }

      delete frame;
      frame = nullptr;
    }

    // execute method
    void Execute(size_t* op_stack, long* stack_pos, long i, StackMethod* method, size_t* instance, bool jit_called);
  };
}
#endif
