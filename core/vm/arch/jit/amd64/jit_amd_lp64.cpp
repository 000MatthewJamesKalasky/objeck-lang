/***************************************************************************
 * JIT compiler for the Windows AMD64 architecture.
 *
 * Copyright (c) 2008-2019 Randy Hollines
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright 
 * notice, this list  of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in 
 * the documentation and/or other materials provided with the distribution.
 * - Neither the name of the Objeck Team nor the names of its 
 * contributors may be used to endorse or promote products derived 
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, RXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, RVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#include "jit_amd_lp64.h"
#include <string>

using namespace Runtime;

/********************************
 * JitCompilerIA64 class
 ********************************/
StackProgram* JitCompilerIA64::program;
PageManager* JitCompilerIA64::page_manager;

void JitCompilerIA64::Initialize(StackProgram* p) {
  program = p;
  page_manager = new PageManager;
}

void JitCompilerIA64::Prolog() {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [<prolog>]" << endl;
#endif

  local_space += 16;
  while(local_space % 16 != 0) {
    local_space++;
  }
  local_space += 8;
  
  unsigned char buffer[4];
  ByteEncode32(buffer, local_space);

  unsigned char setup_code[] = {
    // setup stack frame
    0x48, 0x55,                  // push $rbp
    0x48, 0x89, 0xe5,            // mov  $rsp, $rbp    
    0x48, 0x81, 0xec,            // sub  %imm, $rsp
    buffer[0], buffer[1], buffer[2], buffer[3],      
    // save registers
    0x48, 0x53,                  // push $rbx
    0x48, 0x51,                  // push $rcx
    0x48, 0x52,                  // push $rdx
    0x48, 0x57,                  // push $rdi
    0x48, 0x56,                  // push $rsi
#ifndef _WIN64
    0x49, 0x50,                  // push r8
    0x49, 0x51,                  // push r9
    0x49, 0x52,                  // push r10
    0x49, 0x53,                  // push r11
    0x49, 0x54,                  // push r12
    0x49, 0x55,                  // push r13
    0x49, 0x56,                  // push r14
    0x49, 0x57,                  // push r15
#endif  
  };
  const long setup_size = sizeof(setup_code);
  // copy setup
  for(long i = 0; i < setup_size; ++i) {
    AddMachineCode(setup_code[i]);
  }
}

void JitCompilerIA64::Epilog() 
{
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [<epilog>]" << endl;
#endif
  epilog_index = code_index;

  // nominal
  AddMachineCode(0xe9);
  AddImm(45);
  // null deref
  move_imm_reg(-1, RAX);
  AddMachineCode(0xe9);
  AddImm(40);
  // under bounds
  move_imm_reg(-2, RAX);
  AddMachineCode(0xe9);
  AddImm(25);
  // over bounds
  move_imm_reg(-3, RAX);
  AddMachineCode(0xe9);
  AddImm(10);
  // nominal
  move_imm_reg(0, RAX);
  
  unsigned char teardown_code[] = {
    // restore registers
#ifndef _WIN64
    0x49, 0x5f,       // pop r15
    0x49, 0x5e,       // pop r14
    0x49, 0x5d,       // pop r13
    0x49, 0x5c,       // pop r12
    0x49, 0x5b,       // pop r11
    0x49, 0x5a,       // pop r10
    0x49, 0x59,       // pop r9
    0x49, 0x58,       // pop r8
#endif  
    0x48, 0x5e,       // pop $rsi
    0x48, 0x5f,       // pop $rdi
    0x48, 0x5a,       // pop $rdx
    0x48, 0x59,       // pop $rcx
    0x48, 0x5b,       // pop $rbx
    // tear down stack frame and return
    0x48, 0x89, 0xec, // mov $rbp, $rsp
    0x48, 0x5d,       // pop $rbp
    0x48, 0xc3        // rtn
  };
  const long teardown_size = sizeof(teardown_code);
  // copy teardown
  for(long i = 0; i < teardown_size; ++i) {
    AddMachineCode(teardown_code[i]);
  }
}

void JitCompilerIA64::RegisterRoot() {
  // calculate root address
  // note: the offset required to 
  // get to the first local variable
#ifdef _WIN64
  const long offset = org_local_space + RED_ZONE + TMP_REG_5 + 8;
#else
  const long offset = org_local_space + RED_ZONE + TMP_REG_5;
#endif
  // get to stack locals
  RegisterHolder* holder = GetRegister();
  move_reg_reg(RBP, holder->GetRegister());
  sub_imm_reg(-TMP_REG_5 + offset, holder->GetRegister());

  // set JIT memory to stack locals
  RegisterHolder* mem_holder = GetRegister();
  move_mem_reg(JIT_MEM, RBP, mem_holder->GetRegister());
  move_reg_mem(holder->GetRegister(), 0, mem_holder->GetRegister());

  // 6 slots to hold spilled registers 
  const int index = ((offset - 8) >> 3) + 6;
  if(index > 0) {
    move_imm_reg(index, RCX);
    move_imm_mem(0, 0, holder->GetRegister());
    add_imm_reg(sizeof(size_t), holder->GetRegister());
    loop(-20);
  }

  move_mem_reg(JIT_OFFSET, RBP, mem_holder->GetRegister());
  move_imm_mem(offset, 0, mem_holder->GetRegister());

  // clean up
  ReleaseRegister(mem_holder);
  ReleaseRegister(holder);
}

