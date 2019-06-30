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

#include "interpreter.h"
#include "lib_api.h"

#ifndef _NO_JIT
#if defined(_WIN64) || defined(_X64)
#include "arch/jit/amd64/jit_amd_lp64.h"
#else
#include "arch/jit/ia32/jit_intel_lp32.h"
#endif
#endif

#ifdef _WIN32
#include "arch/win32/win32.h"
#else
#include "arch/posix/posix.h"
#endif

#ifdef _DEBUGGER
#include "../debugger/debugger.h"
#endif

#include <sstream>
#include <math.h>

using namespace Runtime;

StackProgram* StackInterpreter::program;
stack<StackFrame*> StackInterpreter::cached_frames;
set<StackInterpreter*> StackInterpreter::intpr_threads;
#ifdef _WIN32
CRITICAL_SECTION StackInterpreter::cached_frames_cs;
CRITICAL_SECTION StackInterpreter::intpr_threads_cs;
#else
pthread_mutex_t StackInterpreter::cached_frames_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t StackInterpreter::intpr_threads_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/********************************
 * VM initialization
 ********************************/
void StackInterpreter::Initialize(StackProgram* p)
{
  program = p;
  
#ifdef _WIN32
  InitializeCriticalSection(&cached_frames_cs);
  InitializeCriticalSection(&intpr_threads_cs);
#endif

#ifndef _SANITIZE
  // allocate frames
  for(size_t i = 0; i < FRAME_CACHE_SIZE; ++i) {
    StackFrame* frame = new StackFrame();
    frame->mem = (size_t*)calloc(LOCAL_SIZE, sizeof(size_t));
    cached_frames.push(frame);
  }
#endif
  
#ifdef _WIN32
  StackMethod::InitVirtualEntry();
#endif 

#ifndef _NO_JIT
#if defined(_WIN64) || defined(_X64)
  JitCompilerIA64::Initialize(program);
#elif _ARM32
  JitCompilerA32::Initialize(program);
#else
  JitCompilerIA32::Initialize(program);
#endif
#endif
  MemoryManager::Initialize(program);
}

/********************************
 * Main VM execution method. This
 * funciton is used by callbacks 
 * from native code for the C API
 ********************************/
void StackInterpreter::Execute(size_t* op_stack, long* stack_pos, long i, StackMethod* method, size_t* instance, bool jit_called)
{
  long left;
  double left_double, right_double;
  
#ifdef _TIMING
  clock_t start = clock();
#endif

  // inital setup
  if(monitor) {
    (*call_stack_pos) = 0;
  }
  (*frame) = GetStackFrame(method, instance);
  
#ifdef _DEBUG
  wcout << L"creating frame=" << (*frame) << endl;
#endif
  (*frame)->jit_called = jit_called;
  StackInstr** instrs = (*frame)->method->GetInstructions();
  long ip = i;

#ifdef _TIMING
  const wstring mthd_name = (*frame)->method->GetName();
#endif

#ifdef _DEBUG
  wcout << L"\n---------- Executing Interpreted Code: id=" 
        << (((*frame)->method->GetClass()) ? (*frame)->method->GetClass()->GetId() : -1) << L","
        << (*frame)->method->GetId() << L"; method_name='" << (*frame)->method->GetName() 
        << L"' ---------\n" << endl;
#endif

  // execute
  halt = false;
  do {
    StackInstr* instr = instrs[ip++];
    
#ifdef _DEBUGGER
    debugger->ProcessInstruction(instr, ip, call_stack, (*call_stack_pos), (*frame));
#endif
    
    switch(instr->GetType()) {
    case STOR_LOCL_INT_VAR:
      StorLoclIntVar(instr, op_stack, stack_pos);
      break;
      
    case STOR_CLS_INST_INT_VAR:
      StorClsInstIntVar(instr, op_stack, stack_pos);
      break;
      
    case STOR_FUNC_VAR:
      ProcessStoreFunction(instr, op_stack, stack_pos);
      break;

    case STOR_FLOAT_VAR:
      ProcessStoreFloat(instr, op_stack, stack_pos);
      break;
      
    case COPY_LOCL_INT_VAR:
      CopyLoclIntVar(instr, op_stack, stack_pos);
      break;
      
    case COPY_CLS_INST_INT_VAR:
      CopyClsInstIntVar(instr, op_stack, stack_pos);
      break;
      
    case COPY_FLOAT_VAR:
      ProcessCopyFloat(instr, op_stack, stack_pos);
      break;
    
    case LOAD_CHAR_LIT:
    case LOAD_INT_LIT:
#ifdef _DEBUG
      wcout << L"stack oper: LOAD_INT_LIT; call_pos=" << (*call_stack_pos) << endl;
#endif
      PushInt(instr->GetOperand(), op_stack, stack_pos);
      break;

    case SHL_INT:
      ShlInt(op_stack, stack_pos);
      break;
      
    case SHR_INT:
      ShrInt(op_stack, stack_pos);
      break;

    case LOAD_FLOAT_LIT:
#ifdef _DEBUG
      wcout << L"stack oper: LOAD_FLOAT_LIT; call_pos=" << (*call_stack_pos) << endl;
#endif
      PushFloat(instr->GetFloatOperand(), op_stack, stack_pos);
      break;

    case LOAD_LOCL_INT_VAR:
      LoadLoclIntVar(instr, op_stack, stack_pos);
      break;
      
    case LOAD_CLS_INST_INT_VAR:
      LoadClsInstIntVar(instr, op_stack, stack_pos);
      break;
      
    case LOAD_FUNC_VAR:
      ProcessLoadFunction(instr, op_stack, stack_pos);
      break;

    case LOAD_FLOAT_VAR:
      ProcessLoadFloat(instr, op_stack, stack_pos);
      break;

    case AND_INT:
      AndInt(op_stack, stack_pos);
      break;

    case OR_INT:
      OrInt(op_stack, stack_pos);
      break;

    case ADD_INT:
      AddInt( op_stack, stack_pos);
      break;

    case ADD_FLOAT:
      AddFloat(op_stack, stack_pos);
      break;

    case SUB_INT:
      SubInt(op_stack, stack_pos);
      break;

    case SUB_FLOAT:
      SubFloat(op_stack, stack_pos);
      break;

    case MUL_INT:
      MulInt(op_stack, stack_pos);
      break;

    case DIV_INT:
      DivInt(op_stack, stack_pos);
      break;

    case MUL_FLOAT:
      MulFloat(op_stack, stack_pos);
      break;

    case DIV_FLOAT:
      DivFloat(op_stack, stack_pos);
      break;

    case MOD_INT:
      ModInt(op_stack, stack_pos);
      break;

    case BIT_AND_INT:
      BitAndInt(op_stack, stack_pos);
      break;

    case BIT_OR_INT:
      BitOrInt(op_stack, stack_pos);
      break;

    case BIT_XOR_INT:
      BitXorInt(op_stack, stack_pos);
      break;

    case LES_EQL_INT:
      LesEqlInt(op_stack, stack_pos);
      break;

    case GTR_EQL_INT:
      GtrEqlInt(op_stack, stack_pos);
      break;

    case LES_EQL_FLOAT:
      LesEqlFloat(op_stack, stack_pos);
      break;

    case GTR_EQL_FLOAT:
      GtrEqlFloat(op_stack, stack_pos);
      break;

    case EQL_INT:
      EqlInt(op_stack, stack_pos);
      break;

    case NEQL_INT:
      NeqlInt(op_stack, stack_pos);
      break;

    case LES_INT:
      LesInt(op_stack, stack_pos);
      break;

    case GTR_INT:
      GtrInt(op_stack, stack_pos);
      break;

    case EQL_FLOAT:
      EqlFloat(op_stack, stack_pos);
      break;

    case NEQL_FLOAT:
      NeqlFloat(op_stack, stack_pos);
      break;

    case LES_FLOAT:
      LesFloat(op_stack, stack_pos);
      break;

    case GTR_FLOAT:
      GtrFloat(op_stack, stack_pos);
      break;

    case LOAD_ARY_SIZE:
      LoadArySize(op_stack, stack_pos);
      break;

    case CPY_BYTE_ARY:
      CpyByteAry(op_stack, stack_pos);
      break;

    case CPY_CHAR_ARY:
      CpyCharAry(op_stack, stack_pos);
      break;

    case CPY_INT_ARY:
      CpyIntAry(op_stack, stack_pos);
      break;

    case CPY_FLOAT_ARY:
      CpyFloatAry(op_stack, stack_pos);
      break;

      // Note: no supported via JIT -- *start*
    case CEIL_FLOAT:
      PushFloat(ceil(PopFloat(op_stack, stack_pos)), op_stack, stack_pos);
      break;

    case FLOR_FLOAT:
      PushFloat(floor(PopFloat(op_stack, stack_pos)), op_stack, stack_pos);
      break;

    case SIN_FLOAT:
      PushFloat(sin(PopFloat(op_stack, stack_pos)), op_stack, stack_pos);
      break;

    case COS_FLOAT:
      PushFloat(cos(PopFloat(op_stack, stack_pos)), op_stack, stack_pos);
      break;

    case TAN_FLOAT:
      PushFloat(tan(PopFloat(op_stack, stack_pos)), op_stack, stack_pos);
      break;

    case ASIN_FLOAT:
      PushFloat(asin(PopFloat(op_stack, stack_pos)), op_stack, stack_pos);
      break;

    case ACOS_FLOAT:
      PushFloat(acos(PopFloat(op_stack, stack_pos)), op_stack, stack_pos);
      break;

    case ATAN_FLOAT:
      PushFloat(atan(PopFloat(op_stack, stack_pos)), op_stack, stack_pos);
      break;
      
    case ATAN2_FLOAT:
      right_double = PopFloat(op_stack, stack_pos);
      left_double = PopFloat(op_stack, stack_pos);
      PushFloat(atan2(left_double, right_double), op_stack, stack_pos);
      break;
      
    case LOG_FLOAT:
      PushFloat(log(PopFloat(op_stack, stack_pos)), op_stack, stack_pos);
      break;
      
    case POW_FLOAT:
      right_double = PopFloat(op_stack, stack_pos);
      left_double = PopFloat(op_stack, stack_pos);
      PushFloat(pow(left_double, right_double), op_stack, stack_pos);
      break;
      
    case SQRT_FLOAT:
      PushFloat(sqrt(PopFloat(op_stack, stack_pos)), op_stack, stack_pos);
      break;

    case RAND_FLOAT:
      PushFloat(GetRandomValue(), op_stack, stack_pos);
      break;

      // Note: no supported via JIT -- *end*

    case I2F:
#ifdef _DEBUG
      wcout << L"stack oper: I2F; call_pos=" << (*call_stack_pos) << endl;
#endif
      PushFloat((long)PopInt(op_stack, stack_pos), op_stack, stack_pos);
      break;

    case F2I:
#ifdef _DEBUG
      wcout << L"stack oper: F2I; call_pos=" << (*call_stack_pos) << endl;
#endif
      PushInt((long)PopFloat(op_stack, stack_pos), op_stack, stack_pos);
      break;

    case S2I:
      Str2Int(op_stack, stack_pos);
      break;
      
    case S2F:
      Str2Float(op_stack, stack_pos);
      break;

    case I2S:
      Int2Str(op_stack, stack_pos);
      break;
      
    case F2S:
      Float2Str(op_stack, stack_pos);
      break;
      
    case SWAP_INT:
#ifdef _DEBUG
      wcout << L"stack oper: SWAP_INT; call_pos=" << (*call_stack_pos) << endl;
#endif
      SwapInt(op_stack, stack_pos);
      break;

    case POP_INT:
#ifdef _DEBUG
      wcout << L"stack oper: PopInt; call_pos=" << (*call_stack_pos) << endl;
#endif
      PopInt(op_stack, stack_pos);
      break;

    case POP_FLOAT:
#ifdef _DEBUG
      wcout << L"stack oper: POP_FLOAT; call_pos=" << (*call_stack_pos) << endl;
#endif
      PopFloat(op_stack, stack_pos);
      break;

    case RTRN:
      ProcessReturn(instrs, ip);
      // return directly back to JIT code
      if((*frame) && (*frame)->jit_called) {
        (*frame)->jit_called = false;
        ReleaseStackFrame(*frame);
        return;
      }
      break;

    case DYN_MTHD_CALL:
      ProcessDynamicMethodCall(instr, instrs, ip, op_stack, stack_pos);
      // return directly back to JIT code
      if((*frame)->jit_called) {
        (*frame)->jit_called = false;
        ReleaseStackFrame(*frame);
        return;
      }
      break;

    case MTHD_CALL:
      ProcessMethodCall(instr, instrs, ip, op_stack, stack_pos);
      // return directly back to JIT code
      if((*frame)->jit_called) {
        (*frame)->jit_called = false;
        ReleaseStackFrame(*frame);
        return;
      }
      break;

    case JMP:
#ifdef _DEBUG
      wcout << L"stack oper: JMP; call_pos=" << (*call_stack_pos) << endl;
#endif
      if(!instr->GetOperand3()) {
        if(instr->GetOperand2() < 0) {
          ip = (*frame)->method->GetLabelIndex(instr->GetOperand()) + 1;
          instr->SetOperand3(ip);
        }
        else if((long)PopInt(op_stack, stack_pos) == instr->GetOperand2()) {
          ip = (*frame)->method->GetLabelIndex(instr->GetOperand()) + 1;
          instr->SetOperand3(ip);
        }
      }
      else {
        if(instr->GetOperand2() < 0) {
          ip = instr->GetOperand3();
        }
        else if((long)PopInt(op_stack, stack_pos) == instr->GetOperand2()) {
          ip = instr->GetOperand3();
        }
      }
      break;

    case OBJ_TYPE_OF:
      ObjTypeOf(instr, op_stack, stack_pos);
      break;

    case OBJ_INST_CAST:
      ObjInstCast(instr, op_stack, stack_pos);
      break;

    case ASYNC_MTHD_CALL:
      AsyncMthdCall(op_stack, stack_pos);
      break;

    case THREAD_JOIN:
      ThreadJoin(op_stack, stack_pos);
      break;

    case THREAD_MUTEX:
      ThreadMutex(op_stack, stack_pos);
      break;

    case CRITICAL_START:
      CriticalStart(op_stack, stack_pos);
      break;

    case CRITICAL_END:
      CriticalEnd(op_stack, stack_pos);
      break;

      //
      // End: Thread support
      // 

    case NEW_BYTE_ARY:
      ProcessNewByteArray(instr, op_stack, stack_pos);
      break;

    case NEW_CHAR_ARY:
      ProcessNewCharArray(instr, op_stack, stack_pos);
      break;
      
    case NEW_INT_ARY:
      ProcessNewArray(instr, op_stack, stack_pos);
      break;

    case NEW_FLOAT_ARY:
      ProcessNewArray(instr, op_stack, stack_pos, true);
      break;

    case NEW_OBJ_INST:
      ProcessNewObjectInstance(instr, op_stack, stack_pos);
      break;

    case NEW_FUNC_INST:
      ProcessNewFunctionInstance(instr, op_stack, stack_pos);
      break;

    case STOR_BYTE_ARY_ELM:
      ProcessStoreByteArrayElement(instr, op_stack, stack_pos);
      break;

    case STOR_CHAR_ARY_ELM:
      ProcessStoreCharArrayElement(instr, op_stack, stack_pos);
      break;
      
    case LOAD_BYTE_ARY_ELM:
      ProcessLoadByteArrayElement(instr, op_stack, stack_pos);
      break;
      
    case LOAD_CHAR_ARY_ELM:
      ProcessLoadCharArrayElement(instr, op_stack, stack_pos);
      break;

    case STOR_INT_ARY_ELM:
      ProcessStoreIntArrayElement(instr, op_stack, stack_pos);
      break;

    case LOAD_INT_ARY_ELM:
      ProcessLoadIntArrayElement(instr, op_stack, stack_pos);
      break;

    case STOR_FLOAT_ARY_ELM:
      ProcessStoreFloatArrayElement(instr, op_stack, stack_pos);
      break;

    case LOAD_FLOAT_ARY_ELM:
      ProcessLoadFloatArrayElement(instr, op_stack, stack_pos);
      break;

    case THREAD_SLEEP:
#ifdef _DEBUG
      wcout << L"stack oper: THREAD_SLEEP; call_pos=" << (*call_stack_pos) << endl;
#endif

#ifdef _WIN32
      left = (long)PopInt(op_stack, stack_pos);
      Sleep(left);
#else
      left = PopInt(op_stack, stack_pos);
      usleep(left * 1000);
#endif
      break;

    case LOAD_CLS_MEM:
#ifdef _DEBUG
      wcout << L"stack oper: LOAD_CLS_MEM; call_pos=" << (*call_stack_pos) << endl;
#endif
      PushInt((size_t)(*frame)->method->GetClass()->GetClassMemory(), op_stack, stack_pos);
      break;

    case LOAD_INST_MEM:
#ifdef _DEBUG
      wcout << L"stack oper: LOAD_INST_MEM; call_pos=" << (*call_stack_pos) << endl;
#endif
      PushInt((*frame)->mem[0], op_stack, stack_pos);
      break;

      // shared library support
    case DLL_LOAD:
      ProcessDllLoad(instr);
      break;

    case DLL_UNLOAD:
      ProcessDllUnload(instr);
      break;

    case DLL_FUNC_CALL:
      ProcessDllCall(instr, op_stack, stack_pos);
      break;
      
    case TRAP:
    case TRAP_RTRN:
#ifdef _DEBUG
      wcout << L"stack oper: TRAP; call_pos=" << (*call_stack_pos) << endl;
#endif
      if(!TrapProcessor::ProcessTrap(program, (size_t*)(*frame)->mem[0], op_stack, stack_pos, (*frame))) {
        StackErrorUnwind();
#ifdef _DEBUGGER
        halt = true;
        return;
#else
        exit(1);
#endif
      }
      break;

      // note: just for debugger
    case END_STMTS:
      break;

    default:
      break;
    }
  }
  while(!halt);
  
#ifdef _TIMING
  clock_t end = clock();
  wcout << L"---------------------------" << endl;
  wcout << L"Dispatch method='" << mthd_name << L"', time=" << (double)(end - start) / CLOCKS_PER_SEC << L" second(s)." << endl;
#endif
}

void StackInterpreter::StorLoclIntVar(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: STOR_LOCL_INT_VAR; index=" << instr->GetOperand() << endl;
#endif
  size_t* mem = (*frame)->mem;
  mem[instr->GetOperand() + 1] = PopInt(op_stack, stack_pos);
}

void StackInterpreter::StorClsInstIntVar(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: STOR_CLS_INST_INT_VAR; index=" << instr->GetOperand() << endl;
#endif
  size_t* cls_inst_mem = (size_t*)PopInt(op_stack, stack_pos);
  if(!cls_inst_mem) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  size_t mem = PopInt(op_stack, stack_pos);
  cls_inst_mem[instr->GetOperand()] = mem;
}

void StackInterpreter::CopyLoclIntVar(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: COPY_LOCL_INT_VAR; index=" << instr->GetOperand() << endl;
#endif
  size_t* mem = (*frame)->mem;
  mem[instr->GetOperand() + 1] = TopInt(op_stack, stack_pos);
}