void JitCompilerIA64::ProcessParameters(long params) {
#ifdef _DEBUG
  wcout << L"CALLED_PARMS: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
  
  for(long i = 0; i < params; ++i) {
    RegisterHolder* op_stack_holder = GetRegister();
    move_mem_reg(OP_STACK, RBP, op_stack_holder->GetRegister());

    StackInstr* instr = method->GetInstruction(instr_index++);
    instr->SetOffset(code_index);  

    RegisterHolder* stack_pos_holder = GetRegister();
    move_mem_reg(STACK_POS, RBP, stack_pos_holder->GetRegister());
    
    if(instr->GetType() == STOR_LOCL_INT_VAR || instr->GetType() == STOR_CLS_INST_INT_VAR) {
      dec_mem(0, stack_pos_holder->GetRegister());
#ifdef _WIN64    
      move_mem_reg32(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#else
      move_mem_reg(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#endif
      shl_imm_reg(3, stack_pos_holder->GetRegister());
      add_reg_reg(stack_pos_holder->GetRegister(), op_stack_holder->GetRegister());
      RegisterHolder* dest_holder = GetRegister();
      move_mem_reg(0, op_stack_holder->GetRegister(), dest_holder->GetRegister());
      working_stack.push_front(new RegInstr(dest_holder));
      // store int
      ProcessStore(instr);
    }
    else if(instr->GetType() == STOR_FUNC_VAR) {
      dec_mem(0, stack_pos_holder->GetRegister());  
#ifdef _WIN64    
      move_mem_reg32(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#else
      move_mem_reg(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#endif    
      shl_imm_reg(3, stack_pos_holder->GetRegister());
      add_reg_reg(stack_pos_holder->GetRegister(), op_stack_holder->GetRegister());
      RegisterHolder* dest_holder = GetRegister();
      move_mem_reg(0, op_stack_holder->GetRegister(), dest_holder->GetRegister());
      
      RegisterHolder* dest_holder2 = GetRegister();
      move_mem_reg(/*-sizeof(size_t)*/-8, op_stack_holder->GetRegister(), dest_holder2->GetRegister());
      
      move_mem_reg(STACK_POS, RBP, stack_pos_holder->GetRegister());
      dec_mem(0, stack_pos_holder->GetRegister());      

      working_stack.push_front(new RegInstr(dest_holder2));
      working_stack.push_front(new RegInstr(dest_holder));

      // store int
      ProcessStore(instr);
      i++;
    }
    else {
      RegisterHolder* dest_holder = GetXmmRegister();
      dec_mem(0, stack_pos_holder->GetRegister());
#ifdef _WIN64    
      move_mem_reg32(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#else
      move_mem_reg(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#endif    
      shl_imm_reg(3, stack_pos_holder->GetRegister());
      add_reg_reg(stack_pos_holder->GetRegister(), op_stack_holder->GetRegister()); 
      move_mem_xreg(0, op_stack_holder->GetRegister(), dest_holder->GetRegister());
      working_stack.push_front(new RegInstr(dest_holder));

      // store float
      ProcessStore(instr);
    }
    ReleaseRegister(op_stack_holder);
    ReleaseRegister(stack_pos_holder);
  }
}

void JitCompilerIA64::ProcessIntCallParameter() {
#ifdef _DEBUG
  wcout << L"INT_CALL: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
  
  RegisterHolder* op_stack_holder = GetRegister();
  move_mem_reg(OP_STACK, RBP, op_stack_holder->GetRegister());
  
  RegisterHolder* stack_pos_holder = GetRegister();
  move_mem_reg(STACK_POS, RBP, stack_pos_holder->GetRegister());
  
  dec_mem(0, stack_pos_holder->GetRegister()); 
#ifdef _WIN64   
  move_mem_reg32(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#else
  move_mem_reg(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#endif  
  shl_imm_reg(3, stack_pos_holder->GetRegister());
  add_reg_reg(stack_pos_holder->GetRegister(), op_stack_holder->GetRegister());  
  move_mem_reg(0, op_stack_holder->GetRegister(), op_stack_holder->GetRegister());
  working_stack.push_front(new RegInstr(op_stack_holder));
  
  ReleaseRegister(stack_pos_holder);
}

void JitCompilerIA64::ProcessFunctionCallParameter() {
#ifdef _DEBUG
  wcout << L"FUNC_CALL: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
  
  RegisterHolder* op_stack_holder = GetRegister();
  move_mem_reg(OP_STACK, RBP, op_stack_holder->GetRegister());
  
  RegisterHolder* stack_pos_holder = GetRegister();
  move_mem_reg(STACK_POS, RBP, stack_pos_holder->GetRegister());
  
  sub_imm_mem(2, 0, stack_pos_holder->GetRegister());
#ifdef _WIN64
  move_mem_reg32(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#else
  move_mem_reg(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#endif  
  shl_imm_reg(3, stack_pos_holder->GetRegister());
  add_reg_reg(stack_pos_holder->GetRegister(), op_stack_holder->GetRegister());  
  
  RegisterHolder* holder = GetRegister();
  move_reg_reg(op_stack_holder->GetRegister(), holder->GetRegister());
  
  move_mem_reg(0, op_stack_holder->GetRegister(), op_stack_holder->GetRegister());
  working_stack.push_front(new RegInstr(op_stack_holder));
  
  move_mem_reg(8, holder->GetRegister(), holder->GetRegister());
  working_stack.push_front(new RegInstr(holder));
  
  ReleaseRegister(stack_pos_holder);
}

void JitCompilerIA64::ProcessFloatCallParameter() {
#ifdef _DEBUG
  wcout << L"FLOAT_CALL: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
  
  RegisterHolder* op_stack_holder = GetRegister();
  move_mem_reg(OP_STACK, RBP, op_stack_holder->GetRegister());
  
  RegisterHolder* stack_pos_holder = GetRegister();
  move_mem_reg(STACK_POS, RBP, stack_pos_holder->GetRegister());
  
  RegisterHolder* dest_holder = GetXmmRegister();
  dec_mem(0, stack_pos_holder->GetRegister()); 
#ifdef _WIN64   
  move_mem_reg32(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#else
  move_mem_reg(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#endif  
  shl_imm_reg(3, stack_pos_holder->GetRegister());
  add_reg_reg(stack_pos_holder->GetRegister(), op_stack_holder->GetRegister()); 
  move_mem_xreg(0, op_stack_holder->GetRegister(), dest_holder->GetRegister());
  working_stack.push_front(new RegInstr(dest_holder));
  
  ReleaseRegister(op_stack_holder);
  ReleaseRegister(stack_pos_holder);
}

void JitCompilerIA64::ProcessInstructions() {
  while(instr_index < method->GetInstructionCount() && compile_success) {
    StackInstr* instr = method->GetInstruction(instr_index++);
    instr->SetOffset(code_index);
    
    switch(instr->GetType()) {
      // load literal
    case LOAD_CHAR_LIT:
    case LOAD_INT_LIT:
#ifdef _DEBUG
      wcout << L"LOAD_INT: value=" << instr->GetOperand() 
            << L"; regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      working_stack.push_front(new RegInstr(instr));
      break;
      
      // float literal
    case LOAD_FLOAT_LIT:
#ifdef _DEBUG
      wcout << L"LOAD_FLOAT_LIT: value=" << instr->GetFloatOperand() 
            << L"; regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      floats[floats_index] = instr->GetFloatOperand();
      working_stack.push_front(new RegInstr(instr, &floats[floats_index++]));
      break;
      
      // load self
    case LOAD_INST_MEM: {
#ifdef _DEBUG
      wcout << L"LOAD_INST_MEM; regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      working_stack.push_front(new RegInstr(instr));
    }
      break;

      // load self
    case LOAD_CLS_MEM: {
#ifdef _DEBUG
      wcout << L"LOAD_CLS_MEM; regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      working_stack.push_front(new RegInstr(instr));
    }
      break;
      
      // load variable
    case LOAD_LOCL_INT_VAR:
    case LOAD_CLS_INST_INT_VAR:   
    case LOAD_FLOAT_VAR:
    case LOAD_FUNC_VAR:
#ifdef _DEBUG
      wcout << L"LOAD_INT_VAR/LOAD_FLOAT_VAR/LOAD_FUNC_VAR: id=" << instr->GetOperand() << L"; regs=" 
            << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessLoad(instr);
      break;
    
      // store value
    case STOR_LOCL_INT_VAR:
    case STOR_CLS_INST_INT_VAR:
    case STOR_FLOAT_VAR:
    case STOR_FUNC_VAR:
#ifdef _DEBUG
      wcout << L"STOR_INT_VAR/STOR_FLOAT_VAR/STOR_FUNC_VAR: id=" << instr->GetOperand() 
            << L"; regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStore(instr);
      break;

      // copy value
    case COPY_LOCL_INT_VAR:
    case COPY_CLS_INST_INT_VAR:
    case COPY_FLOAT_VAR:
#ifdef _DEBUG
      wcout << L"COPY_INT_VAR/COPY_FLOAT_VAR: id=" << instr->GetOperand() 
            << L"; regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessCopy(instr);
      break;
      
      // mathematical
    case AND_INT:
    case OR_INT:
    case ADD_INT:
    case SUB_INT:
    case MUL_INT:
    case DIV_INT:
    case MOD_INT:
      // TODO: implement
    case BIT_AND_INT:
    case BIT_OR_INT:
    case BIT_XOR_INT:
      // comparison
    case LES_INT:
    case GTR_INT:
    case LES_EQL_INT:
    case GTR_EQL_INT:
    case EQL_INT:
    case NEQL_INT:
    case SHL_INT:
    case SHR_INT:
#ifdef _DEBUG
      wcout << L"INT ADD/SUB/MUL/DIV/MOD/BIT_AND/BIT_OR/BIT_XOR/LES/GTR/EQL/NEQL/SHL_INT/SHR_INT: regs=" 
            << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessIntCalculation(instr);
      break;

    case ADD_FLOAT:
    case SUB_FLOAT:
    case MUL_FLOAT:
    case DIV_FLOAT:
#ifdef _DEBUG
      wcout << L"FLOAT ADD/SUB/MUL/DIV/: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessFloatCalculation(instr);
      break;

    case SIN_FLOAT:
    case COS_FLOAT:
    case TAN_FLOAT:
    case SQRT_FLOAT:
    case ASIN_FLOAT:
    case ACOS_FLOAT:
    case LOG_FLOAT:
    case ATAN2_FLOAT:
    case POW_FLOAT:
#ifdef _DEBUG
      wcout << L"FLOAT SIN/COS/TAN/SQRT/ASIN/ACOS/ATAN2/POW: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessFloatOperation(instr);
      break;

    case LES_FLOAT:
    case GTR_FLOAT:
    case LES_EQL_FLOAT:
    case GTR_EQL_FLOAT:
    case EQL_FLOAT:
    case NEQL_FLOAT: {
#ifdef _DEBUG
      wcout << L"FLOAT LES/GTR/EQL/NEQL: regs=" << aval_regs.size() << L"," 
            << aux_regs.size() << endl;
#endif
      ProcessFloatCalculation(instr);

      RegInstr* left = working_stack.front();
      working_stack.pop_front(); // pop invalid xmm register
      ReleaseXmmRegister(left->GetRegister());

      delete left; 
      left = nullptr;
      
      RegisterHolder* holder = GetRegister();
      cmov_reg(holder->GetRegister(), instr->GetType());
      working_stack.push_front(new RegInstr(holder));
      
    }
      break;
      
    case RTRN:
#ifdef _DEBUG
      wcout << L"RTRN: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessReturn();
      // teardown
      Epilog();
      break;
      
    case MTHD_CALL: {
      StackMethod* called_method = program->GetClass(instr->GetOperand())->GetMethod(instr->GetOperand2());
      if(called_method) {
#ifdef _DEBUG
        assert(called_method);
        wcout << L"MTHD_CALL: name='" << called_method->GetName() << L"': id="<< instr->GetOperand() 
              << L"," << instr->GetOperand2() << L", params=" << (called_method->GetParamCount() + 1) 
              << L": regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif      
        // passing instance variable
        ProcessStackCallback(MTHD_CALL, instr, instr_index, called_method->GetParamCount() + 1);      
        ProcessReturnParameters(called_method->GetReturn());
      }
    }
      break;
      
    case DYN_MTHD_CALL: {
#ifdef _DEBUG
      wcout << L"DYN_MTHD_CALL: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif  
      // passing instance variable
      ProcessStackCallback(DYN_MTHD_CALL, instr, instr_index, instr->GetOperand() + 3);
      ProcessReturnParameters((MemoryType)instr->GetOperand2());
    }
      break;
      
    case NEW_BYTE_ARY:
#ifdef _DEBUG
      wcout << L"NEW_BYTE_ARY: dim=" << instr->GetOperand() << L" regs=" << aval_regs.size()
            << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(NEW_BYTE_ARY, instr, instr_index, instr->GetOperand());
      ProcessReturnParameters(INT_TYPE);
      break;

    case NEW_CHAR_ARY:
#ifdef _DEBUG
      wcout << L"NEW_CHAR_ARY: dim=" << instr->GetOperand() << L" regs=" << aval_regs.size()
            << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(NEW_CHAR_ARY, instr, instr_index, instr->GetOperand());
      ProcessReturnParameters(INT_TYPE);
      break;
      
    case NEW_INT_ARY:
#ifdef _DEBUG
      wcout << L"NEW_INT_ARY: dim=" << instr->GetOperand() << L" regs=" << aval_regs.size() 
            << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(NEW_INT_ARY, instr, instr_index, instr->GetOperand());
      ProcessReturnParameters(INT_TYPE);
      break;

    case NEW_FLOAT_ARY:
#ifdef _DEBUG
      wcout << L"NEW_FLOAT_ARY: dim=" << instr->GetOperand() << L" regs=" << aval_regs.size() 
            << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(NEW_FLOAT_ARY, instr, instr_index, instr->GetOperand());
      ProcessReturnParameters(INT_TYPE);
      break;
      
    case NEW_OBJ_INST: {
#ifdef _DEBUG
      StackClass* called_klass = program->GetClass(instr->GetOperand());      
      wcout << L"NEW_OBJ_INST: name='" << called_klass->GetName() << L"': id=" << instr->GetOperand() 
            << L": regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      // note: object id passed in instruction param
      ProcessStackCallback(NEW_OBJ_INST, instr, instr_index, 0);
      ProcessReturnParameters(INT_TYPE);
    }
      break;
      
    case THREAD_JOIN: {
#ifdef _DEBUG
      wcout << L"THREAD_JOIN: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(THREAD_JOIN, instr, instr_index, 0);
    }
      break;

    case THREAD_SLEEP: {
#ifdef _DEBUG
      wcout << L"THREAD_SLEEP: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(THREAD_SLEEP, instr, instr_index, 1);
    }
      break;
      
    case CRITICAL_START: {
#ifdef _DEBUG
      wcout << L"CRITICAL_START: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(CRITICAL_START, instr, instr_index, 1);
    }
      break;
      
    case CRITICAL_END: {
#ifdef _DEBUG
      wcout << L"CRITICAL_END: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(CRITICAL_END, instr, instr_index, 1);
    }
      break;
      
    case CPY_BYTE_ARY: {
#ifdef _DEBUG
      wcout << L"CPY_BYTE_ARY: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(CPY_BYTE_ARY, instr, instr_index, 5);
    }
      break;

    case CPY_CHAR_ARY: {
#ifdef _DEBUG
      wcout << L"CPY_CHAR_ARY: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(CPY_CHAR_ARY, instr, instr_index, 5);
    }
      break;
      
    case CPY_INT_ARY: {
#ifdef _DEBUG
      wcout << L"CPY_INT_ARY: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(CPY_INT_ARY, instr, instr_index, 5);
    }
      break;

    case CPY_FLOAT_ARY: {
#ifdef _DEBUG
      wcout << L"CPY_FLOAT_ARY: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(CPY_FLOAT_ARY, instr, instr_index, 5);
    }
      break;
      
    case TRAP:
#ifdef _DEBUG
      wcout << L"TRAP: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(TRAP, instr, instr_index, instr->GetOperand());
      break;

    case TRAP_RTRN:
#ifdef _DEBUG
      wcout << L"TRAP_RTRN: args=" << instr->GetOperand() << L"; regs=" 
            << aval_regs.size() << L"," << aux_regs.size() << endl;
      assert(instr->GetOperand());
#endif      
      ProcessStackCallback(TRAP_RTRN, instr, instr_index, instr->GetOperand());
      ProcessReturnParameters(INT_TYPE);
      break;
      
    case STOR_BYTE_ARY_ELM:
#ifdef _DEBUG
      wcout << L"STOR_BYTE_ARY_ELM: regs=" << aval_regs.size() << L"," 
            << aux_regs.size() << endl;
#endif
      ProcessStoreByteElement(instr);
      break;

    case STOR_CHAR_ARY_ELM:
#ifdef _DEBUG
      wcout << L"STOR_CHAR_ARY_ELM: regs=" << aval_regs.size() << L"," 
            << aux_regs.size() << endl;
#endif
      ProcessStoreCharElement(instr);
      break;
      
    case STOR_INT_ARY_ELM:
#ifdef _DEBUG
      wcout << L"STOR_INT_ARY_ELM: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStoreIntElement(instr);
      break;

    case STOR_FLOAT_ARY_ELM:
#ifdef _DEBUG
      wcout << L"STOR_FLOAT_ARY_ELM: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStoreFloatElement(instr);
      break;

    case SWAP_INT: {
#ifdef _DEBUG
      wcout << L"SWAP_INT: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      RegInstr* left = working_stack.front();
      working_stack.pop_front();

      RegInstr* right = working_stack.front();
      working_stack.pop_front();

      working_stack.push_front(left);       
      working_stack.push_front(right);
    }
      break;

    case POP_INT:
    case POP_FLOAT: {
#ifdef _DEBUG
      wcout << L"POP_INT/POP_FLOAT: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      // note: there may be constants that aren't 
      // in registers and don't need to be popped
      if(!working_stack.empty()) {
        // pop and release
        RegInstr* left = working_stack.front();
        working_stack.pop_front(); 
        if(left->GetType() == REG_INT) {
          ReleaseRegister(left->GetRegister());
        }
        else if(left->GetType() == REG_FLOAT) {
          ReleaseXmmRegister(left->GetRegister());
        }
        // clean up
        delete left;
        left = nullptr;
      }
    }
      break;

    case FLOR_FLOAT:
#ifdef _DEBUG
      wcout << L"FLOR_FLOAT: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessFloor(instr);
      break;

    case CEIL_FLOAT:
#ifdef _DEBUG
      wcout << L"CEIL_FLOAT: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessCeiling(instr);
      break;
      
    case F2I:
#ifdef _DEBUG
      wcout << L"F2I: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessFloatToInt(instr);
      break;

    case I2F:
#ifdef _DEBUG
      wcout << L"I2F: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessIntToFloat(instr);
      break;

    case OBJ_TYPE_OF: {
#ifdef _DEBUG
      wcout << L"OBJ_TYPE_OF: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(OBJ_TYPE_OF, instr, instr_index, 1);
      ProcessReturnParameters(INT_TYPE);
    }
      break;
      
    case OBJ_INST_CAST: {
#ifdef _DEBUG
      wcout << L"OBJ_INST_CAST: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(OBJ_INST_CAST, instr, instr_index, 1);
      ProcessReturnParameters(INT_TYPE);
    }
      break;

    case LOAD_ARY_SIZE: {
#ifdef _DEBUG
      wcout << L"LOAD_ARY_SIZE: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(LOAD_ARY_SIZE, instr, instr_index, 1);
      ProcessReturnParameters(INT_TYPE);
    }
      break;
      
    case LOAD_BYTE_ARY_ELM:
#ifdef _DEBUG
      wcout << L"LOAD_BYTE_ARY_ELM: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessLoadByteElement(instr);
      break;
      
    case LOAD_INT_ARY_ELM:
#ifdef _DEBUG
      wcout << L"LOAD_INT_ARY_ELM: regs=" << aval_regs.size() << L"," 
            << aux_regs.size() << endl;
#endif
      ProcessLoadIntElement(instr);
      break;

    case LOAD_CHAR_ARY_ELM:
#ifdef _DEBUG
      wcout << L"LOAD_CHAR_ARY_ELM: regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
      ProcessLoadCharElement(instr);
      break;
      
    case LOAD_FLOAT_ARY_ELM:
#ifdef _DEBUG
      wcout << L"LOAD_FLOAT_ARY_ELM: regs=" << aval_regs.size() << L"," 
            << aux_regs.size() << endl;
#endif
      ProcessLoadFloatElement(instr);
      break;
      
    case JMP:
      ProcessJump(instr);
      break;
      
    case LBL:
#ifdef _DEBUG
      wcout << L"______ LBL: id=" << instr->GetOperand() << L" ______" << endl;
#endif
      break;
      
    default: {
      InstructionType error = (InstructionType)instr->GetType();
      cerr << L"Unknown instruction: " << error << L"!" << endl;
      exit(1);
    }
      break;
    }
  }
}

void JitCompilerIA64::ProcessLoad(StackInstr* instr) {
  // method/function memory
  if(instr->GetOperand2() == LOCL) {
    if(instr->GetType() == LOAD_FUNC_VAR) {
      RegisterHolder* holder = GetRegister();
      move_mem_reg(instr->GetOperand3() + sizeof(size_t), RBP, holder->GetRegister());
      working_stack.push_front(new RegInstr(holder));
      
      RegisterHolder* holder2 = GetRegister();
      move_mem_reg(instr->GetOperand3(), RBP, holder2->GetRegister());
      working_stack.push_front(new RegInstr(holder2));
    }
    else {
      working_stack.push_front(new RegInstr(instr));
    }
  }
  // class or instance memory
  else {
    RegInstr* left = working_stack.front();
    working_stack.pop_front();

    RegisterHolder* holder;
    if(left->GetType() == REG_INT) {
      holder = left->GetRegister();
    }
    else {
      holder = GetRegister();
      move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
    }
    CheckNilDereference(holder->GetRegister());

    // long value
    if(instr->GetType() == LOAD_LOCL_INT_VAR ||
       instr->GetType() == LOAD_CLS_INST_INT_VAR) {
      move_mem_reg(instr->GetOperand3(), holder->GetRegister(), holder->GetRegister());
      working_stack.push_front(new RegInstr(holder));
    }
    // function value
    else if(instr->GetType() == LOAD_FUNC_VAR) {
      RegisterHolder* holder2 = GetRegister();
      move_mem_reg(instr->GetOperand3() + sizeof(size_t), holder->GetRegister(), holder2->GetRegister());
      working_stack.push_front(new RegInstr(holder2));
      
      move_mem_reg(instr->GetOperand3(), holder->GetRegister(), holder->GetRegister());
      working_stack.push_front(new RegInstr(holder));
    }
    // float value
    else {
      RegisterHolder* xmm_holder = GetXmmRegister();
      move_mem_xreg(instr->GetOperand3(), holder->GetRegister(), xmm_holder->GetRegister());
      ReleaseRegister(holder);
      working_stack.push_front(new RegInstr(xmm_holder));    
    }

    delete left;
    left = nullptr;
  }
}

void JitCompilerIA64::ProcessJump(StackInstr* instr) {
  if(!skip_jump) {
#ifdef _DEBUG
    wcout << L"JMP: id=" << instr->GetOperand() << L", regs=" << aval_regs.size() 
          << L"," << aux_regs.size() << endl;
#endif
    if(instr->GetOperand2() < 0) {
      AddMachineCode(0xe9);
    }
    else {
      RegInstr* left = working_stack.front();
      working_stack.pop_front(); 

      switch(left->GetType()) {
      case IMM_INT:{
        RegisterHolder* holder = GetRegister();
        move_imm_reg(left->GetOperand(), holder->GetRegister());
        cmp_imm_reg(instr->GetOperand2(), holder->GetRegister());
        ReleaseRegister(holder);
      }
        break;
        
      case REG_INT:
        cmp_imm_reg(instr->GetOperand2(), left->GetRegister()->GetRegister());
        ReleaseRegister(left->GetRegister());
        break;

      case MEM_INT: {
        RegisterHolder* holder = GetRegister();
        move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
        cmp_imm_reg(instr->GetOperand2(), holder->GetRegister());
        ReleaseRegister(holder);
      }
        break;

      default:
        cerr << L">>> Should never occur (compiler bug?) type=" << left->GetType() << L" <<<" << endl;
        exit(1);
        break;
      }

#ifdef _DEBUG
        std::wcout << L"  " << (++instr_count) << L": [je]" << std::endl;
#endif
      // compare with register
      AddMachineCode(0x0f);
      AddMachineCode(0x84);
      
      // clean up
      delete left;
      left = nullptr;
    }
    // store update index
    jump_table.insert(pair<long, StackInstr*>(code_index, instr));
    // temp offset, updated in next pass
    AddImm(0);
  }
  else {
    RegInstr* left = working_stack.front();
    working_stack.pop_front(); 
    skip_jump = false;

    // release register
    if(left->GetType() == REG_INT) {
      ReleaseRegister(left->GetRegister());
    }

    // clean up
    delete left;
    left = nullptr;
  }
}

void JitCompilerIA64::ProcessReturnParameters(MemoryType type) {
  switch(type) {
  case INT_TYPE:
    ProcessIntCallParameter();
    break;
    
  case FLOAT_TYPE:
    ProcessFloatCallParameter();
    break;
    
  case FUNC_TYPE:
    ProcessFunctionCallParameter();
    break;

  default:
    break;
  }
}

void JitCompilerIA64::ProcessLoadByteElement(StackInstr* instr) {
  RegisterHolder* elem_holder = ArrayIndex(instr, BYTE_ARY_TYPE);
  RegisterHolder* holder = GetRegister();
  xor_reg_reg(holder->GetRegister(), holder->GetRegister());
  move_mem8_reg(0, elem_holder->GetRegister(), holder->GetRegister());
  ReleaseRegister(elem_holder);
  working_stack.push_front(new RegInstr(holder));
}

void JitCompilerIA64::ProcessLoadCharElement(StackInstr* instr) {
  RegisterHolder* holder = GetRegister(false);
  RegisterHolder* elem_holder = ArrayIndex(instr, CHAR_ARY_TYPE);
  xor_reg_reg(holder->GetRegister(), holder->GetRegister());
#ifdef _WIN64  
  move_mem16_reg(0, elem_holder->GetRegister(), holder->GetRegister());
#else
  move_mem32_reg(0, elem_holder->GetRegister(), holder->GetRegister());
#endif
  ReleaseRegister(elem_holder);
  working_stack.push_front(new RegInstr(holder));
}

void JitCompilerIA64::ProcessLoadIntElement(StackInstr* instr) {
  RegisterHolder* elem_holder = ArrayIndex(instr, INT_TYPE);
  move_mem_reg(0, elem_holder->GetRegister(), elem_holder->GetRegister());
  working_stack.push_front(new RegInstr(elem_holder));
}

void JitCompilerIA64::ProcessLoadFloatElement(StackInstr* instr) {
  RegisterHolder* elem_holder = ArrayIndex(instr, FLOAT_TYPE);
  RegisterHolder* holder = GetXmmRegister();
  move_mem_xreg(0, elem_holder->GetRegister(), holder->GetRegister());
  working_stack.push_front(new RegInstr(holder));
  ReleaseRegister(elem_holder);
}

void JitCompilerIA64::ProcessStoreByteElement(StackInstr* instr) {
  RegisterHolder* elem_holder = ArrayIndex(instr, BYTE_ARY_TYPE);
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_INT:
    move_imm_mem8(left->GetOperand(), 0, elem_holder->GetRegister());
    ReleaseRegister(elem_holder);
    break;

  case MEM_INT: {    
    // movb can only use al, bl, cl and dl registers
    RegisterHolder* holder = GetRegister(false);
    move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
    move_reg_mem8(holder->GetRegister(), 0, elem_holder->GetRegister());
    ReleaseRegister(holder);
    ReleaseRegister(elem_holder);
  }
    break;

  case REG_INT: {
    // movb can only use al, bl, cl and dl registers
    RegisterHolder* holder = left->GetRegister();
    if(holder->GetRegister() == RDI || holder->GetRegister() == RSI) {
      RegisterHolder* tmp_holder = GetRegister(false);
      move_reg_reg(holder->GetRegister(), tmp_holder->GetRegister());
      move_reg_mem8(tmp_holder->GetRegister(), 0, elem_holder->GetRegister());      
      ReleaseRegister(tmp_holder);
    }
    else {
      move_reg_mem8(holder->GetRegister(), 0, elem_holder->GetRegister());      
    }
    ReleaseRegister(holder);
    ReleaseRegister(elem_holder);
  }
    break;

  default:
    break;
  }
  
  delete left;
  left = nullptr;
}

void JitCompilerIA64::ProcessStoreCharElement(StackInstr* instr) {
  RegisterHolder* elem_holder = ArrayIndex(instr, CHAR_ARY_TYPE);
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_INT:
    if(elem_holder->GetRegister() > RSP) {    
      // movw can only use al, bl, cl and dl registers
      RegisterHolder* holder = GetRegister(false);
      move_reg_reg(elem_holder->GetRegister(), holder->GetRegister());
      ReleaseRegister(elem_holder);
#ifdef _WIN64     
      move_imm_mem16(left->GetOperand(), 0, elem_holder->GetRegister());
#else
      move_imm_mem(left->GetOperand(), 0, holder->GetRegister());
#endif    
      ReleaseRegister(holder);
    }
    else {
#ifdef _WIN64  
      move_imm_mem16(left->GetOperand(), 0, elem_holder->GetRegister());
#else    
    move_imm_mem(left->GetOperand(), 0, elem_holder->GetRegister());
#endif    
      ReleaseRegister(elem_holder);
    }
    break;

  case MEM_INT: {    
    // movw can only use al, bl, cl and dl registers
    RegisterHolder* holder = GetRegister(false);
    move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
#ifdef _WIN64  
    move_reg_mem16(holder->GetRegister(), 0, elem_holder->GetRegister());
#else
    move_reg_mem32(holder->GetRegister(), 0, elem_holder->GetRegister());
#endif  
    ReleaseRegister(holder);
    ReleaseRegister(elem_holder);
  }
    break;

  case REG_INT: {
    // movw can only use al, bl, cl and dl registers
    RegisterHolder* holder = left->GetRegister();
    if(holder->GetRegister() == RDI || holder->GetRegister() == RSI) {
      RegisterHolder* tmp_holder = GetRegister(false);
      move_reg_reg(holder->GetRegister(), tmp_holder->GetRegister());
#ifdef _WIN64    
      move_reg_mem16(tmp_holder->GetRegister(), 0, elem_holder->GetRegister());
#else
      move_reg_mem32(tmp_holder->GetRegister(), 0, elem_holder->GetRegister()); 
#endif    
      ReleaseRegister(tmp_holder);
    }
    else {
#ifdef _WIN64  
      move_reg_mem16(holder->GetRegister(), 0, elem_holder->GetRegister());
#else
      move_reg_mem32(holder->GetRegister(), 0, elem_holder->GetRegister());
#endif    
    }
    ReleaseRegister(holder);
    ReleaseRegister(elem_holder);
  }
    break;

  default:
    break;
  }
  
  delete left;
  left = nullptr;
}

void JitCompilerIA64::ProcessStoreIntElement(StackInstr* instr) {
  RegisterHolder* elem_holder = ArrayIndex(instr, INT_TYPE);
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_INT:
    move_imm_mem(left->GetOperand(), 0, elem_holder->GetRegister());
    break;

  case MEM_INT: {
    RegisterHolder* holder = GetRegister();
    move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
    move_reg_mem(holder->GetRegister(), 0, elem_holder->GetRegister());
    ReleaseRegister(holder);
  }
    break;

  case REG_INT: {
    RegisterHolder* holder = left->GetRegister();
    move_reg_mem(holder->GetRegister(), 0, elem_holder->GetRegister());
    ReleaseRegister(holder);
  }
    break;

  default:
    break;
  }
  ReleaseRegister(elem_holder);
  
  delete left;
  left = nullptr;
}

void JitCompilerIA64::ProcessStoreFloatElement(StackInstr* instr) {
  RegisterHolder* elem_holder = ArrayIndex(instr, FLOAT_TYPE);
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_FLOAT:
    move_imm_memx(left, 0, elem_holder->GetRegister());
    break;

  case MEM_FLOAT: 
  case MEM_INT: {
    RegisterHolder* holder = GetXmmRegister();
    move_mem_xreg(left->GetOperand(), 
                  RBP, holder->GetRegister());
    move_xreg_mem(holder->GetRegister(), 0, elem_holder->GetRegister());
    ReleaseXmmRegister(holder);
  }
    break;

  case REG_FLOAT: {
    RegisterHolder* holder = left->GetRegister();
    move_xreg_mem(holder->GetRegister(), 0, elem_holder->GetRegister());
    ReleaseXmmRegister(holder);
  }
    break;

  default:
    break;
  }
  ReleaseRegister(elem_holder);
  
  delete left;
  left = nullptr;
}

void JitCompilerIA64::ProcessFloor(StackInstr* instr) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_FLOAT: {
    RegisterHolder* holder = GetXmmRegister();
    round_imm_xreg(left, holder->GetRegister(), true);
    working_stack.push_front(new RegInstr(holder));
    delete left;
    left = nullptr;
  }
    break;
    
  case MEM_FLOAT:
  case MEM_INT: {
    RegisterHolder* holder = GetXmmRegister();
    round_mem_xreg(left->GetOperand(), RBP, holder->GetRegister(), true);
    working_stack.push_front(new RegInstr(holder));
    delete left;
    left = nullptr;
  }
    break;
    
  case REG_FLOAT:
    round_xreg_xreg(left->GetRegister()->GetRegister(), 
    left->GetRegister()->GetRegister(), true);
    working_stack.push_front(left);
    break;

  default:
    break;
  }
}

void JitCompilerIA64::ProcessCeiling(StackInstr* instr) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_FLOAT: {
    RegisterHolder* holder = GetXmmRegister();
    round_imm_xreg(left, holder->GetRegister(), false);
    working_stack.push_front(new RegInstr(holder));
    delete left;
    left = nullptr;
  }
    break;
    
  case MEM_FLOAT:
  case MEM_INT: {
    RegisterHolder* holder = GetXmmRegister();
    round_mem_xreg(left->GetOperand(), RBP, holder->GetRegister(), false);
    working_stack.push_front(new RegInstr(holder));
    delete left;
    left = nullptr;
  }
    break;
    
  case REG_FLOAT:
    round_xreg_xreg(left->GetRegister()->GetRegister(), 
    left->GetRegister()->GetRegister(), false);
    working_stack.push_front(left);
    break;

  default:
    break;
  }
}

void JitCompilerIA64::ProcessFloatToInt(StackInstr* instr) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  RegisterHolder* holder = GetRegister();
  switch(left->GetType()) {
  case IMM_FLOAT:
    cvt_imm_reg(left, holder->GetRegister());
    break;
    
  case MEM_FLOAT:
  case MEM_INT:
    cvt_mem_reg(left->GetOperand(), 
                RBP, holder->GetRegister());
    break;

  case REG_FLOAT:
    cvt_xreg_reg(left->GetRegister()->GetRegister(), holder->GetRegister());
    ReleaseXmmRegister(left->GetRegister());
    break;

  default:
    break;
  }
  working_stack.push_front(new RegInstr(holder));

  delete left;
  left = nullptr;
}

void JitCompilerIA64::ProcessIntToFloat(StackInstr* instr) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  RegisterHolder* holder = GetXmmRegister();
  switch(left->GetType()) {
  case IMM_INT:
    cvt_imm_xreg(left, holder->GetRegister());
    break;
    
  case MEM_INT:
    cvt_mem_xreg(left->GetOperand(), 
                 RBP, holder->GetRegister());
    break;

  case REG_INT:
    cvt_reg_xreg(left->GetRegister()->GetRegister(), holder->GetRegister());
    ReleaseRegister(left->GetRegister());
    break;

  default:
    break;
  }
  working_stack.push_front(new RegInstr(holder));

  delete left;
  left = nullptr;
}

void JitCompilerIA64::ProcessStore(StackInstr* instr) {
  Register dest;
  RegisterHolder* addr_holder = nullptr;

  // instance/method memory
  if(instr->GetOperand2() == LOCL) {
    dest = RBP;
  }
  // class or instance memory
  else {
    RegInstr* left = working_stack.front();
    working_stack.pop_front();
    
    if(left->GetRegister()) {
      addr_holder = left->GetRegister();
    }
    else {
      addr_holder = GetRegister();
      move_mem_reg(left->GetOperand(), RBP, addr_holder->GetRegister());
    }
    dest = addr_holder->GetRegister();
    CheckNilDereference(dest);
    
    delete left;
    left = nullptr;
  }
  
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_INT:
    if(instr->GetType() == STOR_FUNC_VAR) {
      move_imm_mem(left->GetOperand(), instr->GetOperand3(), dest);
      
      RegInstr* left2 = working_stack.front();
      working_stack.pop_front();
      move_imm_mem(left2->GetOperand(), instr->GetOperand3() + sizeof(size_t), dest);

      delete left2;
      left2 = nullptr;
    }
    else {
      move_imm_mem(left->GetOperand(), instr->GetOperand3(), dest);
    }
    break;

  case MEM_INT: {
    RegisterHolder* holder = GetRegister();
    if(instr->GetType() == STOR_FUNC_VAR) {
      move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
      move_reg_mem(holder->GetRegister(), instr->GetOperand3(), dest);

      RegInstr* left2 = working_stack.front();
      working_stack.pop_front();
      move_mem_reg(left2->GetOperand(), RBP, holder->GetRegister());
      move_reg_mem(holder->GetRegister(), instr->GetOperand3() + sizeof(size_t), dest);

      delete left2;
      left2 = nullptr;
    }
    else {      
      move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());            
      move_reg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    }
    ReleaseRegister(holder);
  }
    break;
    
  case REG_INT: {
    RegisterHolder* holder = left->GetRegister();
    if(instr->GetType() == STOR_FUNC_VAR) {
      move_reg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
      
      RegInstr* left2 = working_stack.front();
      working_stack.pop_front();
      RegisterHolder* holder2  = left2->GetRegister();
      
      move_reg_mem(holder2->GetRegister(), instr->GetOperand3() + sizeof(size_t), dest);
      ReleaseRegister(holder2);

      delete left2;
      left2 = nullptr;
    }
    else {      
      move_reg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    }
    ReleaseRegister(holder);
  }
    break;

  case IMM_FLOAT:
    move_imm_memx(left, instr->GetOperand3(), dest);
    break;
    
  case MEM_FLOAT: {
    RegisterHolder* holder = GetXmmRegister();
    move_mem_xreg(left->GetOperand(), RBP, holder->GetRegister());
    move_xreg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    ReleaseXmmRegister(holder);
  }
    break;
    
  case REG_FLOAT: {
    RegisterHolder* holder = left->GetRegister();
    move_xreg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    ReleaseXmmRegister(holder);
  }
    break;
  }

  if(addr_holder) {
    ReleaseRegister(addr_holder);
  }

  delete left;
  left = nullptr;
}