void StackInterpreter::CopyClsInstIntVar(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: COPY_CLS_INST_INT_VAR; index=" << instr->GetOperand() << endl;
#endif

  size_t* cls_inst_mem = (size_t*)PopInt(op_stack, stack_pos);
  if(!cls_inst_mem) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  cls_inst_mem[instr->GetOperand()] = TopInt(op_stack, stack_pos);
}

void StackInterpreter::Str2Int(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: S2I; call_pos=" << (*call_stack_pos) << endl;
#endif

  size_t* str_ptr = (size_t*)PopInt(op_stack, stack_pos);
  long base = (long)PopInt(op_stack, stack_pos);
  if(str_ptr) {
    wchar_t* str = (wchar_t*)(str_ptr + 3);
    try {
      if(wcslen(str) > 2) {
        switch(str[1]) {
          // binary
        case 'b':
          PushInt(stoi(str + 2, nullptr, 2), op_stack, stack_pos);
          return;

          // octal
        case 'o':
          PushInt(stoi(str + 2, nullptr, 8), op_stack, stack_pos);
          return;

          // hexadecimal
        case 'x':
          PushInt(stoi(str + 2, nullptr, 16), op_stack, stack_pos);
          return;

        default:
          break;
        }
      }
      PushInt(stoi(str, nullptr, base), op_stack, stack_pos);
    }
    catch(std::invalid_argument &e) {
#ifdef _WIN32    
      UNREFERENCED_PARAMETER(e);
#endif
      PushInt(0, op_stack, stack_pos);
    }
  }
  else {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
}

void StackInterpreter::Str2Float(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: S2F; call_pos=" << (*call_stack_pos) << endl;
#endif
  
  size_t* str_ptr = (size_t*)PopInt(op_stack, stack_pos);
  if(str_ptr) {
    wchar_t* str = (wchar_t*)(str_ptr + 3);
    wstringstream stream(str);
    FLOAT_VALUE value;
    stream >> value;
    PushFloat(value, op_stack, stack_pos);
  }
  else {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
}

void StackInterpreter::Int2Str(size_t* &op_stack, long* &stack_pos)
{
  size_t* str_ptr = (size_t*)PopInt(op_stack, stack_pos);
  if(str_ptr) {
    wchar_t* str = (wchar_t*)(str_ptr + 3);
    const size_t base = PopInt(op_stack, stack_pos);
    const long value = (long)PopInt(op_stack, stack_pos);
    
    wstringstream stream;
    if(base == 16) {
      stream << std::hex << value;
      wstring conv(stream.str());
      const size_t max = conv.size() < 16 ? conv.size() : 16; 
#ifdef _WIN32
      wcsncpy_s(str, str_ptr[0], conv.c_str(), max);
#else
      wcsncpy(str, conv.c_str(), max);
#endif
    }
    else {
      stream << value;
      wstring conv(stream.str());
      const size_t max = conv.size() < 16 ? conv.size() : 16; 
#ifdef _WIN32
      wcsncpy_s(str, str_ptr[0], conv.c_str(), max);
#else
      wcsncpy(str, conv.c_str(), max);
#endif
    }
  }
}

void inline StackInterpreter::Float2Str(size_t* &op_stack, long* &stack_pos)
{
  size_t* str_ptr = (size_t*)PopInt(op_stack, stack_pos);
  if(str_ptr) {
    wchar_t* str = (wchar_t*)(str_ptr + 3);
    const double value = PopFloat(op_stack, stack_pos);
    
    wstringstream stream;
    stream << value;
    wstring conv(stream.str());
    const size_t max = conv.size() < 16 ? conv.size() : 16; 
#ifdef _WIN32
    wcsncpy_s(str, str_ptr[0], conv.c_str(), max);
#else
    wcsncpy(str, conv.c_str(), max);
#endif
  }
}

void StackInterpreter::ShlInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: SHL_INT; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left << right, op_stack, stack_pos);
}

void StackInterpreter::ShrInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: SHR_INT; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left >> right, op_stack, stack_pos);
}

void StackInterpreter::LoadLoclIntVar(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LOAD_LOCL_INT_VAR; index=" << instr->GetOperand() << endl;
#endif
  size_t* mem = (*frame)->mem;
  PushInt(mem[instr->GetOperand() + 1], op_stack, stack_pos);
}

void StackInterpreter::LoadClsInstIntVar(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LOAD_CLS_INST_INT_VAR; index=" << instr->GetOperand() << endl;
#endif      
  size_t* cls_inst_mem = (size_t*)PopInt(op_stack, stack_pos);
  if(!cls_inst_mem) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  PushInt(cls_inst_mem[instr->GetOperand()], op_stack, stack_pos);
}

void StackInterpreter::AndInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: AND; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left && right, op_stack, stack_pos);
}

void StackInterpreter::OrInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: OR; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left || right, op_stack, stack_pos);
}

void StackInterpreter::AddInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: ADD; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left + right, op_stack, stack_pos);
}

void StackInterpreter::AddFloat(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: ADD; call_pos=" << (*call_stack_pos) << endl;
#endif
  const double left_double = PopFloat(op_stack, stack_pos);
  const double right_double = PopFloat(op_stack, stack_pos);
  PushFloat(left_double + right_double, op_stack, stack_pos);
}

void StackInterpreter::SubInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: SUB; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left - right, op_stack, stack_pos);
}

void StackInterpreter::SubFloat(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: SUB; call_pos=" << (*call_stack_pos) << endl;
#endif
  const double left_double = PopFloat(op_stack, stack_pos);
  const double right_double = PopFloat(op_stack, stack_pos);
  PushFloat(left_double - right_double, op_stack, stack_pos);
}

void StackInterpreter::MulInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: MUL; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left * right, op_stack, stack_pos);
}

void StackInterpreter::DivInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: DIV; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left / right, op_stack, stack_pos);
}

void StackInterpreter::MulFloat(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: MUL; call_pos=" << (*call_stack_pos) << endl;
#endif
  const double left_double = PopFloat(op_stack, stack_pos);
  const double right_double = PopFloat(op_stack, stack_pos);
  PushFloat(left_double * right_double, op_stack, stack_pos);
}

void StackInterpreter::DivFloat(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: DIV; call_pos=" << (*call_stack_pos) << endl;
#endif
  const double left_double = PopFloat(op_stack, stack_pos);
  const double right_double = PopFloat(op_stack, stack_pos);
  PushFloat(left_double / right_double, op_stack, stack_pos);
}

void StackInterpreter::ModInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: MOD; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left % right, op_stack, stack_pos);
}

void StackInterpreter::BitAndInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: BIT_AND; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left & right, op_stack, stack_pos);
}

void StackInterpreter::BitOrInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: BIT_OR; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left | right, op_stack, stack_pos);
}

void StackInterpreter::BitXorInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: BIT_XOR; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left ^ right, op_stack, stack_pos);
}

void StackInterpreter::LesEqlInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LES_EQL; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left <= right, op_stack, stack_pos);
}

void StackInterpreter::GtrEqlInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: GTR_EQL; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left >= right, op_stack, stack_pos);
}

void StackInterpreter::LesEqlFloat(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LES_EQL; call_pos=" << (*call_stack_pos) << endl;
#endif
  const double left_double = PopFloat(op_stack, stack_pos);
  const double right_double = PopFloat(op_stack, stack_pos);
  PushInt(left_double <= right_double, op_stack, stack_pos);
}

void StackInterpreter::GtrEqlFloat(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: GTR_EQL; call_pos=" << (*call_stack_pos) << endl;
#endif
  const double left_double = PopFloat(op_stack, stack_pos);
  const double right_double = PopFloat(op_stack, stack_pos);
  PushInt(left_double >= right_double, op_stack, stack_pos);
}

void StackInterpreter::EqlInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: EQL; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left == right, op_stack, stack_pos);
}

void StackInterpreter::NeqlInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: NEQL; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left != right, op_stack, stack_pos);
}

void StackInterpreter::LesInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LES; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left < right, op_stack, stack_pos);
}

void StackInterpreter::GtrInt(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: GTR; call_pos=" << (*call_stack_pos) << endl;
#endif
  const long left = (long)PopInt(op_stack, stack_pos);
  const long right = (long)PopInt(op_stack, stack_pos);
  PushInt(left > right, op_stack, stack_pos);
}

void StackInterpreter::EqlFloat(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: EQL; call_pos=" << (*call_stack_pos) << endl;
#endif
  const double left_double = PopFloat(op_stack, stack_pos);
  const double right_double = PopFloat(op_stack, stack_pos);
  PushInt(left_double == right_double, op_stack, stack_pos);
}

void StackInterpreter::NeqlFloat(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: NEQL; call_pos=" << (*call_stack_pos) << endl;
#endif
  const double left_double = PopFloat(op_stack, stack_pos);
  const double right_double = PopFloat(op_stack, stack_pos);
  PushInt(left_double != right_double, op_stack, stack_pos);
}

void StackInterpreter::LesFloat(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LES; call_pos=" << (*call_stack_pos) << endl;
#endif
  const double left_double = PopFloat(op_stack, stack_pos);
  const double right_double = PopFloat(op_stack, stack_pos);
  PushInt(left_double < right_double, op_stack, stack_pos);
}

void StackInterpreter::GtrFloat(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: GTR_FLOAT; call_pos=" << (*call_stack_pos) << endl;
#endif
  const double left_double = PopFloat(op_stack, stack_pos);
  const double right_double = PopFloat(op_stack, stack_pos);
  PushInt(left_double > right_double, op_stack, stack_pos);
}