void JitCompilerIA64::ProcessCopy(StackInstr* instr) {
  Register dest;
  // instance/method memory
  if(instr->GetOperand2() == LOCL) {
    dest = RBP;
  }
  // class or instance memory
  else {
    RegInstr* left = working_stack.front();
    working_stack.pop_front();

    RegisterHolder* holder = GetRegister();
    move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
    CheckNilDereference(holder->GetRegister());
    dest = holder->GetRegister();
    ReleaseRegister(holder);
    
    delete left;
    left = nullptr;
  }
  
  RegInstr* left = working_stack.front();
  switch(left->GetType()) {
  case IMM_INT: {
    RegisterHolder* holder = GetRegister();
    move_imm_reg(left->GetOperand(), holder->GetRegister());
    move_reg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    // save register
    working_stack.pop_front();
    working_stack.push_front(new RegInstr(holder));

    delete left;
    left = nullptr;
  }
    break;

  case MEM_INT: {
    RegisterHolder* holder = GetRegister();
    move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
    move_reg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    // save register
    working_stack.pop_front();
    working_stack.push_front(new RegInstr(holder));

    delete left;
    left = nullptr;
  }
    break;

  case REG_INT: {
    RegisterHolder* holder = left->GetRegister();
    move_reg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
  }
    break;

  case IMM_FLOAT: {
    RegisterHolder* holder = GetXmmRegister();
    move_imm_xreg(left, holder->GetRegister());
    move_xreg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    // save register
    working_stack.pop_front();
    working_stack.push_front(new RegInstr(holder));

    delete left;
    left = nullptr;
  }
    break;

  case MEM_FLOAT: {
    RegisterHolder* holder = GetXmmRegister();
    move_mem_xreg(left->GetOperand(), RBP, holder->GetRegister());
    move_xreg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    // save register
    working_stack.pop_front();
    working_stack.push_front(new RegInstr(holder));

    delete left;
    left = nullptr;
  }
    break;

  case REG_FLOAT: {
    RegisterHolder* holder = left->GetRegister();
    move_xreg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
  }
    break;
  }
}

void JitCompilerIA64::ProcessStackCallback(long instr_id, StackInstr* instr, long &instr_index, long params) {
  long non_params;
  if(params < 0) {
    non_params = 0;
  }
  else {
    non_params = (long)working_stack.size() - params;
  }
  
#ifdef _DEBUG
  wcout << L"Return: params=" << params << L", non-params=" << non_params << endl;
#endif

  stack<RegInstr*> regs;
  stack<long> dirty_regs;
  long reg_offset = TMP_REG_0;

  stack<RegInstr*> xmms;
  stack<long> dirty_xmms;
  long xmm_offset = TMP_XMM_0;

  long i = 0;
  for(deque<RegInstr*>::reverse_iterator iter = working_stack.rbegin(); iter != working_stack.rend(); ++iter) {
    RegInstr* left = (*iter);
    if(i < non_params) {
      switch(left->GetType()) {
        case REG_INT:
          move_reg_mem(left->GetRegister()->GetRegister(), reg_offset, RBP);
          dirty_regs.push(reg_offset);
          regs.push(left);
          reg_offset -= sizeof(size_t);
          break;

        case REG_FLOAT:
          move_xreg_mem(left->GetRegister()->GetRegister(), xmm_offset, RBP);
          dirty_xmms.push(xmm_offset);
          xmms.push(left);
          xmm_offset -= sizeof(double);
          break;

        default:
          break;
      }
      // update
      i++;
    }
  }

#ifdef _DEBUG
  assert(reg_offset >= TMP_REG_5);
  assert(xmm_offset >= TMP_XMM_2);
#endif

  if(dirty_regs.size() > 6 || dirty_xmms.size() > 3 ) {
    compile_success = false;
  }

  // copy values to execution stack
  ProcessReturn(params);
  
#ifdef _WIN64
  // set parameters
  move_imm_reg(instr_id, RCX);
  move_imm_reg((size_t)instr, RDX);
  move_mem_reg(CLS_ID, RBP, R8);
  move_mem_reg(MTHD_ID, RBP, R9);
  push_imm(instr_index - 1);
  push_mem(CALL_STACK_POS, RBP);
  push_mem(CALL_STACK, RBP);
  push_mem(STACK_POS, RBP);
  push_mem(OP_STACK, RBP);
  push_mem(INSTANCE_MEM, RBP);

  // call function
  sub_imm_reg(32, RSP);
  move_imm_reg((size_t)JitCompilerIA64::JitStackCallback, R10);
  call_reg(R10);
  add_imm_reg(80, RSP);
#else
  // save other registers
  push_reg(R15);
  push_reg(R14);
  push_reg(R13);
  push_reg(R8);
  
  // set parameters
  move_mem_reg(OP_STACK, RBP, R9);
  move_mem_reg(INSTANCE_MEM, RBP, R8);
  move_mem_reg(MTHD_ID, RBP, RCX);
  move_mem_reg(CLS_ID, RBP, RDX);
  move_imm_reg((long)instr, RSI);
  move_imm_reg(instr_id, RDI);  
  push_imm(instr_index - 1);
  push_mem(CALL_STACK_POS, RBP);
  push_mem(CALL_STACK, RBP);
  push_mem(STACK_POS, RBP);
  
  // call function
  move_imm_reg((long)JitCompilerIA64::JitStackCallback, R15);
  call_reg(R15);
  add_imm_reg(32, RSP);
  
  // restore registers
  pop_reg(R8);
  pop_reg(R13);
  pop_reg(R14);
  pop_reg(R15); 
#endif

  // restore register values
  while(!dirty_regs.empty()) {
    RegInstr* left = regs.top();
    move_mem_reg(dirty_regs.top(), RBP, left->GetRegister()->GetRegister());
    // update
    regs.pop();
    dirty_regs.pop();
  }
  
  while(!dirty_xmms.empty()) {
    RegInstr* left = xmms.top();
    move_mem_xreg(dirty_xmms.top(), RBP, left->GetRegister()->GetRegister());
    // update
    xmms.pop();
    dirty_xmms.pop();
  }
}