void StackInterpreter::LoadArySize(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LOAD_ARY_SIZE; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* array = (size_t*)PopInt(op_stack, stack_pos);
  if(!array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  PushInt(array[2], op_stack, stack_pos);
}

void StackInterpreter::CpyByteAry(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: CPY_BYTE_ARY; call_pos=" << (*call_stack_pos) << endl;
#endif
  long length = (long)PopInt(op_stack, stack_pos);
  const long src_offset = (long)PopInt(op_stack, stack_pos);
  size_t* src_array = (size_t*)PopInt(op_stack, stack_pos);
  const long dest_offset = (long)PopInt(op_stack, stack_pos);
  size_t* dest_array = (size_t*)PopInt(op_stack, stack_pos);

  if(!src_array || !dest_array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }

  const long src_array_len = (long)src_array[2];
  const long dest_array_len = (long)dest_array[2];
  if(length > 0 && src_offset + length <= src_array_len && dest_offset + length <= dest_array_len) {
    const char* src_array_ptr = (char*)(src_array + 3);
    char* dest_array_ptr = (char*)(dest_array + 3);
    ::memcpy(dest_array_ptr + dest_offset, src_array_ptr + src_offset, length);
    PushInt(1, op_stack, stack_pos);
  }
  else {
    PushInt(0, op_stack, stack_pos);
  }
}

void StackInterpreter::CpyCharAry(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: CPY_CHAR_ARY; call_pos=" << (*call_stack_pos) << endl;
#endif
  long length = (long)PopInt(op_stack, stack_pos);
  const long src_offset = (long)PopInt(op_stack, stack_pos);
  size_t* src_array = (size_t*)PopInt(op_stack, stack_pos);
  const long dest_offset = (long)PopInt(op_stack, stack_pos);
  size_t* dest_array = (size_t*)PopInt(op_stack, stack_pos);

  if(!src_array || !dest_array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }

  const long src_array_len = (long)src_array[2];
  const long dest_array_len = (long)dest_array[2];
  if(length > 0 && src_offset + length <= src_array_len && dest_offset + length <= dest_array_len) {
    wchar_t* src_array_ptr = (wchar_t*)(src_array + 3);
    wchar_t* dest_array_ptr = (wchar_t*)(dest_array + 3);
    ::memcpy(dest_array_ptr + dest_offset, src_array_ptr + src_offset, length * sizeof(wchar_t));
    PushInt(1, op_stack, stack_pos);
  }
  else {
    PushInt(0, op_stack, stack_pos);
  }
}

void StackInterpreter::CpyIntAry(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: CPY_INT_ARY; call_pos=" << (*call_stack_pos) << endl;
#endif
  long length = (long)PopInt(op_stack, stack_pos);
  const long src_offset = (long)PopInt(op_stack, stack_pos);
  size_t* src_array = (size_t*)PopInt(op_stack, stack_pos);
  const long dest_offset = (long)PopInt(op_stack, stack_pos);
  size_t* dest_array = (size_t*)PopInt(op_stack, stack_pos);

  if(!src_array || !dest_array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }

  const long src_array_len = (long)src_array[0];
  const long dest_array_len = (long)dest_array[0];
  if(length > 0 && src_offset + length <= src_array_len && dest_offset + length <= dest_array_len) {
    size_t* src_array_ptr = src_array + 3;
    size_t* dest_array_ptr = dest_array + 3;
    ::memcpy(dest_array_ptr + dest_offset, src_array_ptr + src_offset, length * sizeof(size_t));
    PushInt(1, op_stack, stack_pos);
  }
  else {
    PushInt(0, op_stack, stack_pos);
  }
}

void StackInterpreter::CpyFloatAry(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: CPY_FLOAT_ARY; call_pos=" << (*call_stack_pos) << endl;
#endif
  long length = (long)PopInt(op_stack, stack_pos);
  const long src_offset = (long)PopInt(op_stack, stack_pos);
  size_t* src_array = (size_t*)PopInt(op_stack, stack_pos);
  const long dest_offset = (long)PopInt(op_stack, stack_pos);
  size_t* dest_array = (size_t*)PopInt(op_stack, stack_pos);

  if(!src_array || !dest_array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }

  const long src_array_len = (long)src_array[0];
  const long dest_array_len = (long)dest_array[0];
  if(length > 0 && src_offset + length <= src_array_len && dest_offset + length <= dest_array_len) {
    size_t* src_array_ptr = src_array + 3;
    size_t* dest_array_ptr = dest_array + 3;
    ::memcpy(dest_array_ptr + dest_offset, src_array_ptr + src_offset, length * sizeof(FLOAT_VALUE));
    PushInt(1, op_stack, stack_pos);
  }
  else {
    PushInt(0, op_stack, stack_pos);
  }
}

void StackInterpreter::ObjTypeOf(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
  size_t* mem = (size_t*)PopInt(op_stack, stack_pos);
  if(mem) {
    size_t* result = MemoryManager::ValidObjectCast(mem, instr->GetOperand(),
                                                    program->GetHierarchy(),
                                                    program->GetInterfaces());
    if(result) {
      PushInt(1, op_stack, stack_pos);
    }
    else {
      PushInt(0, op_stack, stack_pos);
    }
  }
  else {
    wcerr << L">>> TypeOf(..) check on Nil value <<<" << endl;
    StackErrorUnwind();
    exit(1);
  }
}

void StackInterpreter::ObjInstCast(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
  size_t* mem = (size_t*)PopInt(op_stack, stack_pos);
  size_t result = (size_t)MemoryManager::ValidObjectCast(mem, instr->GetOperand(),
                                                         program->GetHierarchy(),
                                                         program->GetInterfaces());
#ifdef _DEBUG
  wcout << L"stack oper: OBJ_INST_CAST: from=" << mem << L", to=" << instr->GetOperand() << endl;
#endif
  if(!result && mem) {
    StackClass* to_cls = MemoryManager::GetClass((size_t*)mem);
    wcerr << L">>> Invalid object cast: '" << (to_cls ? to_cls->GetName() : L"?")
          << L"' to '" << program->GetClass(instr->GetOperand())->GetName() << L"' <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  PushInt(result, op_stack, stack_pos);
}

void StackInterpreter::AsyncMthdCall(size_t* &op_stack, long* &stack_pos)
{
  size_t* instance = (size_t*)(*frame)->mem[0];
  size_t* param = (size_t*)(*frame)->mem[1];

  StackClass* impl_class = MemoryManager::GetClass(instance);
  if(!impl_class) {
    PopFrame();
    wcerr << L">>> Atempting to dereference a 'Nil' memory element <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }

  const wstring& mthd_name = impl_class->GetName() + L":Run:o.System.Base,";
  StackMethod* called = impl_class->GetMethod(mthd_name);
#ifdef _DEBUG
  assert(called);
  wcout << L"=== ASYNC_MTHD_CALL: id=" << called->GetClass()->GetId() << L","
        << called->GetId() << L"; name='" << called->GetName()
        << L"'; param=" << param << L" ===" << endl;
#endif

  // create and execute the new thread
  // make sure that calls to the model are synced.  Are find method synced?
  ProcessAsyncMethodCall(called, param);
}

void StackInterpreter::ThreadJoin(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: THREAD_JOIN; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* instance = (size_t*)(*frame)->mem[0];
  if(!instance) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }

#ifdef _WIN32
  HANDLE vm_thread = (HANDLE)instance[0];
  if(WaitForSingleObject(vm_thread, INFINITE) != WAIT_OBJECT_0) {
    wcerr << L">>> Unable to join thread! <<<" << endl;
#ifdef _DEBUGGER
    return;
#else
    exit(1);
#endif
  }
#else
  void* status;
  pthread_t vm_thread = (pthread_t)instance[0];
  if(pthread_join(vm_thread, &status)) {
    wcerr << L">>> Unable to join thread! <<<" << endl;
#ifdef _DEBUGGER
    return;
#else
    exit(1);
#endif
  }
#endif
}

void StackInterpreter::ThreadMutex(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: THREAD_MUTEX; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* instance = (size_t*)(*frame)->mem[0];
  if(!instance) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
#ifdef _WIN32
  InitializeCriticalSection((CRITICAL_SECTION*)&instance[1]);
#else
  pthread_mutex_init((pthread_mutex_t*)&instance[1], nullptr);
#endif
}

void StackInterpreter::CriticalStart(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: CRITICAL_START; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* instance = (size_t*)PopInt(op_stack, stack_pos);
  if(!instance) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif        
  }
#ifdef _WIN32
  EnterCriticalSection((CRITICAL_SECTION*)&instance[1]);
#else
  pthread_mutex_lock((pthread_mutex_t*)&instance[1]);
#endif
}

void StackInterpreter::CriticalEnd(size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: CRITICAL_END; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* instance = (size_t*)PopInt(op_stack, stack_pos);
  if(!instance) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
#ifdef _WIN32
  LeaveCriticalSection((CRITICAL_SECTION*)&instance[1]);
#else
  pthread_mutex_unlock((pthread_mutex_t*)&instance[1]);
#endif
}

/********************************
 * Processes a load function
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessLoadFunction(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LOAD_FUNC_VAR; index=" << instr->GetOperand()
        << L"; local=" << ((instr->GetOperand2() == LOCL) ? "true" : "false") << endl;
#endif
  if(instr->GetOperand2() == LOCL) {
    size_t* mem = (*frame)->mem;
    PushInt(mem[instr->GetOperand() + 2], op_stack, stack_pos);
    PushInt(mem[instr->GetOperand() + 1], op_stack, stack_pos);
  } 
  else {
    size_t* cls_inst_mem = (size_t*)PopInt(op_stack, stack_pos);
    if(!cls_inst_mem) {
      wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
      StackErrorUnwind();
#ifdef _DEBUGGER
      halt = true;
      return;
#else
      exit(1);
#endif
    }
    PushInt(cls_inst_mem[instr->GetOperand() + 1], op_stack, stack_pos);
    PushInt(cls_inst_mem[instr->GetOperand()], op_stack, stack_pos);
  }
}

/********************************
 * Processes a load float
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessLoadFloat(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LOAD_FLOAT_VAR; index=" << instr->GetOperand()
        << L"; local=" << ((instr->GetOperand2() == LOCL) ? "true" : "false") << endl;
#endif
  FLOAT_VALUE value;
  if(instr->GetOperand2() == LOCL) {
    size_t* mem = (*frame)->mem;
    // ::memcpy(&value, &mem[instr->GetOperand() + 1], sizeof(FLOAT_VALUE));
    value = *((FLOAT_VALUE*)(&mem[instr->GetOperand() + 1]));

  } else {
    size_t* cls_inst_mem = (size_t*)PopInt(op_stack, stack_pos);
    if(!cls_inst_mem) {
      wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
      StackErrorUnwind();
#ifdef _DEBUGGER
      halt = true;
      return;
#else
      exit(1);
#endif
    }
    // ::memcpy(&value, &cls_inst_mem[instr->GetOperand()], sizeof(FLOAT_VALUE));
    value = *((FLOAT_VALUE*)(&cls_inst_mem[instr->GetOperand()]));
  }
  PushFloat(value, op_stack, stack_pos);
}

/********************************
 * Processes a store function
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessStoreFunction(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: STOR_FUNC_VAR; index=" << instr->GetOperand()
        << L"; local=" << ((instr->GetOperand2() == LOCL) ? "true" : "false") << endl;
#endif
  if(instr->GetOperand2() == LOCL) {
    size_t* mem = (*frame)->mem;
    mem[instr->GetOperand() + 1] = PopInt(op_stack, stack_pos);
    mem[instr->GetOperand() + 2] = PopInt(op_stack, stack_pos);
  } 
  else {
    size_t* cls_inst_mem = (size_t*)PopInt(op_stack, stack_pos);
    if(!cls_inst_mem) {
      wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
      StackErrorUnwind();
#ifdef _DEBUGGER
      halt = true;
      return;
#else
      exit(1);
#endif
    }
    cls_inst_mem[instr->GetOperand()] = PopInt(op_stack, stack_pos);
    cls_inst_mem[instr->GetOperand() + 1] = PopInt(op_stack, stack_pos);
  }
}

/********************************
 * Processes a store float
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessStoreFloat(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: STOR_FLOAT_VAR; index=" << instr->GetOperand()
        << L"; local=" << ((instr->GetOperand2() == LOCL) ? "true" : "false") << endl;
#endif
  if(instr->GetOperand2() == LOCL) {
    const FLOAT_VALUE value = PopFloat(op_stack, stack_pos);
    size_t* mem = (*frame)->mem;
    // ::memcpy(&mem[instr->GetOperand() + 1], &value, sizeof(FLOAT_VALUE));
    *((FLOAT_VALUE*)(&mem[instr->GetOperand() + 1])) = value;
  } 
  else {
    size_t* cls_inst_mem = (size_t*)PopInt(op_stack, stack_pos);
    if(!cls_inst_mem) {
      wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
      StackErrorUnwind();
#ifdef _DEBUGGER
      halt = true;
      return;
#else
      exit(1);
#endif
    }
    const FLOAT_VALUE value = PopFloat(op_stack, stack_pos);
    // ::memcpy(&cls_inst_mem[instr->GetOperand()], &value, sizeof(FLOAT_VALUE));
    *((FLOAT_VALUE*)(&cls_inst_mem[instr->GetOperand()])) = value;
  }
}

/********************************
 * Processes a copy float
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessCopyFloat(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: COPY_FLOAT_VAR; index=" << instr->GetOperand()
        << L"; local=" << ((instr->GetOperand2() == LOCL) ? "true" : "false") << endl;
#endif
  if(instr->GetOperand2() == LOCL) {
    FLOAT_VALUE value = TopFloat(op_stack, stack_pos);
    size_t* mem = (*frame)->mem;
    // ::memcpy(&mem[instr->GetOperand() + 1], &value, sizeof(FLOAT_VALUE));
    *((FLOAT_VALUE*)(&mem[instr->GetOperand() + 1])) = value;
  } else {
    size_t* cls_inst_mem = (size_t*)PopInt(op_stack, stack_pos);
    if(!cls_inst_mem) {
      wcerr << L">>> Atempting to dereference a 'Nil' memory instance <<<" << endl;
      StackErrorUnwind();
#ifdef _DEBUGGER
      halt = true;
      return;
#else
      exit(1);
#endif
    }
    FLOAT_VALUE value = TopFloat(op_stack, stack_pos);
    // ::memcpy(&cls_inst_mem[instr->GetOperand()], &value, sizeof(FLOAT_VALUE));
    *((FLOAT_VALUE*)(&cls_inst_mem[instr->GetOperand()])) = value;
  }
}

/********************************
 * Processes a new object instance
 * request.
 ********************************/
void StackInterpreter::ProcessNewObjectInstance(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: NEW_OBJ_INST: id=" << instr->GetOperand() << endl;
#endif

  size_t inst_mem = (size_t)MemoryManager::AllocateObject(instr->GetOperand(),
                                                          op_stack, *stack_pos);
  PushInt(inst_mem, op_stack, stack_pos);
}

/********************************
 * Processes a new object instance
 * request.
 ********************************/
void StackInterpreter::ProcessNewFunctionInstance(StackInstr* instr, size_t*& op_stack, long*& stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: NEW_FUNC_INST: mem_size=" << instr->GetOperand() << endl;
#endif

  size_t func_mem = (size_t)MemoryManager::AllocateArray(instr->GetOperand(), BYTE_ARY_TYPE, op_stack, *stack_pos);
  PushInt(func_mem, op_stack, stack_pos);
}

/********************************
 * Processes a new array instance
 * request.
 ********************************/
void StackInterpreter::ProcessNewArray(StackInstr* instr, size_t* &op_stack, long* &stack_pos, bool is_float)
{
#ifdef _DEBUG
  wcout << L"stack oper: NEW_INT_ARY/NEW_FLOAT_ARY; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t indices[8];
  size_t value = PopInt(op_stack, stack_pos);
  size_t size = value;
  indices[0] = value;
  long dim = 1;
  for(long i = 1; i < instr->GetOperand(); i++) {
    size_t value = PopInt(op_stack, stack_pos);
    size *= value;
    indices[dim++] = value;
  }

  size_t* mem;  
#if defined(_WIN64) || defined(_X64)
  mem = (size_t*)MemoryManager::AllocateArray((long)(size + dim + 2), INT_TYPE, op_stack, *stack_pos);
#else
  if(is_float) {
    // doubles are twice the size of integers for 32-bit target
    mem = (size_t*)MemoryManager::AllocateArray(size + dim + 2, FLOAT_TYPE, op_stack, *stack_pos);
  }
  else {
    mem = (size_t*)MemoryManager::AllocateArray(size + dim + 2, INT_TYPE, op_stack, *stack_pos);
  }
#endif

  mem[0] = size;
  mem[1] = dim;

  ::memcpy(mem + 2, indices, dim * sizeof(size_t));
  PushInt((size_t)mem, op_stack, stack_pos);
}

/********************************
 * Processes a new byte array instance
 * request.
 ********************************/
void StackInterpreter::ProcessNewByteArray(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: NEW_BYTE_ARY; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t indices[8];
  size_t value = PopInt(op_stack, stack_pos);
  size_t size = value;
  indices[0] = value;
  long dim = 1;
  for(long i = 1; i < instr->GetOperand(); i++) {
    size_t value = PopInt(op_stack, stack_pos);
    size *= value;
    indices[dim++] = value;
  }

  // nullptr terminated string 
  size++;
  size_t* mem = MemoryManager::AllocateArray((long)(size + ((dim + 2) * sizeof(size_t))), BYTE_ARY_TYPE, op_stack, *stack_pos);
  mem[0] = size - 1;
  mem[1] = dim;
  ::memcpy(mem + 2, indices, dim * sizeof(size_t));
  PushInt((size_t)mem, op_stack, stack_pos);
}

/********************************
 * Processes a new char array instance
 * request.
 ********************************/
void StackInterpreter::ProcessNewCharArray(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: NEW_CHAR_ARY; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t indices[8];
  size_t value = PopInt(op_stack, stack_pos);
  size_t size = value;
  indices[0] = value;
  long dim = 1;
  for(long i = 1; i < instr->GetOperand(); i++) {
    size_t value = PopInt(op_stack, stack_pos);
    size *= value;
    indices[dim++] = value;
  }

  // nullptr terminated string 
  size++;
  size_t* mem = MemoryManager::AllocateArray((long)(size + ((dim + 2) * sizeof(size_t))), CHAR_ARY_TYPE, op_stack, *stack_pos);
  mem[0] = size - 1;
  mem[1] = dim;
  ::memcpy(mem + 2, indices, dim * sizeof(size_t));
  PushInt((size_t)mem, op_stack, stack_pos);
}

/********************************
 * Processes a return instruction.
 * This instruction modifies the
 * call stack.
 ********************************/
void StackInterpreter::ProcessReturn(StackInstr** &instrs, long &ip)
{
#ifdef _DEBUG
  wcout << L"stack oper: RTRN; call_pos=" << (*call_stack_pos) << endl;
#endif

  // unregister old frame
#ifdef _DEBUG
  wcout << L"removing frame=" << (*frame) << endl;
#endif
  
  ReleaseStackFrame(*frame);
  
  // restore previous frame
  if(!StackEmpty()) {
    (*frame) = PopFrame();
    instrs = (*frame)->method->GetInstructions();
    ip = (*frame)->ip;
  } 
  else {
    (*frame) = nullptr;
    halt = true;
  }
}

/********************************
 * Processes a asynchronous
 * method call.
 ********************************/
void StackInterpreter::ProcessAsyncMethodCall(StackMethod* called, size_t* param)
{
  size_t* instance = (size_t*)(*frame)->mem[0];
  ThreadHolder* holder = new ThreadHolder;
  holder->called = called;
  holder->self = instance;
  holder->param = param;

#ifdef _WIN32
  HANDLE vm_thread = (HANDLE)_beginthreadex(nullptr, 0, AsyncMethodCall, holder, 0, nullptr);
  if(!vm_thread) {
    wcerr << L">>> Internal error: Unable to create garbage collection thread! <<<" << endl;
    exit(-1);
  }
#else
  pthread_attr_t attrs;
  pthread_attr_init(&attrs);
  pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);

  // execute thread
  pthread_t vm_thread;
  if(pthread_create(&vm_thread, &attrs, AsyncMethodCall, (void*)holder)) {
    wcerr << L">>> Internal error: Internal error: Unable to create runtime thread! <<<" << endl;
    exit(-1);
  }
#endif  
  
  // assign thread ID
  if(!instance) {
    wcerr << L">>> Internal error: Unable to create runtime thread! <<<" << endl;
    exit(-1);
  }

  instance[0] = (size_t)vm_thread;
#ifdef _DEBUG
  wcout << L"*** New Thread ID: " << vm_thread  << L": " << instance << L" ***" << endl;
#endif
}

#ifdef _WIN32
//
// windows thread callback
//
unsigned int WINAPI StackInterpreter::AsyncMethodCall(LPVOID arg)
{
  ThreadHolder* holder = (ThreadHolder*)arg;

  // execute
  size_t* thread_op_stack = new size_t[OP_STACK_SIZE];
  long* thread_stack_pos = new long;
  (*thread_stack_pos) = 0;
  
  // set parameter
  thread_op_stack[(*thread_stack_pos)++] = (size_t)holder->param;

  HANDLE vm_thread;
  DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
                  &vm_thread, 0, TRUE, DUPLICATE_SAME_ACCESS);

#ifdef _DEBUG
  wcout << L"# Starting thread=" << vm_thread << L" #" << endl;
#endif  

  Runtime::StackInterpreter* intpr = new Runtime::StackInterpreter;
  AddThread(intpr);
  intpr->Execute(thread_op_stack, thread_stack_pos, 0, holder->called, holder->self, false);
  
#ifdef _DEBUG
  wcout << L"# final stack: pos=" << (*thread_stack_pos) << L", thread=" << vm_thread << L" #" << endl;
#endif

  // clean up
  delete[] thread_op_stack;
  thread_op_stack = nullptr;

  delete thread_stack_pos;
  thread_stack_pos = nullptr;
  
  RemoveThread(intpr);
  delete intpr;
  intpr = nullptr;

  delete holder;
  holder = nullptr;
  
  return 0;
}
#else
//
// posix thread callback
//
void* StackInterpreter::AsyncMethodCall(void* arg)
{
  ThreadHolder* holder = (ThreadHolder*)arg;

  // execute
  size_t* thread_op_stack = new size_t[OP_STACK_SIZE];
  long* thread_stack_pos = new long;
  (*thread_stack_pos) = 0;

  // set parameter
  thread_op_stack[(*thread_stack_pos)++] = (size_t)holder->param;

#ifdef _DEBUG
  wcout << L"# Starting thread=" << pthread_self() << L" #" << endl;
#endif  

  Runtime::StackInterpreter* intpr = new Runtime::StackInterpreter;
  AddThread(intpr);
  intpr->Execute(thread_op_stack, thread_stack_pos, 0, holder->called, holder->self, false);

#ifdef _DEBUG
  wcout << L"# final stack: pos=" << (*thread_stack_pos) << L", thread=" << pthread_self() << L" #" << endl;
#endif
  
  // clean up
  delete[] thread_op_stack;
  thread_op_stack = nullptr;

  delete thread_stack_pos;
  thread_stack_pos = nullptr;

  RemoveThread(intpr);
  delete intpr;
  intpr = nullptr;
  
  delete holder;
  holder = nullptr;
  
  return nullptr;
}
#endif