void JitCompilerIA64::ProcessReturn(long params) {
  if(!working_stack.empty()) {
    RegisterHolder* op_stack_holder = GetRegister();
    move_mem_reg(OP_STACK, RBP, op_stack_holder->GetRegister());
    
    RegisterHolder* stack_pos_holder = GetRegister();
    move_mem_reg(STACK_POS, RBP, stack_pos_holder->GetRegister());
#ifdef _WIN64      
    move_mem_reg32(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#else
    move_mem_reg(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
#endif  
    shl_imm_reg(3, stack_pos_holder->GetRegister());
    add_reg_reg(stack_pos_holder->GetRegister(), op_stack_holder->GetRegister());  

    long non_params;
    if(params < 0) {
      non_params = 0;
    }
    else {
      non_params = (long)working_stack.size() - params;
    }
#ifdef _DEBUG
    wcout << L"Return: params=" << params << L", non-params=" << non_params << endl;
#endif
    
    long i = 0;     
    for(deque<RegInstr*>::reverse_iterator iter = working_stack.rbegin(); iter != working_stack.rend(); ++iter) {
      // skip non-params... processed above
      RegInstr* left = (*iter);
      if(i < non_params) {
        i++;
      }
      else {
        move_mem_reg(STACK_POS, RBP, stack_pos_holder->GetRegister());
        switch(left->GetType()) {
          case IMM_INT:
            move_imm_mem(left->GetOperand(), 0, op_stack_holder->GetRegister());
            inc_mem(0, stack_pos_holder->GetRegister());
            add_imm_reg(sizeof(size_t), op_stack_holder->GetRegister());
            break;

          case MEM_INT:
          {
            RegisterHolder* temp_holder = GetRegister();
            move_mem_reg(left->GetOperand(), RBP, temp_holder->GetRegister());
            move_reg_mem(temp_holder->GetRegister(), 0, op_stack_holder->GetRegister());
            inc_mem(0, stack_pos_holder->GetRegister());
            add_imm_reg(sizeof(size_t), op_stack_holder->GetRegister());
            ReleaseRegister(temp_holder);
          }
          break;

          case REG_INT:
            move_reg_mem(left->GetRegister()->GetRegister(), 0, op_stack_holder->GetRegister());
            inc_mem(0, stack_pos_holder->GetRegister());
            add_imm_reg(sizeof(size_t), op_stack_holder->GetRegister());
            break;

          case IMM_FLOAT:
            move_imm_memx(left, 0, op_stack_holder->GetRegister());
            inc_mem(0, stack_pos_holder->GetRegister());
            add_imm_reg(sizeof(double), op_stack_holder->GetRegister());
            break;

          case MEM_FLOAT: {
            RegisterHolder* temp_holder = GetXmmRegister();
            move_mem_xreg(left->GetOperand(), RBP, temp_holder->GetRegister());
            move_xreg_mem(temp_holder->GetRegister(), 0, op_stack_holder->GetRegister());
            inc_mem(0, stack_pos_holder->GetRegister());
            add_imm_reg(sizeof(double), op_stack_holder->GetRegister());
            ReleaseXmmRegister(temp_holder);
          }
          break;

          case REG_FLOAT:
            move_xreg_mem(left->GetRegister()->GetRegister(), 0, op_stack_holder->GetRegister());
            inc_mem(0, stack_pos_holder->GetRegister());
            add_imm_reg(sizeof(double), op_stack_holder->GetRegister());
            break;
        }
      }
    }
    ReleaseRegister(op_stack_holder);
    ReleaseRegister(stack_pos_holder);
    
    // clean up working stack
    if(params < 0) {
      params = (long)working_stack.size();
    }
    for(long i = 0; i < params; ++i) {
      RegInstr* left = working_stack.front();
      working_stack.pop_front();

      // release register
      switch(left->GetType()) {
      case REG_INT:
        ReleaseRegister(left->GetRegister());
        break;

      case REG_FLOAT:
        ReleaseXmmRegister(left->GetRegister());
        break;

      default:
        break;
      }
      // clean up
      delete left;
      left = nullptr;
    }
  }
}

RegInstr* JitCompilerIA64::ProcessIntFold(long left_imm, long right_imm, InstructionType type) {
  switch(type) {
  case AND_INT:
    return new RegInstr(IMM_INT, left_imm && right_imm);
    
  case OR_INT:
    return new RegInstr(IMM_INT, left_imm || right_imm);
    
  case ADD_INT:
    return new RegInstr(IMM_INT, left_imm + right_imm);
    
  case SUB_INT:
    return new RegInstr(IMM_INT, left_imm - right_imm);
    
  case MUL_INT:
    return new RegInstr(IMM_INT, left_imm * right_imm);
    
  case DIV_INT:
    return new RegInstr(IMM_INT, left_imm / right_imm);
    
  case MOD_INT:
    return new RegInstr(IMM_INT, left_imm % right_imm);
    
  case SHL_INT:
    return new RegInstr(IMM_INT, left_imm << right_imm);
    
  case SHR_INT:
    return new RegInstr(IMM_INT, left_imm >> right_imm);
    
  case BIT_AND_INT:
    return new RegInstr(IMM_INT, left_imm & right_imm);
    
  case BIT_OR_INT:
    return new RegInstr(IMM_INT, left_imm | right_imm);
    
  case BIT_XOR_INT:
    return new RegInstr(IMM_INT, left_imm ^ right_imm);
    
  case LES_INT:  
    return new RegInstr(IMM_INT, left_imm < right_imm);
    
  case GTR_INT:
    return new RegInstr(IMM_INT, left_imm > right_imm);
    
  case EQL_INT:
    return new RegInstr(IMM_INT, left_imm == right_imm);
    
  case NEQL_INT:
    return new RegInstr(IMM_INT, left_imm != right_imm);
    
  case LES_EQL_INT:
    return new RegInstr(IMM_INT, left_imm <= right_imm);
    
  case GTR_EQL_INT:
    return new RegInstr(IMM_INT, left_imm >= right_imm);
    
  default:
    return nullptr;
  }
}

void JitCompilerIA64::ProcessIntCalculation(StackInstr* instruction) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();

  RegInstr* right = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
    // intermidate
  case IMM_INT:
    switch(right->GetType()) {
    case IMM_INT:
      working_stack.push_front(ProcessIntFold(left->GetOperand(), 
          right->GetOperand(), 
          instruction->GetType()));
      break;
      
    case REG_INT: {
      RegisterHolder* imm_holder = GetRegister();
      move_imm_reg(left->GetOperand(), imm_holder->GetRegister());
      RegisterHolder* holder = right->GetRegister();
      
      math_reg_reg(holder->GetRegister(), imm_holder->GetRegister(), 
                   instruction->GetType());
      
      ReleaseRegister(holder);
      working_stack.push_front(new RegInstr(imm_holder));
    }
      break;
      
    case MEM_INT: {
      RegisterHolder* holder = GetRegister();
      move_mem_reg(right->GetOperand(), RBP, holder->GetRegister());

      RegisterHolder* imm_holder = GetRegister();
      move_imm_reg(left->GetOperand(), imm_holder->GetRegister());

      math_reg_reg(holder->GetRegister(), imm_holder->GetRegister(), 
                   instruction->GetType());
      
      ReleaseRegister(holder);
      working_stack.push_front(new RegInstr(imm_holder));
    }
      break;

    default:
      break;
    }      
    break; 

    // register
  case REG_INT:
    switch(right->GetType()) {
    case IMM_INT: {
      RegisterHolder* holder = left->GetRegister();
      math_imm_reg(right->GetOperand(), holder->GetRegister(), 
                   instruction->GetType());
      working_stack.push_front(new RegInstr(holder));
    }
      break;
    case REG_INT: {
      RegisterHolder* holder = right->GetRegister();
      math_reg_reg(holder->GetRegister(), left->GetRegister()->GetRegister(), instruction->GetType());
      working_stack.push_front(new RegInstr(left->GetRegister()));
      ReleaseRegister(holder);
    }
      break;
    case MEM_INT: {
      RegisterHolder* rhs = GetRegister();
      move_mem_reg(right->GetOperand(), RBP, rhs->GetRegister());
      RegisterHolder* lhs = left->GetRegister();
      math_reg_reg(rhs->GetRegister(), lhs->GetRegister(), instruction->GetType());
      ReleaseRegister(rhs);
      working_stack.push_front(new RegInstr(lhs));
    }
      break;

    default:
      break;
    }
    break;

    // memory
  case MEM_INT:
    switch(right->GetType()) {
    case IMM_INT: {
      RegisterHolder* holder = GetRegister();
      move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
      math_imm_reg(right->GetOperand(), holder->GetRegister(), instruction->GetType());
      working_stack.push_front(new RegInstr(holder));
    }
      break;
    case REG_INT: {
      RegisterHolder* rhs = GetRegister();
      move_mem_reg(left->GetOperand(), RBP, rhs->GetRegister());
      RegisterHolder* lhs = right->GetRegister();
      math_reg_reg(lhs->GetRegister(), rhs->GetRegister(), instruction->GetType());
      ReleaseRegister(lhs);
      working_stack.push_front(new RegInstr(rhs));
    }
      break;
    case MEM_INT: {
      RegisterHolder* holder = GetRegister();
      move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
      math_mem_reg(right->GetOperand(), holder->GetRegister(), instruction->GetType());
      working_stack.push_front(new RegInstr(holder));
    }
      break;

    default:
      break;
    }
    break;

  default:
    break;
  }
  
  delete left;
  left = nullptr;
    
  delete right;
  right = nullptr;
}

void JitCompilerIA64::ProcessFloatCalculation(StackInstr* instruction) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  RegInstr* right = working_stack.front();
  working_stack.pop_front();

  InstructionType type = instruction->GetType();
  switch(left->GetType()) {
    // intermidate
  case IMM_FLOAT:
    switch(right->GetType()) {
    case IMM_FLOAT: {
      RegisterHolder* left_holder = GetXmmRegister();
      move_imm_xreg(left, left_holder->GetRegister());      
      RegisterHolder* right_holder = GetXmmRegister();
      move_imm_xreg(right, right_holder->GetRegister());      
      
      if(type == LES_FLOAT || type == LES_EQL_FLOAT) {
        math_xreg_xreg(left_holder->GetRegister(), right_holder->GetRegister(), 
     instruction->GetType());
        ReleaseXmmRegister(left_holder);
        working_stack.push_front(new RegInstr(right_holder));
      }
      else {
        math_xreg_xreg(right_holder->GetRegister(), left_holder->GetRegister(), 
     instruction->GetType());
        ReleaseXmmRegister(right_holder);
        working_stack.push_front(new RegInstr(left_holder));
      }
    }
      break;
      
    case REG_FLOAT: {      
      RegisterHolder* imm_holder = GetXmmRegister();
      move_imm_xreg(left, imm_holder->GetRegister());
      
      if(type == LES_FLOAT || type == LES_EQL_FLOAT) {
        math_xreg_xreg(imm_holder->GetRegister(), right->GetRegister()->GetRegister(), type);
        ReleaseXmmRegister(imm_holder);
        working_stack.push_front(new RegInstr(right->GetRegister()));
      }
      else {
        math_xreg_xreg(right->GetRegister()->GetRegister(), imm_holder->GetRegister(), type);
        ReleaseXmmRegister(right->GetRegister());
        working_stack.push_front(new RegInstr(imm_holder));
      }
    }
      break;

    case MEM_FLOAT:
    case MEM_INT: {
      RegisterHolder* holder = GetXmmRegister();
      move_mem_xreg(right->GetOperand(), RBP, holder->GetRegister());

      RegisterHolder* imm_holder = GetXmmRegister();
      move_imm_xreg(left, imm_holder->GetRegister());

      if(type == LES_FLOAT || type == LES_EQL_FLOAT) {
        math_xreg_xreg(imm_holder->GetRegister(), holder->GetRegister(), type);
        ReleaseXmmRegister(imm_holder);
        working_stack.push_front(new RegInstr(holder));
      }
      else {
        math_xreg_xreg(holder->GetRegister(), imm_holder->GetRegister(), type);
        ReleaseXmmRegister(holder);
        working_stack.push_front(new RegInstr(imm_holder));
      }
    }
      break;

    default:
      break;
    }      
    break; 

    // register
  case REG_FLOAT:
    switch(right->GetType()) {
    case IMM_FLOAT: {
      RegisterHolder* right_holder = GetXmmRegister();
      move_imm_xreg(right, right_holder->GetRegister());

      RegisterHolder* left_holder = left->GetRegister();      
      if(type == LES_FLOAT || type == LES_EQL_FLOAT) {
        math_xreg_xreg(left_holder->GetRegister(), right_holder->GetRegister(), instruction->GetType());
        ReleaseXmmRegister(left_holder);      
        working_stack.push_front(new RegInstr(right_holder));
      }
      else {
        math_xreg_xreg(right_holder->GetRegister(), left_holder->GetRegister(), instruction->GetType());
        ReleaseXmmRegister(right_holder);      
        working_stack.push_front(new RegInstr(left_holder));
      }
    }
      break;

    case REG_FLOAT: {
      RegisterHolder* holder = right->GetRegister();
      if(type == LES_FLOAT || type == LES_EQL_FLOAT) {
        math_xreg_xreg(left->GetRegister()->GetRegister(), holder->GetRegister(), instruction->GetType());
        working_stack.push_front(new RegInstr(holder));
        ReleaseXmmRegister(left->GetRegister());
      }
      else {
        math_xreg_xreg(holder->GetRegister(), left->GetRegister()->GetRegister(), instruction->GetType());
        working_stack.push_front(new RegInstr(left->GetRegister()));
        ReleaseXmmRegister(holder);
      }
    }
      break;
      
    case MEM_FLOAT:
    case MEM_INT: {
      RegisterHolder* holder = left->GetRegister();
      if(type == LES_FLOAT || type == LES_EQL_FLOAT) {
        RegisterHolder* right_holder = GetXmmRegister();
        move_mem_xreg(right->GetOperand(), RBP, right_holder->GetRegister());
        math_xreg_xreg(holder->GetRegister(), right_holder->GetRegister(), instruction->GetType());
        ReleaseXmmRegister(holder);
        working_stack.push_front(new RegInstr(right_holder));
      }
      else {
        math_mem_xreg(right->GetOperand(), holder->GetRegister(), instruction->GetType());
        working_stack.push_front(new RegInstr(holder));
      }
    }
      break;

    default:
      break;
    }
    break;

    // memory
  case MEM_FLOAT:
  case MEM_INT:
    switch(right->GetType()) {
    case IMM_FLOAT: {
      RegisterHolder* holder = GetXmmRegister();
      move_mem_xreg(left->GetOperand(), RBP, holder->GetRegister());
      
      RegisterHolder* imm_holder = GetXmmRegister();
      move_imm_xreg(right, imm_holder->GetRegister());
      if(type == LES_FLOAT || type == LES_EQL_FLOAT) {
        math_xreg_xreg(holder->GetRegister(), imm_holder->GetRegister(), type);
        ReleaseXmmRegister(holder);
        working_stack.push_front(new RegInstr(imm_holder));
      }
      else {
        math_xreg_xreg(imm_holder->GetRegister(), holder->GetRegister(), type);
        ReleaseXmmRegister(imm_holder);
        working_stack.push_front(new RegInstr(holder));
      }
    }
      break;
      
    case REG_FLOAT: {
      RegisterHolder* holder = right->GetRegister();
      if(type == LES_FLOAT || type == LES_EQL_FLOAT) {
        math_mem_xreg(left->GetOperand(), holder->GetRegister(), instruction->GetType());
        working_stack.push_front(new RegInstr(holder));
      }
      else {
        RegisterHolder* right_holder = GetXmmRegister();
        move_mem_xreg(left->GetOperand(), RBP, right_holder->GetRegister());
        math_xreg_xreg(holder->GetRegister(), right_holder->GetRegister(), instruction->GetType());
        ReleaseXmmRegister(holder);
        working_stack.push_front(new RegInstr(right_holder));
      }
    }
      break;
      
    case MEM_FLOAT:
    case MEM_INT: {
      RegisterHolder* left_holder = GetXmmRegister();
      move_mem_xreg(left->GetOperand(), RBP, left_holder->GetRegister());

      RegisterHolder* right_holder = GetXmmRegister();
      move_mem_xreg(right->GetOperand(), RBP, right_holder->GetRegister());
      if(type == LES_FLOAT || type == LES_EQL_FLOAT) {
        math_xreg_xreg(left_holder->GetRegister(), right_holder->GetRegister(),  
     instruction->GetType());
        ReleaseXmmRegister(left_holder);
        working_stack.push_front(new RegInstr(right_holder));
      }
      else {
        math_xreg_xreg(right_holder->GetRegister(), left_holder->GetRegister(),  
     instruction->GetType());  
        ReleaseXmmRegister(right_holder);
        working_stack.push_front(new RegInstr(left_holder));
      }
    }
      break;

    default:
      break;
    }
    break;

  default:
    break;
  }

  delete left;
  left = nullptr;
    
  delete right;
  right = nullptr;
}

void JitCompilerIA64::ProcessFloatOperation(StackInstr* instruction) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();

  InstructionType type = instruction->GetType();
#ifdef _DEBUG
  assert(left->GetType() == MEM_FLOAT);
#endif

  RegisterHolder* holder = nullptr;
  switch(type) {
  case SIN_FLOAT:
    fld_mem(left->GetOperand(), RBP);
    fsin();
    break;

  case COS_FLOAT:
    fld_mem(left->GetOperand(), RBP);
    fcos();
    break;

  case TAN_FLOAT:
    fld_mem(left->GetOperand(), RBP);
    ftan();
    break;

  case SQRT_FLOAT:
    fld_mem(left->GetOperand(), RBP);
    fsqrt();
    break;

  case ASIN_FLOAT:
    holder = call_xfunc(asin, left);
    break;

  case ACOS_FLOAT:
    holder = call_xfunc(acos, left);
    break;

  case LOG_FLOAT:
    holder = call_xfunc(log, left);
    break;

  case ATAN2_FLOAT:
    holder = call_xfunc2(atan2, left);
    break;

  case POW_FLOAT:
    holder = call_xfunc2(pow, left);
    break;

  default:
#ifdef _DEBUG
    assert(false);
#endif
    break;
  }

  if(!holder) {
    holder = GetXmmRegister();
    fstp_mem(left->GetOperand(), RBP);
    move_mem_xreg(left->GetOperand(), RBP, holder->GetRegister());
  }
  working_stack.push_front(new RegInstr(holder));

  delete left;
  left = nullptr;
}

/////////////////// OPERATIONS ///////////////////

void JitCompilerIA64::move_reg_reg(Register src, Register dest) {
  if(src != dest) {
#ifdef _DEBUG
    wcout << L"  " << (++instr_count) << L": [movq %" << GetRegisterName(src) 
          << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
    // encode
    AddMachineCode(ROB(src, dest));
    AddMachineCode(0x89);
    unsigned char code = 0xc0;
    // write value
    RegisterEncode3(code, 2, src);
    RegisterEncode3(code, 5, dest);
    AddMachineCode(code);
  }
}

void JitCompilerIA64::move_reg_mem8(Register src, long offset, Register dest) { 
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movb %" << GetRegisterName(src) 
        << L", " << offset << L"(%" << GetRegisterName(dest) << L")" << L"]" 
        << endl;
#endif
  // encode
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x88);
  AddMachineCode(ModRM(dest, src));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::move_reg_mem16(Register src, int32_t offset, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movw %" << GetRegisterName(src)
    << L", " << offset << L"(%" << GetRegisterName(dest) << L")" << L"]"
    << endl;
#endif
  // encode
  AddMachineCode(0x66);
  AddMachineCode(0x89);
  AddMachineCode(ModRM(dest, src));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::move_reg_mem32(Register src, long offset, Register dest) { 
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movw %" << GetRegisterName(src) 
        << L", " << offset << L"(%" << GetRegisterName(dest) << L")" << L"]" 
        << endl;
#endif
  // encode
  AddMachineCode(RXB32(src, dest));
  AddMachineCode(0x89);
  AddMachineCode(ModRM(dest, src));
  // write value
  AddImm(offset);
}
    
void JitCompilerIA64::move_reg_mem(Register src, long offset, Register dest) { 
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movl %" << GetRegisterName(src) 
        << L", " << offset << L"(%" << GetRegisterName(dest) << L")" << L"]" 
        << endl;
#endif
  // encode
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x89);
  AddMachineCode(ModRM(dest, src));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::move_mem8_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movb " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest)
        << L"]" << endl;
#endif
  // encode
  AddMachineCode(RXB(dest, src));
  AddMachineCode(0x0f);
  AddMachineCode(0xb6);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::move_mem16_reg(int32_t offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movw " << offset << L"(%"
    << GetRegisterName(src) << L"), %" << GetRegisterName(dest)
    << L"]" << endl;
#endif
  // encode
  AddMachineCode(0x0f);
  AddMachineCode(0xb7);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::move_mem32_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movw " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest)
        << L"]" << endl;
#endif
  // encode
  AddMachineCode(RXB32(dest, src));
  AddMachineCode(0x8b);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::move_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movq " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest)
        << L"]" << endl;
#endif
  // encode
  AddMachineCode(RXB(dest, src));
  AddMachineCode(0x8b);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::move_mem_reg32(int32_t offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movl " << offset << L"(%"
    << GetRegisterName(src) << L"), %" << GetRegisterName(dest)
    << L"]" << endl;
#endif
  // encode
  AddMachineCode(RXB32(dest, src));
  AddMachineCode(0x8b);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}
 
void JitCompilerIA64::move_imm_memx(RegInstr* instr, long offset, Register dest) {
  RegisterHolder* tmp_holder = GetXmmRegister();
  move_imm_xreg(instr, tmp_holder->GetRegister());
  move_xreg_mem(tmp_holder->GetRegister(), offset, dest);
  ReleaseXmmRegister(tmp_holder);
}

void JitCompilerIA64::move_imm_mem8(long imm, long offset, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movb $" << imm << L", " << offset 
        << L"(%" << GetRegisterName(dest) << L")" << L"]" << endl;
#endif
  // encode
  AddMachineCode(XB(dest));
  AddMachineCode(0xc6);
  unsigned char code = 0x80;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  // write value
  AddImm(offset);
  AddMachineCode((unsigned char)imm);
}

void JitCompilerIA64::move_imm_mem16(int32_t imm, int32_t offset, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movw $" << imm << L", " << offset
    << L"(%" << GetRegisterName(dest) << L")" << L"]" << endl;
#endif
  // encode
  AddMachineCode(0x66);
  AddMachineCode(0xc7);
  unsigned char code = 0x80;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  // write value
  AddImm(offset);
  AddImm16(imm);
}

void JitCompilerIA64::move_imm_mem(long imm, long offset, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movq $" << imm << L", " << offset 
        << L"(%" << GetRegisterName(dest) << L")" << L"]" << endl;
#endif
  // encode
  AddMachineCode(XB(dest));
  AddMachineCode(0xc7); 
  unsigned char code = 0x80;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  // write value
  AddImm(offset);
  AddImm(imm);
}

#ifdef _WIN64
void JitCompilerIA64::move_imm_reg(size_t imm, Register reg) {
#else
void JitCompilerIA64::move_imm_reg(long imm, Register reg) {
#endif
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movq $" << imm << L", %" 
        << GetRegisterName(reg) << L"]" << endl;
#endif
  // encode
  AddMachineCode(XB(reg));
  unsigned char code = 0xb8;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  // write value
  AddImm64(imm);
}

void JitCompilerIA64::move_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
#ifdef _WIN64  
  move_imm_reg(instr->GetOperand2(), imm_holder->GetRegister());  
#else
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
#endif  
  move_mem_xreg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}
    
void JitCompilerIA64::move_mem_xreg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movsd " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(RXB(dest, src));
  AddMachineCode(0x0f);
  AddMachineCode(0x10);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}
    
void JitCompilerIA64::move_xreg_mem(Register src, long offset, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [movsd %" << GetRegisterName(src) 
        << L", " << offset << L"(%" << GetRegisterName(dest) << L")" << L"]" 
        << endl;
#endif 
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x11);
  AddMachineCode(ModRM(dest, src));
  // write value
  AddImm(offset);
}
    
void JitCompilerIA64::move_xreg_xreg(Register src, Register dest) {
  if(src != dest) {
#ifdef _DEBUG
    wcout << L"  " << (++instr_count) << L": [movsd %" << GetRegisterName(src) 
          << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
    // encode
    AddMachineCode(0xf2);
    AddMachineCode(ROB(src, dest));
    AddMachineCode(0x0f);
    AddMachineCode(0x11);
    unsigned char code = 0xc0;
    // write value
    RegisterEncode3(code, 2, src);
    RegisterEncode3(code, 5, dest);
    AddMachineCode(code);
  }
}

bool JitCompilerIA64::cond_jmp(InstructionType type) {
  if(instr_index >= method->GetInstructionCount()) {
    return false;
  }
  
  StackInstr* next_instr = method->GetInstruction(instr_index);
  if(next_instr->GetType() == JMP && next_instr->GetOperand2() > -1) {
    // if(false) {
#ifdef _DEBUG
    wcout << L"JMP: id=" << next_instr->GetOperand() << L", regs=" << aval_regs.size() << L"," << aux_regs.size() << endl;
#endif
    AddMachineCode(0x0f);

    //
    // jump if true
    //
    if(next_instr->GetOperand2()) {
      switch(type) {
      case LES_INT:  
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [jl]" << endl;
#endif
        AddMachineCode(0x8c);
        break;

      case GTR_INT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [jg]" << endl;
#endif
        AddMachineCode(0x8f);
        break;

      case EQL_INT:
      case EQL_FLOAT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [je]" << endl;
#endif
        AddMachineCode(0x84);
        break;

      case NEQL_INT:
      case NEQL_FLOAT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [jne]" << endl;
#endif
        AddMachineCode(0x85);
        break;

      case LES_EQL_INT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [jle]" << endl;
#endif
        AddMachineCode(0x8e);
        break;
        
      case GTR_EQL_INT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [jge]" << endl;
#endif
        AddMachineCode(0x8d);
        break;
    
      case LES_FLOAT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [ja]" << endl;
#endif
        AddMachineCode(0x87);
        break;
    
      case GTR_FLOAT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [ja]" << endl;
#endif
        AddMachineCode(0x87);
        break;

      case LES_EQL_FLOAT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [jae]" << endl;
#endif
        AddMachineCode(0x83);
        break;
    
      case GTR_EQL_FLOAT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [jae]" << endl;
#endif
        AddMachineCode(0x83);
        break;
        
      default:
        break;
      }  
    }
    //
    // jump - false
    //
    else {
      switch(type) {
      case LES_INT:  
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [jge]" << endl;
#endif
        AddMachineCode(0x8d);
        break;

      case GTR_INT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [jle]" << endl;
#endif
        AddMachineCode(0x8e);
        break;

      case EQL_INT:
      case EQL_FLOAT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [jne]" << endl;
#endif
        AddMachineCode(0x85);
        break;

      case NEQL_INT:
      case NEQL_FLOAT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [je]" << endl;
#endif
        AddMachineCode(0x84);
        break;

      case LES_EQL_INT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [jg]" << endl;
#endif
        AddMachineCode(0x8f);
        break;
        
      case GTR_EQL_INT:
#ifdef _DEBUG
        wcout << L"  " << (++instr_count) << L": [jl]" << endl;
#endif
        AddMachineCode(0x8c);
        break;

      case LES_FLOAT:
        AddMachineCode(0x86);
        break;
    
      case GTR_FLOAT:
        AddMachineCode(0x86);
        break;

      case LES_EQL_FLOAT:
        AddMachineCode(0x82);
        break;
    
      case GTR_EQL_FLOAT:
        AddMachineCode(0x82);
        break;
    
      default:
        break;
      }  
    }
    
    // store update index
    jump_table.insert(pair<long, StackInstr*>(code_index, next_instr));
    
    // temp offset
    AddImm(0);
    skip_jump = true;
    
    return true;
  }
  
  return false;
}

void JitCompilerIA64::loop(long offset)
{
  AddMachineCode(0xe2);
  AddMachineCode((unsigned char)offset);
}

RegisterHolder* JitCompilerIA64::call_xfunc(double(*func_ptr)(double), RegInstr* left)
{
  move_xreg_mem(XMM0, TMP_XMM_0, RBP);
  move_mem_xreg(left->GetOperand(), RBP, XMM0);

  RegisterHolder* call_holder = GetRegister();
  move_imm_reg((size_t)func_ptr, call_holder->GetRegister());
  call_reg(call_holder->GetRegister());
  ReleaseRegister(call_holder);
  
  RegisterHolder* result_holder = GetXmmRegister();
  if(result_holder->GetRegister() != XMM0) {
    move_xreg_xreg(XMM0, result_holder->GetRegister());
    move_mem_xreg(TMP_XMM_0, RBP, XMM0);
  }

  return result_holder;
}

RegisterHolder* JitCompilerIA64::call_xfunc2(double(*func_ptr)(double, double), RegInstr* left)
{
  RegInstr* right = working_stack.front();
  working_stack.pop_front();

#ifdef _DEBUG
  assert(right->GetType() == MEM_FLOAT);
#endif

  move_xreg_mem(XMM1, TMP_XMM_1, RBP);
  move_mem_xreg(left->GetOperand(), RBP, XMM1);

  move_xreg_mem(XMM0, TMP_XMM_0, RBP);
  move_mem_xreg(right->GetOperand(), RBP, XMM0);
  
  RegisterHolder* call_holder = GetRegister();
  move_imm_reg((size_t)func_ptr, call_holder->GetRegister());
  call_reg(call_holder->GetRegister());
  ReleaseRegister(call_holder);

  RegisterHolder* result_holder = GetXmmRegister();
  move_mem_xreg(TMP_XMM_1, RBP, XMM1);
  if(result_holder->GetRegister() != XMM0) {
    move_xreg_xreg(XMM0, result_holder->GetRegister());
    move_mem_xreg(TMP_XMM_0, RBP, XMM0);
  }

  delete right;
  right = nullptr;

  return result_holder;
}

void JitCompilerIA64::math_imm_reg(long imm, Register reg, InstructionType type) 
{
  switch(type) {
  case AND_INT:
    and_imm_reg(imm, reg);
    break;

  case OR_INT:
    or_imm_reg(imm, reg);
    break;
    
  case ADD_INT:
    add_imm_reg(imm, reg);
    break;

  case SUB_INT:
    sub_imm_reg(imm, reg);
    break;

  case MUL_INT:
    mul_imm_reg(imm, reg);
    break;

  case DIV_INT:
    div_imm_reg(imm, reg);
    break;
    
  case MOD_INT:
    div_imm_reg(imm, reg, true);
    break;
    
  case SHL_INT:
    shl_imm_reg(imm, reg);
    break;
    
  case SHR_INT:
    shr_imm_reg(imm, reg);
    break;

  case BIT_AND_INT:
    and_imm_reg(imm, reg);
    break;
    
  case BIT_OR_INT:
    or_imm_reg(imm, reg);
    break;
    
  case BIT_XOR_INT:
    xor_imm_reg(imm, reg);
    break;
    
  case LES_INT:  
  case GTR_INT:
  case EQL_INT:
  case NEQL_INT:
  case LES_EQL_INT:
  case GTR_EQL_INT:
    cmp_imm_reg(imm, reg);
    if(!cond_jmp(type)) {
      cmov_reg(reg, type);
    }
    break;

  default:
    break;
  }
}

void JitCompilerIA64::math_reg_reg(Register src, Register dest, InstructionType type) {
  switch(type) {
  case SHL_INT:
    shl_reg_reg(src, dest);
    break;
    
  case SHR_INT:
    shr_reg_reg(src, dest);
    break;
  case AND_INT:
    and_reg_reg(src, dest);
    break;

  case OR_INT:
    or_reg_reg(src, dest);
    break;
    
  case ADD_INT:
    add_reg_reg(src, dest);
    break;

  case SUB_INT:
    sub_reg_reg(src, dest);
    break;

  case MUL_INT:
    mul_reg_reg(dest, src);
    break;

  case DIV_INT:
    div_reg_reg(src, dest);
    break;

  case MOD_INT:
    div_reg_reg(src, dest, true);
    break;

  case BIT_AND_INT:
    and_reg_reg(src, dest);
    break;

  case BIT_OR_INT:
    or_reg_reg(src, dest);
    break;

  case BIT_XOR_INT:
    xor_reg_reg(src, dest);
    break;
    
  case LES_INT:  
  case GTR_INT:
  case EQL_INT:
  case NEQL_INT:
  case LES_EQL_INT:
  case GTR_EQL_INT:
    cmp_reg_reg(src, dest);
    if(!cond_jmp(type)) {
      cmov_reg(dest, type);
    }
    break;

  default:
    break;
  }
}

void JitCompilerIA64::math_mem_reg(long offset, Register reg, InstructionType type) {
  switch(type) {
  case SHL_INT:
    shl_mem_reg(offset, RBP, reg);
    break;

  case SHR_INT:
    shr_mem_reg(offset, RBP, reg);
    break;
    
  case AND_INT:
    and_mem_reg(offset, RBP, reg);
    break;
    
  case OR_INT:
    or_mem_reg(offset, RBP, reg);
    break;
    
  case ADD_INT:
    add_mem_reg(offset, RBP, reg);
    break;
    
  case SUB_INT:
    sub_mem_reg(offset, RBP, reg);
    break;
    
  case MUL_INT:
    mul_mem_reg(offset, RBP, reg);
    break;

  case DIV_INT:
    div_mem_reg(offset, RBP, reg, false);
    break;
    
  case MOD_INT:
    div_mem_reg(offset, RBP, reg, true);
    break;

  case BIT_AND_INT:
    and_mem_reg(offset, RBP, reg);
    break;

  case BIT_OR_INT:
    or_mem_reg(offset, RBP, reg);
    break;

  case BIT_XOR_INT:
    xor_mem_reg(offset, RBP, reg);
    break;
    
  case LES_INT:
  case LES_EQL_INT:
  case GTR_INT:
  case EQL_INT:
  case NEQL_INT:  
  case GTR_EQL_INT:
    cmp_mem_reg(offset, RBP, reg);
    if(!cond_jmp(type)) {
      cmov_reg(reg, type);
    }
    break;

  default:
    break;
  }
}

void JitCompilerIA64::math_imm_xreg(RegInstr* instr, Register reg, InstructionType type) {
  switch(type) {
  case ADD_FLOAT:
    add_imm_xreg(instr, reg);
    break;

  case SUB_FLOAT:
    sub_imm_xreg(instr, reg);
    break;

  case MUL_FLOAT:
    mul_imm_xreg(instr, reg);
    break;

  case DIV_FLOAT:
    div_imm_xreg(instr, reg);
    break;
    
  case LES_FLOAT:
  case LES_EQL_FLOAT:
  case GTR_FLOAT:
  case EQL_FLOAT:
  case NEQL_FLOAT:
  case GTR_EQL_FLOAT:
    cmp_imm_xreg(instr, reg);
    if(!cond_jmp(type)) {
      cmov_reg(reg, type);
    }
    break;
    
  default:
    break;
  }
}

void JitCompilerIA64::math_mem_xreg(long offset, Register dest, InstructionType type) {
  RegisterHolder* holder = GetXmmRegister();
  move_mem_xreg(offset, RBP, holder->GetRegister());
  math_xreg_xreg(holder->GetRegister(), dest, type);
  ReleaseXmmRegister(holder);
}

void JitCompilerIA64::math_xreg_xreg(Register src, Register dest, InstructionType type) {
  switch(type) {
  case ADD_FLOAT:
    add_xreg_xreg(src, dest);
    break;

  case SUB_FLOAT:
    sub_xreg_xreg(src, dest);
    break;

  case MUL_FLOAT:
    mul_xreg_xreg(src, dest);
    break;

  case DIV_FLOAT:
    div_xreg_xreg(src, dest);
    break;
    
  case LES_FLOAT:
  case LES_EQL_FLOAT:
  case GTR_FLOAT:
  case EQL_FLOAT:
  case NEQL_FLOAT:
  case GTR_EQL_FLOAT:
    cmp_xreg_xreg(src, dest);
    if(!cond_jmp(type)) {
      cmov_reg(dest, type);
    }
    break;

  default:
    break;
  }
}    

void JitCompilerIA64::cmp_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [cmpq %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
#ifndef _WIN64  
  AddMachineCode(ROB(src, dest));
#endif  
  AddMachineCode(0x39);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::cmp_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [cmpq " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) 
        << L"]" << endl;
#endif
  // encode
#ifndef _WIN64  
  AddMachineCode(RXB(dest, src));
#endif
  AddMachineCode(0x3b);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}
    
void JitCompilerIA64::cmp_imm_reg(long imm, Register reg) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [cmpq $" << imm << L", %"
        << GetRegisterName(reg) << L"]" << endl;
#endif
  // encode
  AddMachineCode(XB(reg));
  AddMachineCode(0x81);
  unsigned char code = 0xf8;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  // write value
  AddImm(imm);
}

void JitCompilerIA64::cmov_reg(Register reg, InstructionType oper) {
  // set register to 0; if eflag than set to 1
  move_imm_reg(0, reg);
  RegisterHolder* true_holder = GetRegister();
  move_imm_reg(1, true_holder->GetRegister());
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [cmovq %" 
        << GetRegisterName(reg) << L", %" 
        << GetRegisterName(true_holder->GetRegister()) << L" ]" << endl;
#endif
  // encode
  AddMachineCode(ROB(reg, true_holder->GetRegister()));
  AddMachineCode(0x0f);
  switch(oper) {    
  case GTR_INT:
    AddMachineCode(0x4f);
    break;

  case LES_INT:
    AddMachineCode(0x4c);
    break;
    
  case EQL_INT:
  case EQL_FLOAT:
    AddMachineCode(0x44);
    break;

  case NEQL_INT:
  case NEQL_FLOAT:
    AddMachineCode(0x45);
    break;
    
  case LES_FLOAT:
    AddMachineCode(0x47);
    break;
    
  case GTR_FLOAT:
    AddMachineCode(0x47);
    break;

  case LES_EQL_INT:
    AddMachineCode(0x4e);
    break;

  case GTR_EQL_INT:
    AddMachineCode(0x4d);
    break;
    
  case LES_EQL_FLOAT:
    AddMachineCode(0x43);
    break;

  case GTR_EQL_FLOAT:
    AddMachineCode(0x43);
    break;

  default:
    cerr << L">>> Unknown compare! <<<" << endl;
    exit(1);
    break;
  }
  unsigned char code = 0xc0;
  
  // write value
  RegisterEncode3(code, 2, reg);
  RegisterEncode3(code, 5, true_holder->GetRegister());
  AddMachineCode(code);
  ReleaseRegister(true_holder);
}

void JitCompilerIA64::add_imm_mem(long imm, long offset, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [addq $" << imm << L", " 
        << offset << L"(%"<< GetRegisterName(dest) << L")]" << endl;
#endif
  // encode
  AddMachineCode(XB(dest));
  AddMachineCode(0x81);
  AddMachineCode(ModRM(dest, RAX));
  // write value
  AddImm(offset);
  AddImm(imm);
}
    
void JitCompilerIA64::add_imm_reg(long imm, Register reg) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [addq $" << imm << L", %"
        << GetRegisterName(reg) << L"]" << endl;
#endif
  // encode
  AddMachineCode(B(reg));
  AddMachineCode(0x81);
  unsigned char code = 0xc0;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  // write value
  AddImm(imm);
}
    
void JitCompilerIA64::add_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
#ifdef _WIN64
  move_imm_reg(instr->GetOperand2(), imm_holder->GetRegister());
#else
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
#endif
  add_mem_xreg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::sub_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
#ifdef _WIN64
  move_imm_reg(instr->GetOperand2(), imm_holder->GetRegister());
#else
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
#endif
  sub_mem_xreg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::div_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
#ifdef _WIN64
  move_imm_reg(instr->GetOperand2(), imm_holder->GetRegister());
#else
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
#endif
  div_mem_xreg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::mul_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
#ifdef _WIN64
  move_imm_reg(instr->GetOperand2(), imm_holder->GetRegister());
#else
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
#endif
  mul_mem_xreg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::add_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [addq %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x01);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::sub_xreg_xreg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [subsd %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x5c);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);       
}

void JitCompilerIA64::mul_xreg_xreg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [mulsd %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(dest, src));
  AddMachineCode(0x0f);
  AddMachineCode(0x59);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);
}

void JitCompilerIA64::div_xreg_xreg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [divsd %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x5e);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);
}

void JitCompilerIA64::add_xreg_xreg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [addsd %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x58);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);
}
    
void JitCompilerIA64::add_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [addq " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) 
        << L"]" << endl;
#endif
  // encode
  AddMachineCode(RXB(dest, src));
  AddMachineCode(0x03);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::add_mem_xreg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [addsd " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) 
        << L"]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(RXB(dest, src));
  AddMachineCode(0x0f);
  AddMachineCode(0x58);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);  
}