/********************************
 * Processes a synchronous
 * dynamic method call.
 ********************************/
void StackInterpreter::ProcessDynamicMethodCall(StackInstr* instr, StackInstr** &instrs, long &ip, size_t* &op_stack, long* &stack_pos)
{
  // save current method
  (*frame)->ip = ip;
  PushFrame((*frame));

  // make call
  const size_t mthd_cls_id = PopInt(op_stack, stack_pos);
  const long cls_id = (mthd_cls_id >> (16 * (0))) & 0xFFFF;
  const long mthd_id = (mthd_cls_id >> (16 * (1))) & 0xFFFF;

  // pop instance
  size_t* instance = (size_t*)PopInt(op_stack, stack_pos);

#ifdef _DEBUG
  wcout << L"stack oper: DYN_MTHD_CALL; cls_mtd_id=" << cls_id << L"," << mthd_id << endl;
#endif
  StackMethod* called = program->GetClass(cls_id)->GetMethod(mthd_id);
#ifdef _DEBUG
  wcout << L"=== Binding function call: to: '" << called->GetName() << L"' ===" << endl;
#endif

#ifndef _NO_JIT
  // execute JIT call
  if(instr->GetOperand3()) {
    ProcessJitMethodCall(called, instance, instrs, ip, op_stack, stack_pos);
  }
  // execute interpreter
  else {
    (*frame) = GetStackFrame(called, instance);    
    instrs = (*frame)->method->GetInstructions();
    ip = 0;
  }
#else
  ProcessInterpretedMethodCall(called, instance, instrs, ip);
#endif
}

/********************************
 * Processes a synchronous method
 * call.
 ********************************/
void StackInterpreter::ProcessMethodCall(StackInstr* instr, StackInstr** &instrs, long &ip, size_t* &op_stack, long* &stack_pos)
{
  // save current method
  (*frame)->ip = ip;
  PushFrame((*frame));

  // pop instance
  size_t* instance = (size_t*)PopInt(op_stack, stack_pos);

  // make call
  StackMethod* called = program->GetClass(instr->GetOperand())->GetMethod(instr->GetOperand2());

  // dynamically bind class for virtual method
  if(called->IsVirtual()) {
    StackClass* impl_class = MemoryManager::GetClass((size_t*)instance);
    if(!impl_class) {
      PopFrame();
      wcerr << L">>> Atempting to dereference a 'Nil' memory element <<<" << endl;
      StackErrorUnwind();
#ifdef _DEBUGGER
      halt = true;
      return;
#else
      exit(1);
#endif
    }
    
#ifdef _DEBUG
    wcout << L"=== Binding virtual method call: from: '" << called->GetName();
#endif

    // binding method
    const wstring qualified_method_name = called->GetName();
    const wstring method_ending = qualified_method_name.substr(qualified_method_name.find(L':'));
    wstring method_name = impl_class->GetName() + method_ending;

    // check method cache
    called = StackMethod::GetVirtualEntry(method_name);
    if(!called) {
      called = impl_class->GetMethod(method_name);
      while(!called) {
        impl_class = program->GetClass(impl_class->GetParentId());
        method_name = impl_class->GetName() + method_ending;
        called = program->GetClass(impl_class->GetId())->GetMethod(method_name);
      }
      // add cache entry
      StackMethod::AddVirtualEntry(method_name, called);
    }

#ifdef _DEBUG
    wcout << L"'; to: '" << method_name << L"' ===" << endl;
#endif
  }

#ifndef _NO_JIT
  // execute JIT call
  if(instr->GetOperand3()) {
    ProcessJitMethodCall(called, instance, instrs, ip, op_stack, stack_pos);
  }
  // execute interpreter
  else {
    ProcessInterpretedMethodCall(called, instance, instrs, ip);
  }
#else
  ProcessInterpretedMethodCall(called, instance, instrs, ip);
#endif
}

/********************************
 * Processes an interpreted
 * synchronous method call.
 ********************************/
void StackInterpreter::ProcessJitMethodCall(StackMethod* called, size_t* instance, StackInstr** &instrs, long &ip, size_t* &op_stack, long* &stack_pos)
{
#if defined(_DEBUGGER) || defined(_NO_JIT)
  ProcessInterpretedMethodCall(called, instance, instrs, ip);
#else
  // compile, if needed
  if(!called->GetNativeCode()) {
    
#if defined(_WIN64) || defined(_X64)
    JitCompilerIA64 jit_compiler;
#elif _ARM32
    JitCompilerA32 jit_compiler;
#else
    JitCompilerIA32 jit_compiler;
#endif
    if(!jit_compiler.Compile(called)) {
      ProcessInterpretedMethodCall(called, instance, instrs, ip);
#ifdef _DEBUG
      wcerr << L"### Unable to compile: " << called->GetName() << L" ###" << endl;
#endif
      return;
    }
  }
  
  // execute
  (*frame) = GetStackFrame(called, instance);
  JitExecutor jit_executor;
  long status = jit_executor.Execute(called, (size_t*)instance, op_stack, stack_pos, call_stack, call_stack_pos, *frame);
  if(status < 0) {
    switch(status) {
    case -1:
      wcerr << L">>> Atempting to dereference a 'Nil' memory instance in native JIT code <<<" << endl;
      break;
    case -2:
    case -3:
      wcerr << L">>> Index out of bounds in native JIT code! <<<" << endl;
      break;
    }
    StackErrorUnwind(called);
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }

  // restore previous state
  ReleaseStackFrame(*frame);
  (*frame) = PopFrame();
  instrs = (*frame)->method->GetInstructions();
  ip = (*frame)->ip;
#endif
}