void JitCompilerIA64::sub_mem_xreg(long offset, Register src, Register dest) {
  RegisterHolder* holder = GetXmmRegister();
  move_mem_xreg(offset, src, holder->GetRegister());
  sub_xreg_xreg(dest, holder->GetRegister());
  move_xreg_xreg(holder->GetRegister(), dest);
  ReleaseXmmRegister(holder);
}

void JitCompilerIA64::mul_mem_xreg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [mulsd " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) 
        << L"]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(RXB(dest, src));
  AddMachineCode(0x0f);
  AddMachineCode(0x59);
  AddMachineCode(ModRM(src, dest));
  AddImm(offset);
}

void JitCompilerIA64::div_mem_xreg(long offset, Register src, Register dest) {
  RegisterHolder* holder = GetXmmRegister();
  move_mem_xreg(offset, src, holder->GetRegister());
  div_xreg_xreg(dest, holder->GetRegister());
  move_xreg_xreg(holder->GetRegister(), dest);
  ReleaseXmmRegister(holder);
}

void JitCompilerIA64::sub_imm_reg(long imm, Register reg) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [subq $" << imm << L", %"
        << GetRegisterName(reg) << L"]" << endl;
#endif
  // encode
  AddMachineCode(B(reg));
  AddMachineCode(0x81);
  unsigned char code = 0xe8;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  AddImm(imm);
}