/********************************
 * Processes an interpreted
 * synchronous method call.
 ********************************/
void StackInterpreter::ProcessInterpretedMethodCall(StackMethod* called, size_t* instance, StackInstr** &instrs, long &ip)
{
#ifdef _DEBUG
  wcout << L"=== MTHD_CALL: id=" << called->GetClass()->GetId() << L","
        << called->GetId() << L"; name='" << called->GetName() << L"' ===" << endl;
#endif  
  (*frame) = GetStackFrame(called, instance);
  instrs = (*frame)->method->GetInstructions();
  ip = 0;
#ifdef _DEBUG
  wcout << L"creating frame=" << (*frame) << endl;
#endif
}

/********************************
 * Processes a load integer array
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessLoadIntArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LOAD_INT_ARY_ELM; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* array = (size_t*)PopInt(op_stack, stack_pos);
  if(!array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory element <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  const long size = (long)array[0];
  array += 2;
  long index = ArrayIndex(instr, array, size, op_stack, stack_pos);
  PushInt(array[index + instr->GetOperand()], op_stack, stack_pos);
}

/********************************
 * Processes a load store array
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessStoreIntArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: STOR_INT_ARY_ELM; call_pos=" << (*call_stack_pos) << endl;
#endif

  size_t* array = (size_t*)PopInt(op_stack, stack_pos);
  if(!array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory element <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  
  const long size = (long)array[0];
  array += 2;
  long index = ArrayIndex(instr, array, size, op_stack, stack_pos);
  array[index + instr->GetOperand()] = PopInt(op_stack, stack_pos);
}

/********************************
 * Processes a load byte array
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessLoadByteArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LOAD_BYTE_ARY_ELM; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* array = (size_t*)PopInt(op_stack, stack_pos);
  if(!array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory element <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  const long size = (long)array[0];
  array += 2;
  long index = ArrayIndex(instr, array, size, op_stack, stack_pos);
  array += instr->GetOperand();
  PushInt(((char*)array)[index], op_stack, stack_pos);
}

/********************************
 * Processes a load char array
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessLoadCharArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LOAD_CHAR_ARY_ELM; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* array = (size_t*)PopInt(op_stack, stack_pos);
  if(!array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory element <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  const long size = (long)array[0];
  array += 2;
  long index = ArrayIndex(instr, array, size, op_stack, stack_pos);
  array += instr->GetOperand();
  PushInt(((wchar_t*)array)[index], op_stack, stack_pos);
}

/********************************
 * Processes a store byte array
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessStoreByteArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: STOR_BYTE_ARY_ELM; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* array = (size_t*)PopInt(op_stack, stack_pos);
  if(!array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory element <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  const long size = (long)array[0];
  array += 2;
  long index = ArrayIndex(instr, array, size, op_stack, stack_pos);
  array += instr->GetOperand();
  ((char*)array)[index] = (char)PopInt(op_stack, stack_pos);
}

/********************************
 * Processes a store char array
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessStoreCharArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: STOR_CHAR_ARY_ELM; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* array = (size_t*)PopInt(op_stack, stack_pos);
  if(!array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory element <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  const long size = (long)array[0];
  array += 2;
  long index = ArrayIndex(instr, array, size, op_stack, stack_pos);
  array += instr->GetOperand();
  ((wchar_t*)array)[index] = (wchar_t)PopInt(op_stack, stack_pos);
}

/********************************
 * Processes a load float array
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessLoadFloatArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: LOAD_FLOAT_ARY_ELM; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* array = (size_t*)PopInt(op_stack, stack_pos);
  if(!array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory element <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  const long size = (long)array[0];
  array += 2;
  long index = ArrayIndex(instr, array, size, op_stack, stack_pos);
  FLOAT_VALUE value;
  ::memcpy(&value, array + index + instr->GetOperand(), sizeof(FLOAT_VALUE));
  PushFloat(value, op_stack, stack_pos);
}

/********************************
 * Processes a store float array
 * variable instruction.
 ********************************/
void StackInterpreter::ProcessStoreFloatArrayElement(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: STOR_FLOAT_ARY_ELM; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* array = (size_t*)PopInt(op_stack, stack_pos);
  if(!array) {
    wcerr << L">>> Atempting to dereference a 'Nil' memory element <<<" << endl;
    StackErrorUnwind();
#ifdef _DEBUGGER
    halt = true;
    return;
#else
    exit(1);
#endif
  }
  const long size = (long)array[0];
  array += 2;
  long index = ArrayIndex(instr, array, size, op_stack, stack_pos);
  FLOAT_VALUE value = PopFloat(op_stack, stack_pos);
  ::memcpy(array + index + instr->GetOperand(), &value, sizeof(FLOAT_VALUE));
}

/********************************
 * Shared library operations
 ********************************/

typedef void (*ext_load_def)();
void StackInterpreter::ProcessDllLoad(StackInstr* instr)
{
#ifdef _DEBUG
  wcout << L"stack oper: shared LIBRARY_LOAD; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* instance = (size_t*)(*frame)->mem[0];
  if(!instance) {
    wcerr << L">>> Unable to load shared library! <<<" << endl;
#ifdef _DEBUGGER
    exit(1);
#else
    return;
#endif
  }

  size_t* str_obj = (size_t*)instance[0];
  if(!str_obj || !(size_t*)str_obj[0]) {
    wcerr << L">>> Name of runtime shared library was not specified! <<<" << endl;
#ifdef _DEBUGGER
    return;
#else
    exit(1);
#endif
  }
  
  wstring path_str;
#ifdef _OBJECK_NATIVE_LIB_PATH
#ifdef _WIN32
  size_t len;
  char* lib_path = nullptr;
  if(_dupenv_s(&lib_path, &len, "pathext")) {
    path_str += BytesToUnicode(lib_path);
    path_str += L"\\native\\";
  }
  else {
    path_str += L"..\\lib\\native\\";
  }
#else
  char* lib_path = getenv("OBJECK_LIB_PATH");
  if(lib_path) {
    path_str += BytesToUnicode(lib_path);
    path_str += L"/native/";
  }
  else {
    path_str += L"../lib/native/";
  }
#endif
#else
#ifdef _WIN32
  path_str += L"..\\lib\\native\\";
#else
  path_str += L"../lib/native/";
#endif
#endif
  size_t* array = (size_t*)str_obj[0];
  const wstring post_path_str((wchar_t*)(array + 3));
  path_str += post_path_str;
  
  string dll_string = UnicodeToBytes(path_str);
  if(dll_string.size() == 0) {
    wcerr << L">>> Name of runtime shared library was not specified! <<<" << endl;
#ifdef _DEBUGGER
    return;
#else
    exit(1);
#endif
  }

#ifdef _WIN32
  dll_string += ".dll";  
#elif _OSX
  dll_string += ".dylib";
#else
  dll_string += ".so";
#endif 

  // load shared library
#ifdef _WIN32
  // Load shared library file
  HINSTANCE dll_handle = LoadLibrary(dll_string.c_str());
  if(!dll_handle) {
    wcerr << L">>> Runtime error loading shared library: " << dll_string.c_str() << L" <<<" << endl;
#ifdef _DEBUGGER
    return;
#else
    exit(1);
#endif
  }
  instance[1] = (size_t)dll_handle;

  // call load function
  ext_load_def ext_load = (ext_load_def)GetProcAddress(dll_handle, "load_lib");
  if(!ext_load) {
    wcerr << L">>> Runtime error calling function: load_lib <<<" << endl;
    FreeLibrary(dll_handle);
#ifdef _DEBUGGER
    return;
#else
    exit(1);
#endif
  }
  (*ext_load)();
#else
  void* dll_handle = dlopen(dll_string.c_str(), RTLD_LAZY);
  if(!dll_handle) {
    wcerr << L">>> Runtime error loading shared library: " << dlerror() << L" <<<" << endl;
#ifdef _DEBUGGER
    return;
#else
    exit(1);
#endif
  }
  instance[1] = (size_t)dll_handle;

  // call load function
  ext_load_def ext_load = (ext_load_def)dlsym(dll_handle, "load_lib");
  char* error;
  if((error = dlerror()) != nullptr)  {
    wcerr << L">>> Runtime error calling function: " << error << L" <<<" << endl;
#ifdef _DEBUGGER
    return;
#else
    exit(1);
#endif
  }
  // call function
  (*ext_load)();
#endif
}

typedef void (*ext_unload_def)();
void StackInterpreter::ProcessDllUnload(StackInstr* instr)
{
#ifdef _DEBUG
  wcout << L"stack oper: shared library_UNLOAD; call_pos=" << (*call_stack_pos) << endl;
#endif
  size_t* instance = (size_t*)(*frame)->mem[0];
  // unload shared library
#ifdef _WIN32
  HINSTANCE dll_handle = (HINSTANCE)instance[1];
  if(dll_handle) {
    // call unload function  
    ext_load_def ext_unload = (ext_load_def)GetProcAddress(dll_handle, "unload_lib");
    if(!ext_unload) {
      wcerr << L">>> Runtime error calling function: unload_lib <<<" << endl;
      FreeLibrary(dll_handle);
#ifdef _DEBUGGER
      return;
#else
      exit(1);
#endif
    }
    (*ext_unload)();
    // free handle
    FreeLibrary(dll_handle);
  }
#else
  void* dll_handle = (void*)instance[1];
  if(dll_handle) {
    // call unload function
    ext_unload_def ext_unload = (ext_unload_def)dlsym(dll_handle, "unload_lib");
    char* error;
    if((error = dlerror()) != nullptr)  {
      wcerr << L">>> Runtime error calling function: " << error << L" <<<" << endl;
#ifdef _DEBUGGER
      return;
#else
      exit(1);
#endif
    }
    // call function
    (*ext_unload)();
    // unload lib
    dlclose(dll_handle);
  }
#endif
}

typedef void (*lib_func_def) (VMContext& callbacks);
void StackInterpreter::ProcessDllCall(StackInstr* instr, size_t* &op_stack, long* &stack_pos)
{
#ifdef _DEBUG
  wcout << L"stack oper: shared LIBRARY_FUNC_CALL; call_pos=" << (*call_stack_pos) << endl;
#endif 
  size_t* instance = (size_t*)(*frame)->mem[0];
  size_t* str_obj = (size_t*)(*frame)->mem[1];
  size_t* array = (size_t*)str_obj[0];
  if(!array) {
    wcerr << L">>> Runtime error calling function <<<" << endl;
#ifdef _DEBUGGER
    return;
#else
    exit(1);
#endif
  }

  const wstring wstr((wchar_t*)(array + 3));
  size_t* args = (size_t*)(*frame)->mem[2];
  lib_func_def ext_func;

#ifdef _WIN32
  HINSTANCE dll_handle = (HINSTANCE)instance[1];
  if(dll_handle) {
    // get function pointer
    const string str =   UnicodeToBytes(wstr);
    ext_func = (lib_func_def)GetProcAddress(dll_handle, str.c_str());
    if(!ext_func) {
      wcerr << L">>> Runtime error calling function: " << wstr << L" <<<" << endl;
      FreeLibrary(dll_handle);
#ifdef _DEBUGGER
      return;
#else
      exit(1);
#endif
    }
    // call function
    VMContext context;
    context.data_array = args;
    context.op_stack = op_stack;
    context.stack_pos = stack_pos;
    context.call_method_by_name = APITools_MethodCall;
    context.call_method_by_id = APITools_MethodCallId;
    context.alloc_array = MemoryManager::AllocateArray;
    context.alloc_obj = MemoryManager::AllocateObject;
    (*ext_func)(context);
  }
#else
  // load function
  void* dll_handle = (void*)instance[1];
  if(dll_handle) {
    const string str = UnicodeToBytes(wstr);
    ext_func = (lib_func_def)dlsym(dll_handle, str.c_str());
    char* error;
    if((error = dlerror()) != nullptr)  {
      wcerr << L">>> Runtime error calling function: " << error << L" <<<" << endl;
#ifdef _DEBUGGER
      return;
#else
      exit(1);
#endif
    }
    // call function
    VMContext context;
    context.data_array = args;
    context.op_stack = op_stack;
    context.stack_pos = stack_pos;
    context.call_method_by_name = APITools_MethodCall;
    context.call_method_by_id = APITools_MethodCallId;
    context.alloc_array = MemoryManager::AllocateArray;
    context.alloc_obj = MemoryManager::AllocateObject;
    (*ext_func)(context);
  }  
#endif
}

StackFrame* Runtime::StackInterpreter::GetStackFrame(StackMethod* method, size_t* instance)
{
#ifdef _WIN32
  EnterCriticalSection(&cached_frames_cs);
#else
  pthread_mutex_lock(&cached_frames_mutex);
#endif
  if(cached_frames.empty()) {
    // load cache
    for(int i = 0; i < CALL_STACK_SIZE; ++i) {
      StackFrame* frame = new StackFrame();
      frame->mem = (size_t*)calloc(LOCAL_SIZE, sizeof(size_t));
      cached_frames.push(frame);
    }
  }
  StackFrame* frame = cached_frames.top();
  cached_frames.pop();

  frame->method = method;
  frame->mem[0] = (size_t)instance;
  frame->ip = -1;
  frame->jit_called = false;
  frame->jit_mem = nullptr;
  frame->jit_offset = 0;
#ifdef _DEBUG
  wcout << L"fetching frame=" << frame << endl;
#endif

#ifdef _WIN32
  LeaveCriticalSection(&cached_frames_cs);
#else
  pthread_mutex_unlock(&cached_frames_mutex);
#endif
  return frame;
}

void Runtime::StackInterpreter::ReleaseStackFrame(StackFrame* frame)
{
#ifdef _WIN32
  EnterCriticalSection(&cached_frames_cs);
#else
  pthread_mutex_lock(&cached_frames_mutex);
#endif      

  // load cache
  frame->jit_mem = nullptr;
  memset(frame->mem, 0, LOCAL_SIZE * sizeof(size_t));
  cached_frames.push(frame);
#ifdef _DEBUG
  wcout << L"caching frame=" << frame << endl;
#endif    

#ifdef _WIN32
  LeaveCriticalSection(&cached_frames_cs);
#else
  pthread_mutex_unlock(&cached_frames_mutex);
#endif
}

void Runtime::StackInterpreter::StackErrorUnwind()
{
  long pos = (*call_stack_pos);
#ifdef _DEBUGGER
  wcerr << L"Unwinding local stack (" << this << L"):" << endl;
  StackMethod* method = (*frame)->method;
  if((*frame)->ip > 0 && pos > -1 &&
     method->GetInstruction((*frame)->ip)->GetLineNumber() > 0) {
    wcerr << L"  method: pos=" << pos << L", file="
      << (*frame)->method->GetClass()->GetFileName() << L", name='"
      << (*frame)->method->GetName() << L"', line="
      << method->GetInstruction((*frame)->ip)->GetLineNumber() << endl;
  }
  if(pos != 0) {
    while(--pos) {
      StackMethod* method = call_stack[pos]->method;
      if(call_stack[pos]->ip > 0 && pos > -1 &&
         method->GetInstruction(call_stack[pos]->ip)->GetLineNumber() > 0) {
        wcerr << L"  method: pos=" << pos << L", file="
          << call_stack[pos]->method->GetClass()->GetFileName() << L", name='"
          << call_stack[pos]->method->GetName() << L"', line="
          << method->GetInstruction(call_stack[pos]->ip)->GetLineNumber() << endl;
      }
    }
    pos = 0;
  }
  wcerr << L"  ..." << endl;
#else
  wcerr << L"Unwinding local stack (" << this << L"):" << endl;
  wcerr << L"  method: pos=" << pos << L", name="
    << (*frame)->method->GetName() << endl;
  if(pos != 0) {
    while(--pos && pos > -1) {
      wcerr << L"  method: pos=" << pos << L", name="
        << call_stack[pos]->method->GetName() << endl;
    }
  }
  wcerr << L"  ..." << endl;
#endif
}

long Runtime::StackInterpreter::ArrayIndex(StackInstr* instr, size_t* array, const long size, size_t*& op_stack, long*& stack_pos)
{
  // generate index
  long index = (long)PopInt(op_stack, stack_pos);
  const long dim = instr->GetOperand();

  for(long i = 1; i < dim; i++) {
    index *= (long)array[i];
    index += (long)PopInt(op_stack, stack_pos);
  }

#ifdef _DEBUG
  wcout << L"  [raw index=" << index << L", raw size=" << size << L"]" << endl;
#endif

  // 64-bit bounds check
#if defined(_WIN64) || defined(_X64)
  if(index < 0 || index >= size) {
    wcerr << L">>> Index out of bounds: " << index << L"," << size << L" <<<" << endl;
    StackErrorUnwind();
    exit(1);
  }
#else
      // 32-bit bounds check
  if(instr->GetType() == LOAD_FLOAT_ARY_ELM || instr->GetType() == STOR_FLOAT_ARY_ELM) {
    // float array
    index *= 2;
    if(index < 0 || index >= size * 2) {
      wcerr << L">>> Index out of bounds: " << index << L"," << (size * 2) << L" <<<" << endl;
      StackErrorUnwind();
      exit(1);
    }
  }
  else {
    // interger array
    if(index < 0 || index >= size) {
      wcerr << L">>> Index out of bounds: " << index << L"," << size << L" <<<" << endl;
      StackErrorUnwind();
      exit(1);
    }
  }
#endif

  return index;
}

size_t* Runtime::StackInterpreter::CreateStringObject(const wstring& value_str, size_t*& op_stack, long*& stack_pos)
{
  // create character array
  const long char_array_size = (long)value_str.size();
  const long char_array_dim = 1;
  size_t* char_array = (size_t*)MemoryManager::AllocateArray(char_array_size + 1 + ((char_array_dim + 2) * sizeof(size_t)),
                                                             CHAR_ARY_TYPE, op_stack, *stack_pos, false);
  char_array[0] = char_array_size + 1;
  char_array[1] = char_array_dim;
  char_array[2] = char_array_size;

  // copy wstring
  wchar_t* char_array_ptr = (wchar_t*)(char_array + 3);
#ifdef _WIN32
  wcsncpy_s(char_array_ptr, char_array_size + 1, value_str.c_str(), char_array_size);
#else
  wcsncpy(char_array_ptr, value_str.c_str(), char_array_size);
#endif

  // create 'System.String' object instance
  size_t * str_obj = MemoryManager::AllocateObject(program->GetStringObjectId(), op_stack, *stack_pos, false);
  str_obj[0] = (size_t)char_array;
  str_obj[1] = char_array_size;
  str_obj[2] = char_array_size;

  return str_obj;
}

void Runtime::StackInterpreter::StackErrorUnwind(StackMethod* method)
{
  long pos = (*call_stack_pos);
  wcerr << L"Unwinding local stack (" << this << L"):" << endl;
  wcerr << L"  method: pos=" << pos << L", name="
    << method->GetName() << endl;
  while(--pos) {
    if(pos > -1) {
      wcerr << L"  method: pos=" << pos << L", name="
        << call_stack[pos]->method->GetName() << endl;
    }
  }
  wcerr << L"  ..." << endl;
}