void JitCompilerIA64::sub_imm_mem(long imm, long offset, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [subq $" << imm << L", " 
        << offset << L"(%"<< GetRegisterName(dest) << L")]" << endl;
#endif
  // encode
  AddMachineCode(XB(dest));
  AddMachineCode(0x81);
  AddMachineCode(ModRM(dest, RBP));
  // write value
  AddImm(offset);
  AddImm(imm);
}

void JitCompilerIA64::sub_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [subq %" << GetRegisterName(src)
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x29);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::sub_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [subq " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) 
        << L"]" << endl;
#endif

  // encode
  AddMachineCode(RXB(dest, src));
  AddMachineCode(0x2b);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::mul_imm_reg(long imm, Register reg) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [imuq $" << imm 
        << L", %"<< GetRegisterName(reg) << L"]" << endl;
#endif
  // encode
  AddMachineCode(ROB(reg, reg));
  AddMachineCode(0x69);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, reg);
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  // write value
  AddImm(imm);
}

void JitCompilerIA64::mul_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [imuq %" << GetRegisterName(src) << L", %"<< GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0xaf);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::mul_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [imuq " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) 
        << L"]" << endl;
#endif
  // encode
  AddMachineCode(RXB(dest, src));
  AddMachineCode(0x0f);
  AddMachineCode(0xaf);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::div_imm_reg(long imm, Register reg, bool is_mod) {
  RegisterHolder* imm_holder = GetRegister();
  move_imm_reg(imm, imm_holder->GetRegister());
  div_reg_reg(imm_holder->GetRegister(), reg, is_mod);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::div_mem_reg(long offset, Register src, Register dest, bool is_mod) {
  if(is_mod) {
    if(dest != RDX) {
      move_reg_mem(RDX, TMP_REG_1, RBP);
    }
    move_reg_mem(RAX, TMP_REG_0, RBP);
  }
  else {
    if(dest != RAX) {
      move_reg_mem(RAX, TMP_REG_0, RBP);
    }
    move_reg_mem(RDX, TMP_REG_1, RBP);
  }

  // ============
  move_reg_reg(dest, RAX);
  AddMachineCode(0x48); // cdq
  AddMachineCode(0x99);
  
  // encode
  AddMachineCode(XB(src));
  AddMachineCode(0xf7);
  AddMachineCode(ModRM(src, RDI));
  // write value
  AddImm(offset);
  
#ifdef _DEBUG
  if(is_mod) {
    wcout << L"  " << (++instr_count) << L": [imod " << offset << L"(%" 
          << GetRegisterName(src) << L")]" << endl;
  }
  else {
    wcout << L"  " << (++instr_count) << L": [idiv " << offset << L"(%" 
          << GetRegisterName(src) << L")]" << endl;
  }
#endif
  // ============

  if(is_mod) {
    if(dest != RDX) {
      move_reg_reg(RDX, dest);
      move_mem_reg(TMP_REG_1, RBP, RDX);
    }

    if(dest != RAX) {
      move_mem_reg(TMP_REG_0, RBP, RAX);
    }
  }
  else {
    if(dest != RAX) {
      move_reg_reg(RAX, dest);
      move_mem_reg(TMP_REG_0, RBP, RAX);
    }
    
    if(dest != RDX) {
      move_mem_reg(TMP_REG_1, RBP, RDX);
    }
  }
}

void JitCompilerIA64::div_reg_reg(Register src, Register dest, bool is_mod) {
  if(is_mod) {
    if(dest != RDX) {
      move_reg_mem(RDX, TMP_REG_1, RBP);
    }
    move_reg_mem(RAX, TMP_REG_0, RBP);
  }
  else {
    if(dest != RAX) {
      move_reg_mem(RAX, TMP_REG_0, RBP);
    }
    move_reg_mem(RDX, TMP_REG_1, RBP);
  }
  
  // ============
  move_reg_reg(dest, RAX);
  AddMachineCode(0x48); // cdq
  AddMachineCode(0x99);
  
  if(src != RAX && src != RDX) {
    // encode
    AddMachineCode(B(src));
    AddMachineCode(0xf7);
    unsigned char code = 0xf8;
    // write value
    RegisterEncode3(code, 5, src);
    AddMachineCode(code);
    
#ifdef _DEBUG
    if(is_mod) {
      wcout << L"  " << (++instr_count) << L": [imod %" 
            << GetRegisterName(src) << L"]" << endl;
    }
    else {
      wcout << L"  " << (++instr_count) << L": [idiv %" 
            << GetRegisterName(src) << L"]" << endl;
    }
#endif
  }
  else {
    // encode
    AddMachineCode(XB(RBP));
    AddMachineCode(0xf7);
    AddMachineCode(ModRM(RBP, RDI));
    // write value
    if(src == RAX) {
      AddImm(TMP_REG_0);
    }
    else {
      AddImm(TMP_REG_1);
    }
    
#ifdef _DEBUG
    if(is_mod) {
      wcout << L"  " << (++instr_count) << L": [imod " << TMP_REG_0 << L"(%" 
            << GetRegisterName(RBP) << L")]" << endl;
    }
    else {
      wcout << L"  " << (++instr_count) << L": [idiv " << TMP_REG_0 << L"(%" 
            << GetRegisterName(RBP) << L")]" << endl;
    }
#endif
  }
  // ============
  
  if(is_mod) {
    if(dest != RDX) {
      move_reg_reg(RDX, dest);
      move_mem_reg(TMP_REG_1, RBP, RDX);
    }

    if(dest != RAX) {
      move_mem_reg(TMP_REG_0, RBP, RAX);
    }
  }
  else {
    if(dest != RAX) {
      move_reg_reg(RAX, dest);
      move_mem_reg(TMP_REG_0, RBP, RAX);
    }
     
    if(dest != RDX) {
      move_mem_reg(TMP_REG_1, RBP, RDX);
    }
  }
}

void JitCompilerIA64::dec_reg(Register dest) {
  AddMachineCode(B(dest));
  unsigned char code = 0x48;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [decq %" 
        << GetRegisterName(dest) << L"]" << endl;
#endif
}

void JitCompilerIA64::dec_mem(long offset, Register dest) {
  AddMachineCode(XB(dest));
  AddMachineCode(0xff);
  unsigned char code = 0x88;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  AddImm(offset);
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [decq " << offset << L"(%" 
        << GetRegisterName(dest) << L")" << L"]" << endl;
#endif
}

void JitCompilerIA64::inc_mem(long offset, Register dest) {
  AddMachineCode(XB(dest));
  AddMachineCode(0xff);
  unsigned char code = 0x80;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  AddImm(offset);
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [incq " << offset << L"(%" 
        << GetRegisterName(dest) << L")" << L"]" << endl;
#endif
}

void JitCompilerIA64::shl_imm_reg(long value, Register dest) {
  AddMachineCode(B(dest));
  AddMachineCode(0xc1);
  unsigned char code = 0xe0;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  AddMachineCode((unsigned char)value);
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [shlq $" << value << L", %" 
        << GetRegisterName(dest) << L"]" << endl;
#endif
}

void JitCompilerIA64::shl_reg_reg(Register src, Register dest)
{
  Register old_dest;
  RegisterHolder* reg_holder = nullptr;
  if(dest == RCX) {
    reg_holder = GetRegister();
    old_dest = dest;
    dest = reg_holder->GetRegister();
    move_reg_reg(old_dest, dest);
  }
  
  if(src != RCX) {
    move_reg_mem(RCX, TMP_REG_0, RBP);
    move_reg_reg(src, RCX);
  }
  
  // --------------------

  // encode
  AddMachineCode(B(dest));
  AddMachineCode(0xd3);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, RSP);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [shlq %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif

  // --------------------
  
  if(src != RCX) {
    move_mem_reg(TMP_REG_0, RBP, RCX);
  }
  
  if(reg_holder) {
    move_reg_reg(dest, old_dest);
    ReleaseRegister(reg_holder);
  }
}

void JitCompilerIA64::shl_mem_reg(long offset, Register src, Register dest) 
{
  RegisterHolder* mem_holder = GetRegister();
  move_mem_reg(offset, src, mem_holder->GetRegister());
  shl_reg_reg(mem_holder->GetRegister(), dest);
  ReleaseRegister(mem_holder);
}

void JitCompilerIA64::shr_imm_reg(long value, Register dest) {
  AddMachineCode(B(dest));
  AddMachineCode(0xc1);
  unsigned char code = 0xe8;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  AddMachineCode((unsigned char)value);
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [shrq $" << value << L", %" 
        << GetRegisterName(dest) << L"]" << endl;
#endif
}

void JitCompilerIA64::shr_reg_reg(Register src, Register dest)
{
  Register old_dest;
  RegisterHolder* reg_holder = nullptr;
  if(dest == RCX) {
    reg_holder = GetRegister();
    old_dest = dest;
    dest = reg_holder->GetRegister();
    move_reg_reg(old_dest, dest);
  }
  
  if(src != RCX) {
    move_reg_mem(RCX, TMP_REG_0, RBP);
    move_reg_reg(src, RCX);
  }
  
  // --------------------
  
  // encode
  AddMachineCode(B(dest));
  AddMachineCode(0xd3);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, RBP);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [shrq %" << GetRegisterName(RCX) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif

  // --------------------
  
  if(src != RCX) {
    move_mem_reg(TMP_REG_0, RBP, RCX);
  }
  
  if(reg_holder) {
    move_reg_reg(dest, old_dest);
    ReleaseRegister(reg_holder);
  }
}

void JitCompilerIA64::shr_mem_reg(long offset, Register src, Register dest) 
{
  RegisterHolder* mem_holder = GetRegister();
  move_mem_reg(offset, src, mem_holder->GetRegister());
  shr_reg_reg(mem_holder->GetRegister(), dest);
  ReleaseRegister(mem_holder);
}

void JitCompilerIA64::push_mem(long offset, Register dest) {
  AddMachineCode(B(dest));
  AddMachineCode(0xff);
  unsigned char code = 0xb0;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  AddImm(offset);
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [pushq " << offset << L"(%" 
        << GetRegisterName(dest) << L")" << L"]" << endl;
#endif
}

void JitCompilerIA64::push_reg(Register reg) {
  AddMachineCode(B(reg));
  unsigned char code = 0x50;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [pushq %" << GetRegisterName(reg) 
        << L"]" << endl;
#endif
}

void JitCompilerIA64::push_imm(long value) {
  AddMachineCode(0x68);
  AddImm(value);
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [pushq $" << value << L"]" << endl;
#endif
}

void JitCompilerIA64::pop_reg(Register reg) {
  AddMachineCode(B(reg));  
  unsigned char code = 0x58;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [popq %" << GetRegisterName(reg) 
        << L"]" << endl;
#endif
}

void JitCompilerIA64::call_reg(Register reg) {
  AddMachineCode(B(reg));  
  AddMachineCode(0xff);
  unsigned char code = 0xd0;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [call %" << GetRegisterName(reg)
        << L"]" << endl;
#endif
}

void JitCompilerIA64::cmp_xreg_xreg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [ucomisd %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(0x66);
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x2e);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);
}

void JitCompilerIA64::cmp_mem_xreg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [ucomisd " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) 
        << L"]" << endl;
#endif
  // encode
  AddMachineCode(0x66);
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x2e);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::cmp_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
#ifdef _WIN64
  move_imm_reg(instr->GetOperand2(), imm_holder->GetRegister());
#else
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
#endif
  cmp_mem_xreg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::cvt_xreg_reg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [cvtsd2si %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(dest, src));
  AddMachineCode(0x0f);
  AddMachineCode(0x2c);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);
}

void JitCompilerIA64::round_imm_xreg(RegInstr* instr, Register reg, bool is_floor) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
#ifdef _WIN64
  move_imm_reg(instr->GetOperand2(), imm_holder->GetRegister());
#else
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
#endif
  round_mem_xreg(0, imm_holder->GetRegister(), reg, is_floor);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::round_mem_xreg(long offset, Register src, Register dest, bool is_floor) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << (is_floor ? ": [floor " : ": [ceil ") 
        << offset << L"(%" << GetRegisterName(src) << L"), %" << GetRegisterName(dest) 
        << L"]" << endl;
#endif
  
  AddMachineCode(0x66);
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x3a);
  AddMachineCode(0x0b);
  // memory
  AddMachineCode(ModRM(src, dest));
  AddImm(offset);
  // mode
  if(is_floor) {
    AddMachineCode(0x03);
  }
  else {
    AddMachineCode(0x05);
  }
}

void JitCompilerIA64::round_xreg_xreg(Register src, Register dest, bool is_floor) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << (is_floor ? ": [floor %" : ": [ceil %") 
        << GetRegisterName(src) << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  
  AddMachineCode(0x66);
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x3a);
  AddMachineCode(0x0b);
  // registers
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  // mode
  if(is_floor) {
    AddMachineCode(0x03);
  }
  else {
    AddMachineCode(0x05);
  }
}

void JitCompilerIA64::cvt_imm_reg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
#ifdef _WIN64
  move_imm_reg(instr->GetOperand2(), imm_holder->GetRegister());
#else
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
#endif
  cvt_mem_reg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::cvt_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [cvtsd2si " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(RXB(dest, src));
  AddMachineCode(0x0f);
  AddMachineCode(0x2c);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::cvt_reg_xreg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [cvtsi2sd %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(dest, src));
  AddMachineCode(0x0f);
  AddMachineCode(0x2a);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);
}

void JitCompilerIA64::cvt_imm_xreg(RegInstr* instr, Register reg) {
  RegisterHolder* imm_holder = GetRegister();
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
  cvt_reg_xreg(imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::cvt_mem_xreg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [cvtsi2sd " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(RXB(dest, src));
  AddMachineCode(0x0f);
  AddMachineCode(0x2a);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::and_imm_reg(long imm, Register reg) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [andq $" << imm << L", %"
        << GetRegisterName(reg) << L"]" << endl;
#endif
  // encode
  AddMachineCode(B(reg));
  AddMachineCode(0x81);
  unsigned char code = 0xe0;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  // write value
  AddImm(imm);
}

void JitCompilerIA64::and_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [andq %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x21);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::and_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [andq " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x23);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::or_imm_reg(long imm, Register reg) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [orq $" << imm << L", %"
        << GetRegisterName(reg) << L"]" << endl;
#endif
  // encode
  AddMachineCode(B(reg));
  AddMachineCode(0x81);
  unsigned char code = 0xc8;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  // write value
  AddImm(imm);
}

void JitCompilerIA64::or_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [orq %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x09);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::or_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [orq " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x0b);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::xor_imm_reg(long imm, Register reg) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [xorq $" << imm << L", %"
        << GetRegisterName(reg) << L"]" << endl;
#endif
  // encode
  AddMachineCode(B(reg));
  AddMachineCode(0x81);
  unsigned char code = 0xf0;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  // write value
  AddImm(imm);
}

void JitCompilerIA64::xor_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [xorq %" << GetRegisterName(src) 
        << L", %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x31);
  unsigned char code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::xor_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [xorq " << offset << L"(%" 
        << GetRegisterName(src) << L"), %" << GetRegisterName(dest) << L"]" << endl;
#endif
  // encode
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x33);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

// --- x87 ---

void JitCompilerIA64::fld_mem(int32_t offset, Register src) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [fld " << offset << L"(%"
    << GetRegisterName(src) << L")]" << endl;
#endif
  // encode
  AddMachineCode(0xdd);
  AddMachineCode(ModRM(src, RAX));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::fstp_mem(int32_t offset, Register src) {
#ifdef _DEBUG
  wcout << L"  " << (++instr_count) << L": [fld " << offset << L"(%"
    << GetRegisterName(src) << L")]" << endl;
#endif
  // encode
  AddMachineCode(0xdd);
  AddMachineCode(ModRM(src, RBX));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::fsin() {
  AddMachineCode(0xd9);
  AddMachineCode(0xfe);
}

void JitCompilerIA64::fcos() {
  AddMachineCode(0xd9);
  AddMachineCode(0xff);
}

void JitCompilerIA64::ftan() {
  AddMachineCode(0xd9);
  AddMachineCode(0xf2);
  AddMachineCode(0xdd);
  AddMachineCode(0xd8);
}

void JitCompilerIA64::fsqrt() {
  AddMachineCode(0xd9);
  AddMachineCode(0xfa);
}

/********************************
 * Calculates the AMD64 MOD R/M
 * offset
 ********************************/
unsigned char Runtime::JitCompilerIA64::ModRM(Register eff_adr, Register mod_rm)
{
  unsigned char byte;

  switch(mod_rm) {
  case RSP:
  case XMM4:
  case R12:
  case XMM12:
    byte = 0xa0;
    break;

  case RAX:
  case XMM0:
  case R8:
  case XMM8:
    byte = 0x80;
    break;

  case RBX:
  case XMM3:
  case R11:
  case XMM11:
    byte = 0x98;
    break;

  case RCX:
  case XMM1:
  case R9:
  case XMM9:
    byte = 0x88;
    break;

  case RDX:
  case XMM2:
  case R10:
  case XMM10:
    byte = 0x90;
    break;

  case RDI:
  case XMM7:
  case R15:
  case XMM15:
    byte = 0xb8;
    break;

  case RSI:
  case XMM6:
  case R14:
  case XMM14:
    byte = 0xb0;
    break;

  case RBP:
  case XMM5:
  case R13:
  case XMM13:
    byte = 0xa8;
    break;

  default:
    wcerr << L"internal error" << endl;
    exit(1);
    break;
  }

  switch(eff_adr) {
  case RAX:
  case XMM0:
  case R8:
  case XMM8:
    break;

  case RBX:
  case XMM3:
  case R11:
  case XMM11:
    byte += 3;
    break;

  case RCX:
  case XMM1:
  case R9:
  case XMM9:
    byte += 1;
    break;

  case RDX:
  case XMM2:
  case R10:
  case XMM10:
    byte += 2;
    break;

  case RDI:
  case XMM7:
  case R15:
  case XMM15:
    byte += 7;
    break;

  case RSI:
  case XMM6:
  case R14:
  case XMM14:
    byte += 6;
    break;

  case RBP:
  case XMM5:
  case R13:
  case XMM13:
    byte += 5;
    break;

  case XMM4:
  case R12:
  case XMM12:
    byte += 4;
    break;

    // should never happen for esp
  case RSP:
    wcerr << L"invalid register reference" << endl;
    exit(1);
    break;

  default:
    wcerr << L"internal error" << endl;
    exit(1);
    break;
  }

  return byte;
}

/********************************
 * Returns the name of a register
 ********************************/
wstring Runtime::JitCompilerIA64::GetRegisterName(Register reg)
{
  switch(reg) {
  case RAX:
    return L"rax";

  case RBX:
    return L"rbx";

  case RCX:
    return L"rcx";

  case RDX:
    return L"rdx";

  case RDI:
    return L"rdi";

  case RSI:
    return L"rsi";

  case RBP:
    return L"rbp";

  case RSP:
    return L"rsp";

  case R8:
    return L"r8";

  case R9:
    return L"r9";

  case R10:
    return L"r10";

  case R11:
    return L"r11";

  case R12:
    return L"r12";

  case R13:
    return L"r13";

  case R14:
    return L"r14";

  case R15:
    return L"r15";

  case XMM0:
    return L"xmm0";

  case XMM1:
    return L"xmm1";

  case XMM2:
    return L"xmm2";

  case XMM3:
    return L"xmm3";

  case XMM4:
    return L"xmm4";

  case XMM5:
    return L"xmm5";

  case XMM6:
    return L"xmm6";

  case XMM7:
    return L"xmm7";

  case XMM8:
    return L"xmm8";

  case XMM9:
    return L"xmm9";

  case XMM10:
    return L"xmm10";

  case XMM11:
    return L"xmm11";

  case XMM12:
    return L"xmm12";

  case XMM13:
    return L"xmm13";

  case XMM14:
    return L"xmm14";

  case XMM15:
    return L"xmm15";
  }

  return L"?";
}

/********************************
 * Encodes an array with the
 * binary ID of a register
 ********************************/
void Runtime::JitCompilerIA64::RegisterEncode3(unsigned char& code, long offset, Register reg)
{
#ifdef _DEBUG
  assert(offset == 2 || offset == 5);
#endif

  unsigned char reg_id;
  switch(reg) {
  case RAX:
  case XMM0:
  case R8:
  case XMM8:
    reg_id = 0x0;
    break;

  case RBX:
  case XMM3:
  case R11:
  case XMM11:
    reg_id = 0x3;
    break;

  case RCX:
  case XMM1:
  case R9:
  case XMM9:
    reg_id = 0x1;
    break;

  case RDX:
  case XMM2:
  case R10:
  case XMM10:
    reg_id = 0x2;
    break;

  case RDI:
  case XMM7:
  case R15:
  case XMM15:
    reg_id = 0x7;
    break;

  case RSI:
  case XMM6:
  case R14:
  case XMM14:
    reg_id = 0x6;
    break;

  case RSP:
  case XMM4:
  case R12:
  case XMM12:
    reg_id = 0x4;
    break;

  case RBP:
  case XMM5:
  case R13:
  case XMM13:
    reg_id = 0x5;
    break;

  default:
    wcerr << L"internal error" << endl;
    exit(1);
    break;
  }

  if(offset == 2) {
    reg_id = reg_id << 3;
  }
  code = code | reg_id;
}

//
// Stack call
//
void Runtime::JitCompilerIA64::JitStackCallback(const long instr_id, StackInstr* instr, const long cls_id, const long mthd_id, size_t* inst, size_t* op_stack, long* stack_pos, StackFrame** call_stack, long* call_stack_pos, const long ip)
{
#ifdef _DEBUG
  wcout << L"Stack Call: instr=" << instr_id
    << L", oper_1=" << instr->GetOperand() << L", oper_2=" << instr->GetOperand2()
    << L", oper_3=" << instr->GetOperand3() << L", self=" << inst << L"("
    << (size_t)inst << L"), stack=" << op_stack << L", stack_addr=" << stack_pos
    << L", stack_pos=" << (*stack_pos) << endl;
#endif
  switch(instr_id) {
  case MTHD_CALL:
  case DYN_MTHD_CALL: {

#ifdef _DEBUG
    StackMethod* called = program->GetClass(instr->GetOperand())->GetMethod(instr->GetOperand2());
    wcout << L"jit oper: MTHD_CALL: mthd=" << called->GetName() << endl;
#endif
    StackInterpreter intpr(call_stack, call_stack_pos);
    intpr.Execute(op_stack, stack_pos, ip, program->GetClass(cls_id)->GetMethod(mthd_id), inst, true);
  }
    break;

  case LOAD_ARY_SIZE: {
    size_t* array = (size_t*)PopInt(op_stack, stack_pos);
    if(!array) {
      wcerr << L"Attempting to dereference a 'Nil' memory instance" << endl;
      wcerr << L"  native method: name=" << program->GetClass(cls_id)->GetMethod(mthd_id)->GetName() << endl;
      exit(1);
    }
    PushInt(op_stack, stack_pos, array[2]);
  }
    break;

  case NEW_BYTE_ARY: {
    size_t indices[8];
    size_t value = PopInt(op_stack, stack_pos);
    size_t size = value;
    indices[0] = value;
    long dim = 1;
    for(long i = 1; i < instr->GetOperand(); ++i) {
      size_t value = PopInt(op_stack, stack_pos);
      size *= value;
      indices[dim++] = value;
    }

    // nullptr terminated string workaround
    size++;
    size_t* mem = MemoryManager::AllocateArray((long)(size + ((dim + 2) * sizeof(size_t))), BYTE_ARY_TYPE, op_stack, *stack_pos);
    mem[0] = size;
    mem[1] = dim;
    memcpy(mem + 2, indices, dim * sizeof(size_t));
    PushInt(op_stack, stack_pos, (size_t)mem);

#ifdef _DEBUG
    wcout << L"jit oper: NEW_BYTE_ARY: dim=" << dim << L"; size=" << size
      << L"; index=" << (*stack_pos) << L"; mem=" << mem << endl;
#endif
  }
    break;

  case NEW_CHAR_ARY: {
    size_t indices[8];
    size_t value = PopInt(op_stack, stack_pos);
    size_t size = value;
    indices[0] = value;
    long dim = 1;
    for(long i = 1; i < instr->GetOperand(); ++i) {
      size_t value = PopInt(op_stack, stack_pos);
      size *= value;
      indices[dim++] = value;
    }

    size++;
    size_t* mem = (size_t*)MemoryManager::AllocateArray((long)(size + ((dim + 2) * sizeof(size_t))), CHAR_ARY_TYPE, op_stack, *stack_pos);
    mem[0] = size - 1;
    mem[1] = dim;
    memcpy(mem + 2, indices, dim * sizeof(size_t));
    PushInt(op_stack, stack_pos, (size_t)mem);

#ifdef _DEBUG
    wcout << L"jit oper: NEW_CHAR_ARY: dim=" << dim << L"; size=" << size
      << L"; index=" << (*stack_pos) << L"; mem=" << mem << endl;
#endif
  }
    break;

  case NEW_INT_ARY: {
    size_t indices[8];
    size_t value = PopInt(op_stack, stack_pos);
    size_t size = value;
    indices[0] = value;
    long dim = 1;
    for(long i = 1; i < instr->GetOperand(); ++i) {
      size_t value = PopInt(op_stack, stack_pos);
      size *= value;
      indices[dim++] = value;
    }

    size_t* mem = (size_t*)MemoryManager::AllocateArray((long)(size + dim + 2), INT_TYPE, op_stack, *stack_pos);
#ifdef _DEBUG
    wcout << L"jit oper: NEW_INT_ARY: dim=" << dim << L"; size=" << size
      << L"; index=" << (*stack_pos) << L"; mem=" << mem << endl;
#endif
    mem[0] = size;
    mem[1] = dim;
    memcpy(mem + 2, indices, dim * sizeof(size_t));
    PushInt(op_stack, stack_pos, (size_t)mem);
  }
    break;

  case NEW_FLOAT_ARY: {
    size_t indices[8];
    size_t value = PopInt(op_stack, stack_pos);
    size_t size = value;
    indices[0] = value;
    long dim = 1;
    for(long i = 1; i < instr->GetOperand(); ++i) {
      size_t value = PopInt(op_stack, stack_pos);
      size *= value;
      indices[dim++] = value;
    }

    size_t* mem = (size_t*)MemoryManager::AllocateArray((long)(size + dim + 2), INT_TYPE, op_stack, *stack_pos);
    mem[0] = size;
    mem[1] = dim;

    memcpy(mem + 2, indices, dim * sizeof(size_t));
    PushInt(op_stack, stack_pos, (size_t)mem);
  }
    break;

  case NEW_OBJ_INST: {
#ifdef _DEBUG
    StackClass* klass = program->GetClass(instr->GetOperand());
    wcout << L"jit oper: NEW_OBJ_INST: class=" << klass->GetName() << endl;
#endif
    size_t* mem = MemoryManager::AllocateObject(instr->GetOperand(), op_stack, *stack_pos);
    PushInt(op_stack, stack_pos, (size_t)mem);
  }
    break;

  case OBJ_TYPE_OF: {
    size_t* mem = (size_t*)PopInt(op_stack, stack_pos);
    size_t* result = MemoryManager::ValidObjectCast(mem, instr->GetOperand(), program->GetHierarchy(), program->GetInterfaces());
    if(result) {
      PushInt(op_stack, stack_pos, 1);
    }
    else {
      PushInt(op_stack, stack_pos, 0);
    }
  }
    break;

  case OBJ_INST_CAST: {
    size_t* mem = (size_t*)PopInt(op_stack, stack_pos);
    long to_id = instr->GetOperand();
#ifdef _DEBUG
    wcout << L"jit oper: OBJ_INST_CAST: from=" << mem << L", to=" << to_id << endl;
#endif  
    size_t result = (size_t)MemoryManager::ValidObjectCast(mem, to_id, program->GetHierarchy(), program->GetInterfaces());
    if(!result && mem) {
      StackClass* to_cls = MemoryManager::GetClass(mem);
      wcerr << L">>> Invalid object cast: '" << (to_cls ? to_cls->GetName() : L"?")
        << L"' to '" << program->GetClass(to_id)->GetName() << L"' <<<" << endl;
      wcerr << L"  native method: name=" << program->GetClass(cls_id)->GetMethod(mthd_id)->GetName() << endl;
      exit(1);
    }
    PushInt(op_stack, stack_pos, result);
  }
    break;

    //----------- threads -----------

  case THREAD_JOIN: {
    size_t* instance = inst;
    if(!instance) {
      wcerr << L"Attempting to dereference a 'Nil' memory instance" << endl;
      wcerr << L"  native method: name=" << program->GetClass(cls_id)->GetMethod(mthd_id)->GetName() << endl;
      exit(1);
    }

#ifdef _WIN64
    HANDLE vm_thread = (HANDLE)instance[0];
    if(WaitForSingleObject(vm_thread, INFINITE) != WAIT_OBJECT_0) {
      wcerr << L"Unable to join thread!" << endl;
      exit(-1);
    }
#else
    void* status;
    pthread_t vm_thread = (pthread_t)instance[0];
    if(pthread_join(vm_thread, &status)) {
      wcerr << L"Unable to join thread!" << endl;
      exit(-1);
    }
#endif      
  }
    break;

  case THREAD_SLEEP:
#ifdef _WIN64    
    Sleep((DWORD)PopInt(op_stack, stack_pos));
#else
    usleep(PopInt(op_stack, stack_pos));
#endif      
    break;

  case THREAD_MUTEX: {
    size_t* instance = inst;
    if(!instance) {
      wcerr << L"Attempting to dereference a 'Nil' memory instance" << endl;
      wcerr << L"  native method: name=" << program->GetClass(cls_id)->GetMethod(mthd_id)->GetName() << endl;
      exit(1);
    }
#ifdef _WIN64      
    InitializeCriticalSection((CRITICAL_SECTION*)& instance[1]);
#else
    pthread_mutex_init((pthread_mutex_t*)& instance[1], nullptr);
#endif    
  }
    break;

  case CRITICAL_START: {
    size_t* instance = (size_t*)PopInt(op_stack, stack_pos);
    if(!instance) {
      wcerr << L"Attempting to dereference a 'Nil' memory instance" << endl;
      wcerr << L"  native method: name=" << program->GetClass(cls_id)->GetMethod(mthd_id)->GetName() << endl;
      exit(1);
    }
#ifdef _WIN64      
    EnterCriticalSection((CRITICAL_SECTION*)& instance[1]);
#else
    pthread_mutex_lock((pthread_mutex_t*)& instance[1]);
#endif      
  }
     break;

  case CRITICAL_END: {
    size_t* instance = (size_t*)PopInt(op_stack, stack_pos);
    if(!instance) {
      wcerr << L"Attempting to dereference a 'Nil' memory instance" << endl;
      wcerr << L"  native method: name=" << program->GetClass(cls_id)->GetMethod(mthd_id)->GetName() << endl;
      exit(1);
    }
#ifdef _WIN64      
    LeaveCriticalSection((CRITICAL_SECTION*)& instance[1]);
#else
    pthread_mutex_unlock((pthread_mutex_t*)& instance[1]);
#endif      
  }
    break;

    // ---------------- memory copy ----------------
  case CPY_BYTE_ARY: {
    long length = (long)PopInt(op_stack, stack_pos);
    const long src_offset = (long)PopInt(op_stack, stack_pos);
    size_t* src_array = (size_t*)PopInt(op_stack, stack_pos);
    const long dest_offset = (long)PopInt(op_stack, stack_pos);
    size_t* dest_array = (size_t*)PopInt(op_stack, stack_pos);

    if(!src_array || !dest_array) {
      wcerr << L">>> Attempting to dereference a 'Nil' memory instance <<<" << endl;
      wcerr << L"  native method: name=" << program->GetClass(cls_id)->GetMethod(mthd_id)->GetName() << endl;
      exit(1);
    }

    const long src_array_len = (long)src_array[2];
    const long dest_array_len = (long)dest_array[2];
    if(length > 0 && src_offset + length <= src_array_len && dest_offset + length <= dest_array_len) {
      char* src_array_ptr = (char*)(src_array + 3);
      char* dest_array_ptr = (char*)(dest_array + 3);
      memcpy(dest_array_ptr + dest_offset, src_array_ptr + src_offset, length);
      PushInt(op_stack, stack_pos, 1);
    }
    else {
      PushInt(op_stack, stack_pos, 0);
    }
  }
    break;

  case CPY_CHAR_ARY: {
    long length = (long)PopInt(op_stack, stack_pos);
    const long src_offset = (long)PopInt(op_stack, stack_pos);
    size_t* src_array = (size_t*)PopInt(op_stack, stack_pos);
    const long dest_offset = (long)PopInt(op_stack, stack_pos);
    size_t* dest_array = (size_t*)PopInt(op_stack, stack_pos);

    if(!src_array || !dest_array) {
      wcerr << L">>> Attempting to dereference a 'Nil' memory instance <<<" << endl;
      wcerr << L"  native method: name=" << program->GetClass(cls_id)->GetMethod(mthd_id)->GetName() << endl;
      exit(1);
    }

    const long src_array_len = (long)src_array[2];
    const long dest_array_len = (long)dest_array[2];

    if(length > 0 && src_offset + length <= src_array_len && dest_offset + length <= dest_array_len) {
      const wchar_t* src_array_ptr = (wchar_t*)(src_array + 3);
      wchar_t* dest_array_ptr = (wchar_t*)(dest_array + 3);
      memcpy(dest_array_ptr + dest_offset, src_array_ptr + src_offset, length * sizeof(wchar_t));
      PushInt(op_stack, stack_pos, 1);
    }
    else {
      PushInt(op_stack, stack_pos, 0);
    }
  }
    break;

  case CPY_INT_ARY: {
    long length = (long)PopInt(op_stack, stack_pos);
    const long src_offset = (long)PopInt(op_stack, stack_pos);
    size_t* src_array = (size_t*)PopInt(op_stack, stack_pos);
    const long dest_offset = (long)PopInt(op_stack, stack_pos);
    size_t* dest_array = (size_t*)PopInt(op_stack, stack_pos);

    if(!src_array || !dest_array) {
      wcerr << L">>> Attempting to dereference a 'Nil' memory instance <<<" << endl;
      wcerr << L"  native method: name=" << program->GetClass(cls_id)->GetMethod(mthd_id)->GetName() << endl;
      exit(1);
    }

    const long src_array_len = (long)src_array[0];
    const long dest_array_len = (long)dest_array[0];
    if(length > 0 && src_offset + length <= src_array_len && dest_offset + length <= dest_array_len) {
      size_t* src_array_ptr = src_array + 3;
      size_t* dest_array_ptr = dest_array + 3;
      memcpy(dest_array_ptr + dest_offset, src_array_ptr + src_offset, length * sizeof(size_t));
      PushInt(op_stack, stack_pos, 1);
    }
    else {
      PushInt(op_stack, stack_pos, 0);
    }
  }
    break;

  case CPY_FLOAT_ARY: {
    long length = (long)PopInt(op_stack, stack_pos);
    const long src_offset = (long)PopInt(op_stack, stack_pos);
    size_t* src_array = (size_t*)PopInt(op_stack, stack_pos);
    const long dest_offset = (long)PopInt(op_stack, stack_pos);
    size_t* dest_array = (size_t*)PopInt(op_stack, stack_pos);

    if(!src_array || !dest_array) {
      wcerr << L">>> Attempting to dereference a 'Nil' memory instance <<<" << endl;
      wcerr << L"  native method: name=" << program->GetClass(cls_id)->GetMethod(mthd_id)->GetName() << endl;
      exit(1);
    }

    const long src_array_len = (long)src_array[0];
    const long dest_array_len = (long)dest_array[0];
    if(length > 0 && src_offset + length <= src_array_len && dest_offset + length <= dest_array_len) {
      size_t* src_array_ptr = src_array + 3;
      size_t* dest_array_ptr = dest_array + 3;
      memcpy(dest_array_ptr + dest_offset, src_array_ptr + src_offset, length * sizeof(FLOAT_VALUE));
      PushInt(op_stack, stack_pos, 1);
    }
    else {
      PushInt(op_stack, stack_pos, 0);
    }
  }
    break;

  case TRAP:
  case TRAP_RTRN:
    if(!TrapProcessor::ProcessTrap(program, inst, op_stack, stack_pos, nullptr)) {
      wcerr << L"  JIT compiled machine code..." << endl;
      exit(1);
    }
    break;

#ifdef _DEBUG
  default:
    wcerr << L"Unknown callback!" << endl;
    break;

    wcout << L"  ending stack: pos=" << (*stack_pos) << endl;
#endif
  }
}

Runtime::RegisterHolder* Runtime::JitCompilerIA64::ArrayIndex(StackInstr* instr, MemoryType type)
{
  RegInstr* holder = working_stack.front();
  working_stack.pop_front();

  RegisterHolder* array_holder;
  switch(holder->GetType()) {
  case IMM_INT:
    wcerr << L">>> trying to index a constant! <<<" << endl;
    exit(1);
    break;

  case REG_INT:
    array_holder = holder->GetRegister();
    break;

  case MEM_INT:
    array_holder = GetRegister();
    move_mem_reg((long)holder->GetOperand(), RBP, array_holder->GetRegister());
    break;

  default:
    wcerr << L"internal error" << endl;
    exit(1);
    break;
  }
  CheckNilDereference(array_holder->GetRegister());

  /* Algorithm:
   long index = PopInt();
   const long dim = instr->GetOperand();

   for(int i = 1; i < dim; ++i) {
     index *= array[i];
     index += PopInt();
   }
  */

  if(holder) {
    delete holder;
    holder = nullptr;
  }

  // get initial index
  RegisterHolder* index_holder;
  holder = working_stack.front();
  working_stack.pop_front();
  switch(holder->GetType()) {
  case IMM_INT:
    index_holder = GetRegister();
    move_imm_reg((long)holder->GetOperand(), index_holder->GetRegister());
    break;

  case REG_INT:
    index_holder = holder->GetRegister();
    break;

  case MEM_INT:
    index_holder = GetRegister();
    move_mem_reg((long)holder->GetOperand(), RBP, index_holder->GetRegister());
    break;

  default:
    wcerr << L"internal error" << endl;
    exit(1);
    break;
  }

  const long dim = instr->GetOperand();
  for(int i = 1; i < dim; ++i) {
    // index *= array[i];
    mul_mem_reg((i + 2) * sizeof(size_t), array_holder->GetRegister(), index_holder->GetRegister());
    if(holder) {
      delete holder;
      holder = nullptr;
    }

    holder = working_stack.front();
    working_stack.pop_front();
    switch(holder->GetType()) {
    case IMM_INT:
      add_imm_reg((long)holder->GetOperand(), index_holder->GetRegister());
      break;

    case REG_INT:
      add_reg_reg(holder->GetRegister()->GetRegister(), index_holder->GetRegister());
      break;

    case MEM_INT:
      add_mem_reg((long)holder->GetOperand(), RBP, index_holder->GetRegister());
      break;

    default:
      break;
    }
  }

  // bounds check
  RegisterHolder* bounds_holder = GetRegister();
#ifdef _WIN64    
  move_mem_reg32(0, array_holder->GetRegister(), bounds_holder->GetRegister());
#else
  move_mem_reg(0, array_holder->GetRegister(), bounds_holder->GetRegister());
#endif    

  // ajust indices
  switch(type) {
  case BYTE_ARY_TYPE:
    break;

  case CHAR_ARY_TYPE:
#ifdef _WIN64    
    shl_imm_reg(1, index_holder->GetRegister());
    shl_imm_reg(1, bounds_holder->GetRegister());
#else
    shl_imm_reg(2, index_holder->GetRegister());
    shl_imm_reg(2, bounds_holder->GetRegister());
#endif      
    break;

  case INT_TYPE:
  case FLOAT_TYPE:
    shl_imm_reg(3, index_holder->GetRegister());
    shl_imm_reg(3, bounds_holder->GetRegister());
    break;

  default:
    break;
  }
  CheckArrayBounds(index_holder->GetRegister(), bounds_holder->GetRegister());
  ReleaseRegister(bounds_holder);

  // skip first 2 integers (size and dimension) and all dimension indices
  add_imm_reg((instr->GetOperand() + 2) * sizeof(size_t), index_holder->GetRegister());
  add_reg_reg(index_holder->GetRegister(), array_holder->GetRegister());
  ReleaseRegister(index_holder);

  delete holder;
  holder = nullptr;

  return array_holder;
}

void Runtime::JitCompilerIA64::ProcessIndices()
{
#ifdef _DEBUG
  wcout << L"Calculating indices for variables..." << endl;
#endif
  multimap<long, StackInstr*> values;
  for(long i = 0; i < method->GetInstructionCount(); ++i) {
    StackInstr* instr = method->GetInstruction(i);
    switch(instr->GetType()) {
    case LOAD_LOCL_INT_VAR:
    case LOAD_CLS_INST_INT_VAR:
    case STOR_LOCL_INT_VAR:
    case STOR_CLS_INST_INT_VAR:
    case LOAD_FUNC_VAR:
    case STOR_FUNC_VAR:
    case COPY_LOCL_INT_VAR:
    case COPY_CLS_INST_INT_VAR:
    case LOAD_FLOAT_VAR:
    case STOR_FLOAT_VAR:
    case COPY_FLOAT_VAR:
      values.insert(pair<long, StackInstr*>(instr->GetOperand(), instr));
      break;

    default:
      break;
    }
  }

  long index = RED_ZONE;
  long last_id = -1;
  multimap<long, StackInstr*>::iterator value;
  for(value = values.begin(); value != values.end(); ++value) {
    long id = value->first;
    StackInstr* instr = value->second;
    // instance reference
    if(instr->GetOperand2() == INST || instr->GetOperand2() == CLS) {
      instr->SetOperand3(instr->GetOperand() * sizeof(size_t));
    }
    // local reference
    else {
      // note: all local variables are allocated in 4 or 8 bytes
      // blocks depending upon type
      if(last_id != id) {
        switch(instr->GetType()) {
        case LOAD_LOCL_INT_VAR:
        case LOAD_CLS_INST_INT_VAR:
        case STOR_LOCL_INT_VAR:
        case STOR_CLS_INST_INT_VAR:
        case COPY_LOCL_INT_VAR:
        case COPY_CLS_INST_INT_VAR:
          index -= sizeof(size_t);
          break;

        case LOAD_FUNC_VAR:
        case STOR_FUNC_VAR:
          index -= sizeof(size_t) * 2;
          break;

        default:
          index -= sizeof(double);
          break;
        }
      }
      instr->SetOperand3(index);
      last_id = id;
    }
#ifdef _DEBUG
    if(instr->GetOperand2() == INST || instr->GetOperand2() == CLS) {
      wcout << L"native memory: index=" << instr->GetOperand() << L"; jit index="
        << instr->GetOperand3() << endl;
    }
    else {
      wcout << L"native stack: index=" << instr->GetOperand() << L"; jit index="
        << instr->GetOperand3() << endl;
    }
#endif
  }
  org_local_space = local_space = -(index + TMP_REG_5);

#ifdef _DEBUG
  wcout << L"Local space required: " << (local_space + 16) << L" byte(s)" << endl;
#endif
}

bool Runtime::JitCompilerIA64::Compile(StackMethod* cm)
{
  compile_success = true;

  if(!cm->GetNativeCode()) {
#ifdef _TIMING
    clock_t start = clock();
#endif
    skip_jump = false;
    method = cm;

#ifdef _DEBUG
    long cls_id = method->GetClass()->GetId();
    long mthd_id = method->GetId();
    wcout << L"---------- Compiling Native Code: method_id=" << cls_id << L","
      << mthd_id << L"; mthd_name='" << method->GetName() << L"'; params="
      << method->GetParamCount() << L" ----------" << endl;
#endif  
    // code buffer memory
    code_buf_max = BUFFER_SIZE;
    code = (unsigned char*)malloc(code_buf_max);

    // floats memory
#ifdef _WIN64

    floats = (double*)VirtualAlloc(nullptr, sizeof(double) * MAX_DBLS, MEM_COMMIT, PAGE_READWRITE);
    if(!floats) {
      wcerr << L"Unable to allocate JIT memory for floats!" << endl;
      exit(1);
    }
#else
    if(posix_memalign((void**)& floats, PAGE_SIZE, sizeof(double) * MAX_DBLS)) {
      wcerr << L"Unable to reallocate JIT memory!" << endl;
      exit(1);
    }
#endif    
    floats_index = instr_index = code_index = epilog_index = instr_count = 0;

    rax_reg = new RegisterHolder(RAX);
#ifdef _WIN64
    // general use registers
    aval_regs.push_back(new RegisterHolder(RDX));
    aval_regs.push_back(new RegisterHolder(RCX));
    aval_regs.push_back(new RegisterHolder(RBX));
    aval_regs.push_back(rax_reg);
    // aux general use registers
    aux_regs.push(new RegisterHolder(RSI));
    aux_regs.push(new RegisterHolder(RDI));
    // floating point registers
    aval_xregs.push_back(new RegisterHolder(XMM15));
    aval_xregs.push_back(new RegisterHolder(XMM14));
    aval_xregs.push_back(new RegisterHolder(XMM13));
    aval_xregs.push_back(new RegisterHolder(XMM12));
    aval_xregs.push_back(new RegisterHolder(XMM11));
    aval_xregs.push_back(new RegisterHolder(XMM10));
#ifdef _DEBUG
    wcout << L"Compiling code for Windows AMD64 architecture..." << endl;
#endif
#else
    // general use registers
    aval_regs.push_back(new RegisterHolder(RDX));
    aval_regs.push_back(new RegisterHolder(RCX));
    aval_regs.push_back(new RegisterHolder(RBX));
    aval_regs.push_back(rax_reg);
    // aux general use registers
    //        aux_regs.push(new RegisterHolder(RDI));
    //        aux_regs.push(new RegisterHolder(RSI));
    aux_regs.push(new RegisterHolder(R15));
    aux_regs.push(new RegisterHolder(R14));
    aux_regs.push(new RegisterHolder(R13));
    // aux_regs.push(new RegisterHolder(R12));
    aux_regs.push(new RegisterHolder(R11));
    aux_regs.push(new RegisterHolder(R10));
    // aux_regs.push(new RegisterHolder(R9));
    aux_regs.push(new RegisterHolder(R8));
    // floating point registers
    aval_xregs.push_back(new RegisterHolder(XMM15));
    aval_xregs.push_back(new RegisterHolder(XMM14));
    aval_xregs.push_back(new RegisterHolder(XMM13));
    aval_xregs.push_back(new RegisterHolder(XMM12));
    aval_xregs.push_back(new RegisterHolder(XMM11));
    aval_xregs.push_back(new RegisterHolder(XMM10));
#ifdef _DEBUG
    wcout << L"Compiling code for Posix AMD64 architecture..." << endl;
#endif
#endif

    // process offsets
    ProcessIndices();

    // setup
    Prolog();

    // method information
#ifdef _WIN64    
    move_reg_mem(RCX, CLS_ID, RBP);
    move_reg_mem(RDX, MTHD_ID, RBP);
    move_reg_mem(R8, CLASS_MEM, RBP);
    move_reg_mem(R9, INSTANCE_MEM, RBP);
#else
    move_reg_mem(RDI, CLS_ID, RBP);
    move_reg_mem(RSI, MTHD_ID, RBP);
    move_reg_mem(RDX, CLASS_MEM, RBP);
    move_reg_mem(RCX, INSTANCE_MEM, RBP);
    move_reg_mem(R8, OP_STACK, RBP);
    move_reg_mem(R9, STACK_POS, RBP);
#endif

    // register root
    RegisterRoot();

    // translate parameters
    ProcessParameters(method->GetParamCount());
    // translate program
    ProcessInstructions();
    if(!compile_success) {
#ifdef _WIN64
      VirtualFree(floats, 0, MEM_RELEASE);
#else
      free(floats);
#endif
      floats = nullptr;

      return false;
    }

    // show content
    unordered_map<long, StackInstr*>::iterator iter;
    for(iter = jump_table.begin(); iter != jump_table.end(); ++iter) {
      StackInstr* instr = iter->second;
      const long src_offset = iter->first;
      const long dest_index = method->GetLabelIndex(instr->GetOperand()) + 1;
      const long dest_offset = method->GetInstruction(dest_index)->GetOffset();
      const long offset = dest_offset - src_offset - 4; // 64-bit jump offset
      memcpy(&code[src_offset], &offset, 4);
#ifdef _DEBUG
      wcout << L"jump update: src=" << src_offset
        << L"; dest=" << dest_offset << endl;
#endif
    }

    for(size_t i = 0; i < nil_deref_offsets.size(); ++i) {
      const long index = nil_deref_offsets[i];
      long offset = epilog_index - index + 1;
      memcpy(&code[index], &offset, 4);
    }

    for(size_t i = 0; i < bounds_less_offsets.size(); ++i) {
      const long index = bounds_less_offsets[i];
      long offset = epilog_index - index + 16;
      memcpy(&code[index], &offset, 4);
    }

    for(size_t i = 0; i < bounds_greater_offsets.size(); ++i) {
      const long index = bounds_greater_offsets[i];
      long offset = epilog_index - index + 31;
      memcpy(&code[index], &offset, 4);
    }

#ifdef _DEBUG
    wcout << L"Caching JIT code: actual=" << code_index
      << L", buffer=" << code_buf_max << L" byte(s)" << endl;
#endif
    // store compiled code
    method->SetNativeCode(new NativeCode(page_manager->GetPage(code, code_index), code_index, floats));

    free(code);
    code = nullptr;

#ifdef _TIMING
    wcout << L"JIT compiling: method='" << method->GetName() << L"', time="
      << (double)(clock() - start) / CLOCKS_PER_SEC << L" second(s)." << endl;
#endif

    compile_success = true;
  }

  return compile_success;
}

/********************************
 * JitExecutor class
 ********************************/
StackProgram* JitExecutor::program;
void JitExecutor::Initialize(StackProgram* p) {
  program = p;
}

long JitExecutor::ExecuteMachineCode(long cls_id, long mthd_id, size_t* inst, unsigned char* code, const long code_size,
                                     size_t* op_stack, long* stack_pos, StackFrame** call_stack, long* call_stack_pos, StackFrame* frame) 
{
  // create function
  // notes: pointers to jit_memand jit_offset are updated by JIT code
  jit_fun_ptr jit_fun = (jit_fun_ptr)code;
  const long status = jit_fun(cls_id, mthd_id, method->GetClass()->GetClassMemory(), inst, op_stack,
                              stack_pos, call_stack, call_stack_pos, &(frame->jit_mem), &(frame->jit_offset));

#ifdef _DEBUG
  wcout << L"JIT return=: " << status << endl;
#endif 

  return status;
}

/********************************
 * RegInstr class
 ********************************/
Runtime::RegInstr::RegInstr(StackInstr* si)
{
  switch(si->GetType()) {
  case LOAD_CHAR_LIT:
  case LOAD_INT_LIT:
    type = IMM_INT;
    operand = si->GetOperand();
    break;

  case LOAD_CLS_MEM:
    type = MEM_INT;
    operand = CLASS_MEM;
    break;

  case LOAD_INST_MEM:
    type = MEM_INT;
    operand = INSTANCE_MEM;
    break;

  case LOAD_LOCL_INT_VAR:
  case LOAD_CLS_INST_INT_VAR:
  case STOR_LOCL_INT_VAR:
  case STOR_CLS_INST_INT_VAR:
  case LOAD_FUNC_VAR:
  case STOR_FUNC_VAR:
  case COPY_LOCL_INT_VAR:
  case COPY_CLS_INST_INT_VAR:
    type = MEM_INT;
    operand = si->GetOperand3();
    break;

  case LOAD_FLOAT_VAR:
  case STOR_FLOAT_VAR:
  case COPY_FLOAT_VAR:
    type = MEM_FLOAT;
    operand = si->GetOperand3();
    break;

  default:
#ifdef _DEBUG
    assert(false);
#endif
    break;
  }
  instr = si;
  holder = nullptr;
}

/********************************
 * Manage executable buffers of memory
 ********************************/
Runtime::PageHolder::PageHolder()
{
  index = 0;
  available = PAGE_SIZE;

#ifdef _WIN64    
  buffer = (unsigned char*)VirtualAlloc(nullptr, PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  if(!buffer) {
    wcerr << L"Unable to allocate JIT memory!" << endl;
    exit(1);
  }
#else
  if(posix_memalign((void**)& buffer, PAGE_SIZE, PAGE_SIZE)) {
    wcerr << L"Unable to allocate JIT memory!" << endl;
    exit(1);
  }

  if(mprotect(buffer, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC) < 0) {
    wcerr << L"Unable to mprotect" << endl;
    exit(1);
  }
#endif
}

Runtime::PageManager::PageManager()
{
  for(int i = 0; i < 4; ++i) {
    holders.push_back(new PageHolder(PAGE_SIZE * (i + 1)));
  }
}

Runtime::PageManager::~PageManager()
{
  while(!holders.empty()) {
    PageHolder* tmp = holders.front();
    holders.erase(holders.begin());
    // delete
    delete tmp;
    tmp = nullptr;
  }
}

unsigned char* Runtime::PageManager::GetPage(unsigned char* code, int32_t size)
{
  bool placed = false;

  unsigned char* temp = nullptr;
  for(size_t i = 0; !placed && i < holders.size(); ++i) {
    PageHolder* holder = holders[i];
    if(holder->CanAddCode(size)) {
      temp = holder->AddCode(code, size);
      placed = true;
    }
  }

  if(!placed) {
    PageHolder* buffer = new PageHolder(size);
    temp = buffer->AddCode(code, size);
    holders.push_back(buffer);
  }

  return temp;
}
