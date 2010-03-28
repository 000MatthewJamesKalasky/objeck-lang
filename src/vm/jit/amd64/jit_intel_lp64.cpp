/***************************************************************************
 * JIT compiler for the x86 architecture.
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
 * "AS IS" AND ANY EXPRESS OR IPLIED WARRANTIES, INCLUDING, BUT NOT 
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

#include "jit_intel_lp64.h"
#include <string>

using namespace Runtime;

/********************************
 * JitCompilerIA64 class
 ********************************/
StackProgram* JitCompilerIA64::program;
void JitCompilerIA64::Initialize(StackProgram* p) {
  program = p;
}

void JitCompilerIA64::Prolog() {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [<prolog>]" << endl;
#endif

  local_space += 8;
  BYTE_VALUE buffer[4];
  ByteEncode32(buffer, local_space);

  BYTE_VALUE setup_code[] = {
    // setup stack frame
    0x48, 0x55,                                    // push %rbp
    0x48, 0x89, 0xe5,                              // mov  %rsp, %rbp    
    0x48, 0x81, 0xec,                              // sub  $imm, %rsp
    buffer[0], buffer[1], buffer[2], buffer[3],      
                                                   // save registers
    0x48, 0x50,                                    // push rax
    0x48, 0x53,                                    // push rbx
    /****/
    0x48, 0x51,                                    // push rcx
    0x48, 0x52,                                    // push rdx
    0x48, 0x57,                                    // push rdi
    0x48, 0x56,                                    // push rsi
    0x49, 0x50,                                    // push r8
    0x49, 0x51,                                    // push r9
    /****/
    0x49, 0x52,                                    // push r10
    0x49, 0x53,                                    // push r11
    0x49, 0x54,                                    // push r12
    0x49, 0x55,                                    // push r13
    0x49, 0x56,                                    // push r14
    0x49, 0x57,                                    // push r15
  };
  const long setup_size = sizeof(setup_code);
  // copy setup
  for(long i = 0; i < setup_size; i++) {
    AddMachineCode(setup_code[i]);
  }
}

void JitCompilerIA64::Epilog(long imm) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [<epilog>]" << endl;
#endif
  
  move_mem_reg(RTRN_VALUE, RBP, RAX);
  move_mem_reg(0, RAX, RAX);
  move_imm_reg(imm, RAX);
  
  BYTE_VALUE teardown_code[] = {
    // restore registers
    0x49, 0x5f,       // pop r15
    0x49, 0x5e,       // pop r14
    0x49, 0x5d,       // pop r13
    0x49, 0x5c,       // pop r12
    0x49, 0x5b,       // pop r11
    0x49, 0x5a,       // pop r10
    /****/
    0x49, 0x59,       // pop r9
    0x49, 0x58,       // pop r8
    0x48, 0x5e,       // pop rsi
    0x48, 0x5f,       // pop rdi
    0x48, 0x5a,       // pop rdx
    0x48, 0x59,       // pop rcx
    /****/
    0x48, 0x5b,       // pop rbx
    0x48, 0x58,       // pop rax    
    // tear down stack frame and return
    0x48, 0x89, 0xec, // mov  %rbp, %rsp
    0x48, 0x5d,       // pop %rbp
    0x48, 0xc3        // rtn
  };
  const long teardown_size = sizeof(teardown_code);
  // copy teardown
  for(long i = 0; i < teardown_size; i++) {
    AddMachineCode(teardown_code[i]);
  }
}

void JitCompilerIA64::RegisterRoot() {
  // caculate root address
  RegisterHolder* holder = GetRegister();
  // note: -8 is the offset requried to 
  // get to the first local variale
  const long offset = local_space + TMP_REG_5 - 8;
  move_reg_reg(RBP, holder->GetRegister());
  sub_imm_reg(offset, holder->GetRegister());
  // push call values

  move_imm_reg(offset + TMP_REG_5, R8);
  move_reg_reg(holder->GetRegister(), RCX);
  move_mem_reg(INSTANCE_MEM, RBP, RDX);
  move_mem_reg(MTHD_ID, RBP, RSI);
  move_mem_reg(CLS_ID, RBP, RDI);
  
  /*
  push_imm(offset + TMP_REG_5);
  push_reg(holder->GetRegister());
  push_mem(INSTANCE_MEM, RBP);
  push_mem(MTHD_ID, RBP);
  push_mem(CLS_ID, RBP);
  */
  // call method
  RegisterHolder* call_holder = GetRegister();
  move_imm_reg((long)MemoryManager::AddJitMethodRoot, call_holder->GetRegister());
  call_reg(call_holder->GetRegister());
  // add_imm_reg(20, RSP);
  // clean up
  ReleaseRegister(holder);
  ReleaseRegister(call_holder);
}

void JitCompilerIA64::UnregisterRoot() {
  // caculate root address
  RegisterHolder* holder = GetRegister();
  move_reg_reg(RBP, holder->GetRegister());
  // note: -8 is the offset requried to 
  // get to the first local variable
  sub_imm_reg(local_space + TMP_REG_5 - 8, 
	      holder->GetRegister());
  // push call value
  move_reg_reg(holder->GetRegister(), RDI);
  // call method
  RegisterHolder* call_holder = GetRegister();
  move_imm_reg((long)MemoryManager::RemoveJitMethodRoot, call_holder->GetRegister());
  call_reg(call_holder->GetRegister());
  // clean up
  // add_imm_reg(4, RSP);
  ReleaseRegister(holder);
  ReleaseRegister(call_holder);
}

void JitCompilerIA64::ProcessParameters(long params) {
#ifdef _DEBUG
  cout << "CALLED_PARMS: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
  
  for(long i = 0; i < params; i++) {
    RegisterHolder* op_stack_holder = GetRegister();
    move_mem_reg(OP_STACK, RBP, op_stack_holder->GetRegister());

    StackInstr* instr = method->GetInstruction(instr_index++);
    instr->SetOffset(code_index);  

    RegisterHolder* stack_pos_holder = GetRegister();
    move_mem_reg(STACK_POS, RBP, stack_pos_holder->GetRegister());
      
    if(instr->GetType() == STOR_INT_VAR) {
      RegisterHolder* dest_holder = GetRegister();
      dec_mem(0, stack_pos_holder->GetRegister());  
      move_mem_reg(0, stack_pos_holder->GetRegister(), 
		   stack_pos_holder->GetRegister());
      shl_reg(stack_pos_holder->GetRegister(), 3);
      add_reg_reg(stack_pos_holder->GetRegister(),
		  op_stack_holder->GetRegister());
      move_mem_reg(0, op_stack_holder->GetRegister(), 
		   dest_holder->GetRegister());
      working_stack.push_front(new RegInstr(dest_holder));
      // store int
      ProcessStore(instr);
    }
    else {
      RegisterHolder* dest_holder = GetXmmRegister();
      sub_imm_mem(2, 0, stack_pos_holder->GetRegister());
      move_mem_reg(0, stack_pos_holder->GetRegister(), 
		   stack_pos_holder->GetRegister());
      shl_reg(stack_pos_holder->GetRegister(), 4);
      add_reg_reg(stack_pos_holder->GetRegister(),
		  op_stack_holder->GetRegister()); 
      move_mem_xreg(0, op_stack_holder->GetRegister(), 
		    dest_holder->GetRegister());
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
  cout << "INT_CALL: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
  
  RegisterHolder* op_stack_holder = GetRegister();
  move_mem_reg(OP_STACK, RBP, op_stack_holder->GetRegister());
  
  RegisterHolder* stack_pos_holder = GetRegister();
  move_mem_reg(STACK_POS, RBP, stack_pos_holder->GetRegister());
  
  dec_mem(0, stack_pos_holder->GetRegister());  
  move_mem_reg(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
  shl_reg(stack_pos_holder->GetRegister(), 3);
  add_reg_reg(stack_pos_holder->GetRegister(), op_stack_holder->GetRegister());  
  move_mem_reg(0, op_stack_holder->GetRegister(), op_stack_holder->GetRegister());
  working_stack.push_front(new RegInstr(op_stack_holder));
  
  ReleaseRegister(stack_pos_holder);
}

void JitCompilerIA64::ProcessFloatCallParameter() {
#ifdef _DEBUG
  cout << "FLOAT_CALL: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
  
  RegisterHolder* op_stack_holder = GetRegister();
  move_mem_reg(OP_STACK, RBP, op_stack_holder->GetRegister());
  
  RegisterHolder* stack_pos_holder = GetRegister();
  move_mem_reg(STACK_POS, RBP, stack_pos_holder->GetRegister());
  
  RegisterHolder* dest_holder = GetXmmRegister();
  sub_imm_mem(2, 0, stack_pos_holder->GetRegister());
  move_mem_reg(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
  shl_reg(stack_pos_holder->GetRegister(), 4);
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
    case LOAD_INT_LIT:
#ifdef _DEBUG
      cout << "LOAD_INT: value=" << instr->GetOperand() 
	   << "; regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      working_stack.push_front(new RegInstr(instr));
      break;

      // float literal
    case LOAD_FLOAT_LIT:
#ifdef _DEBUG
      cout << "LOAD_FLOAT_LIT: value=" << instr->GetFloatOperand() 
	   << "; regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      floats[floats_index] = instr->GetFloatOperand();
      working_stack.push_front(new RegInstr(instr, &floats[floats_index++]));
      break;
      
      // load self
    case LOAD_INST_MEM: {
#ifdef _DEBUG
      cout << "LOAD_INST_MEM; regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      working_stack.push_front(new RegInstr(instr));
    }
      break;

      // load self
    case LOAD_CLS_MEM: {
#ifdef _DEBUG
      cout << "LOAD_CLS_MEM; regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      RegisterHolder* holder = GetRegister();
      move_imm_reg((long)method->GetClass()->GetClassMemory(), holder->GetRegister());
      working_stack.push_front(new RegInstr(holder));
    }
      break;
      
      // load variable
    case LOAD_INT_VAR:
    case LOAD_FLOAT_VAR:
#ifdef _DEBUG
      cout << "LOAD_INT_VAR/LOAD_FLOAT_VAR: id=" << instr->GetOperand() << "; regs=" 
	   << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessLoad(instr);
      break;
    
      // store value
    case STOR_INT_VAR:
    case STOR_FLOAT_VAR:
#ifdef _DEBUG
      cout << "STOR_INT_VAR/STOR_FLOAT_VAR: id=" << instr->GetOperand() 
	   << "; regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessStore(instr);
      break;

      // copy value
    case COPY_INT_VAR:
    case COPY_FLOAT_VAR:
#ifdef _DEBUG
      cout << "COPY_INT_VAR/COPY_FLOAT_VAR: id=" << instr->GetOperand() 
	   << "; regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
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
      // comparison
    case LES_INT:
    case GTR_INT:
    case LES_EQL_INT:
    case GTR_EQL_INT:
    case EQL_INT:
    case NEQL_INT:
#ifdef _DEBUG
      cout << "INT ADD/SUB/MUL/DIV/MOD/LES/GTR/EQL/NEQL: regs=" 
	   << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessIntCalculation(instr);
      break;

    case SHL_INT:
    case SHR_INT:
#ifdef _DEBUG
      cout << "SHL/SHR: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessIntShift(instr);
      break;

    case ADD_FLOAT:
    case SUB_FLOAT:
    case MUL_FLOAT:
    case DIV_FLOAT:
#ifdef _DEBUG
      cout << "FLOAT ADD/SUB/MUL/DIV/: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessFloatCalculation(instr);
      break;

    case LES_FLOAT:
    case GTR_FLOAT:
    case LES_EQL_FLOAT:
    case GTR_EQL_FLOAT:
    case EQL_FLOAT:
    case NEQL_FLOAT: {
#ifdef _DEBUG
      cout << "FLOAT LES/GTR/EQL/NEQL: regs=" << aval_regs.size() << "," 
	   << aux_regs.size() << endl;
#endif
      ProcessFloatCalculation(instr);

      RegInstr* left = working_stack.front();
      working_stack.pop_front(); // pop invalid xmm register
      ReleaseXmmRegister(left->GetRegister());

      delete left; 
      left = NULL;
      
      RegisterHolder* holder = GetRegister();
      cmov_reg(holder->GetRegister(), instr->GetType());
      working_stack.push_front(new RegInstr(holder));
      
    }
      break;
      
    case RTRN:
#ifdef _DEBUG
      cout << "RTRN: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessReturn();
      // unregister root
      UnregisterRoot();
      // teardown
      Epilog(1);
      break;
      
    case MTHD_CALL: {
      StackMethod* called_method = program->GetClass(instr->GetOperand())->GetMethod(instr->GetOperand2());
      if(called_method) {
#ifdef _DEBUG
	assert(called_method);
	cout << "MTHD_CALL: id="<< instr->GetOperand() << "," << instr->GetOperand2() << ", params=" 
	     << (called_method->GetParamCount() + 1) << "; regs=" << aval_regs.size() << "," 
	     << aux_regs.size() << endl;
#endif      
	// passing instance variable
	ProcessStackCallback(MTHD_CALL, instr, instr_index, called_method->GetParamCount() + 1);
      
	switch(called_method->GetReturn()) {
	case INT_TYPE:
	  ProcessReturnParameters(true);      
	  break;
	
	case FLOAT_TYPE:
	  ProcessReturnParameters(false);
	  break;
	}
      }
    }
      break;
      
    case NEW_BYTE_ARY:
#ifdef _DEBUG
      cout << "NEW_BYTE_ARY: dim=" << instr->GetOperand() << " regs=" << aval_regs.size()
	   << "," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(NEW_BYTE_ARY, instr, instr_index, instr->GetOperand());
      ProcessReturnParameters(true);
      break;
      
    case NEW_INT_ARY:
#ifdef _DEBUG
      cout << "NEW_INT_ARY: dim=" << instr->GetOperand() << " regs=" << aval_regs.size() 
	   << "," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(NEW_INT_ARY, instr, instr_index, instr->GetOperand());
      ProcessReturnParameters(true);
      break;

    case NEW_FLOAT_ARY:
#ifdef _DEBUG
      cout << "NEW_FLOAT_ARY: dim=" << instr->GetOperand() << " regs=" << aval_regs.size() 
	   << "," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(NEW_FLOAT_ARY, instr, instr_index, instr->GetOperand());
      ProcessReturnParameters(true);
      break;
      
    case NEW_OBJ_INST: {
#ifdef _DEBUG
      cout << "NEW_OBJ_INST: id=" << instr->GetOperand() << "regs=" << aval_regs.size()
	   << "," << aux_regs.size() << endl;
#endif
      // note: object id passed in instruction param
      ProcessStackCallback(NEW_OBJ_INST, instr, instr_index, 0);
      ProcessReturnParameters(true);
    }
      break;
      
    case TRAP:
#ifdef _DEBUG
      cout << "TRAP: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(TRAP, instr, instr_index, instr->GetOperand());
      break;

    case TRAP_RTRN:
#ifdef _DEBUG
      cout << "TRAP_RTRN: args=" << instr->GetOperand() << "; regs=" 
	   << aval_regs.size() << "," << aux_regs.size() << endl;
      assert(instr->GetOperand());
#endif      
      ProcessStackCallback(TRAP_RTRN, instr, instr_index, instr->GetOperand());
      ProcessReturnParameters(true);
      break;
      
    case STOR_BYTE_ARY_ELM:
#ifdef _DEBUG
      cout << "STOR_BYTE_ARY_ELM: regs=" << aval_regs.size() << "," 
	   << aux_regs.size() << endl;
#endif
      ProcessStoreByteElement(instr);
      break;
      
    case STOR_INT_ARY_ELM:
#ifdef _DEBUG
      cout << "STOR_INT_ARY_ELM: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessStoreIntElement(instr);
      break;

    case STOR_FLOAT_ARY_ELM:
#ifdef _DEBUG
      cout << "STOR_FLOAT_ARY_ELM: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessStoreFloatElement(instr);
      break;

    case POP_INT:
    case POP_FLOAT: {
#ifdef _DEBUG
      cout << "POP_INT/POP_FLOAT: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      // note: there may be constants that aren't 
      // in registers and don't need to be popped
      if(!working_stack.empty()) {
	// pop and release
	RegInstr* left = working_stack.front();
	working_stack.pop_front(); 
	if(left->GetType() == REG_32) {
	  ReleaseRegister(left->GetRegister());
	}
	else if(left->GetType() == REG_64) {
	  ReleaseXmmRegister(left->GetRegister());
	}
	// clean up
	delete left;
	left = NULL;
      }
    }
      break;

    case FLOR_FLOAT:
#ifdef _DEBUG
      cout << "FLOR_FLOAT: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessFloor(instr);
      break;

    case CEIL_FLOAT:
#ifdef _DEBUG
      cout << "CEIL_FLOAT: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessCeiling(instr);
      break;
      
    case F2I:
#ifdef _DEBUG
      cout << "F2I: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessFloatToInt(instr);
      break;

    case I2F:
#ifdef _DEBUG
      cout << "I2F: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessIntToFloat(instr);
      break;
      
    case LOAD_ARY_SIZE:
#ifdef _DEBUG
      cout << "LOAD_ARY_SIZE: regs=" << aval_regs.size() << "," 
	   << aux_regs.size() << endl;
#endif
      // note: array passed in stack
      ProcessStackCallback(LOAD_ARY_SIZE, instr, instr_index, 1);
      ProcessReturnParameters(true);
      break;

    case OBJ_INST_CAST: {
#ifdef _DEBUG
      cout << "OBJ_INST_CAST: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessStackCallback(OBJ_INST_CAST, instr, instr_index, 1);
      ProcessReturnParameters(true);
    }
      break;
      
    case LOAD_BYTE_ARY_ELM:
#ifdef _DEBUG
      cout << "LOAD_BYTE_ARY_ELM: regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
      ProcessLoadByteElement(instr);
      break;
      
    case LOAD_INT_ARY_ELM:
#ifdef _DEBUG
      cout << "LOAD_INT_ARY_ELM: regs=" << aval_regs.size() << "," 
	   << aux_regs.size() << endl;
#endif
      ProcessLoadIntElement(instr);
      break;

    case LOAD_FLOAT_ARY_ELM:
#ifdef _DEBUG
      cout << "LOAD_FLOAT_ARY_ELM: regs=" << aval_regs.size() << "," 
	   << aux_regs.size() << endl;
#endif
      ProcessLoadFloatElement(instr);
      break;
      
    case JMP:
      ProcessJump(instr);
      break;
      
    case LBL:
#ifdef _DEBUG
      cout << "LBL: id=" << instr->GetOperand() << endl;
#endif
      break;
      
    default: {
      InstructionType error = (InstructionType)instr->GetType();
      cerr << "Unknown instruction: " << error << "!" << endl;
      exit(1);
    }
      break;
    }
  }
}

void JitCompilerIA64::ProcessIntShift(StackInstr* instruction) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();

  switch(left->GetType()) {
    // immediate
  case IMM_32: {
    RegisterHolder* holder = GetRegister();
    move_imm_reg(left->GetOperand(), holder->GetRegister());
    if(instruction->GetType() == SHL_INT) {
      shl_reg(holder->GetRegister(), instruction->GetOperand());
    }
    else {
      shr_reg(holder->GetRegister(), instruction->GetOperand());
    }
    working_stack.push_front(new RegInstr(holder));
  }
    break;
    
    // register
  case REG_32: {
    RegisterHolder* holder = left->GetRegister();
    if(instruction->GetType() == SHL_INT) {
      shl_reg(holder->GetRegister(), instruction->GetOperand());
    }
    else {
      shr_reg(holder->GetRegister(), instruction->GetOperand());
    }
    working_stack.push_front(new RegInstr(left->GetRegister()));
  }
    break;
    
    // memory
  case MEM_32: {
    RegisterHolder* holder = GetRegister();
    move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
    if(instruction->GetType() == SHL_INT) {
      shl_reg(holder->GetRegister(), instruction->GetOperand());
    }
    else {
      shr_reg(holder->GetRegister(), instruction->GetOperand());
    }
    working_stack.push_front(new RegInstr(holder));
  }
    break;
  }
    
  delete left;
  left = NULL;
}

void JitCompilerIA64::ProcessLoad(StackInstr* instr) {
  // method/function memory
  if(instr->GetOperand2() == LOCL) {
    working_stack.push_front(new RegInstr(instr));
  }
  // class or instance memory
  else {
    RegInstr* left = working_stack.front();
    working_stack.pop_front();

    RegisterHolder* holder = GetRegister();
    move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
    // CheckNilDereference(holder->GetRegister());

    // long value
    if(instr->GetType() == LOAD_INT_VAR) {
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
    left = NULL;
  }
}

void JitCompilerIA64::ProcessJump(StackInstr* instr) {
  if(!skip_jump) {
#ifdef _DEBUG
    cout << "JMP: id=" << instr->GetOperand() << ", regs=" << aval_regs.size() 
	 << "," << aux_regs.size() << endl;
#endif
    if(instr->GetOperand2() < 0) {
      AddMachineCode(0xe9);
    }
    else {
      RegInstr* left = working_stack.front();
      working_stack.pop_front(); 

      switch(left->GetType()) {
      case IMM_32:{
        RegisterHolder* holder = GetRegister();
        move_imm_reg(left->GetOperand(), holder->GetRegister());
        cmp_imm_reg(instr->GetOperand2(), holder->GetRegister());
        ReleaseRegister(holder);
      }
        break;
        
      case REG_32:
        cmp_imm_reg(instr->GetOperand2(), left->GetRegister()->GetRegister());
        ReleaseRegister(left->GetRegister());
        break;

      case MEM_32: {
        RegisterHolder* holder = GetRegister();
        move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
        cmp_imm_reg(instr->GetOperand2(), holder->GetRegister());
        ReleaseRegister(holder);
      }
        break;

      default:
        cerr << ">>> Should never occur (compiler bug?) type=" << left->GetType() << " <<<" << endl;
        exit(1);
        break;
      }

      // 1 byte compare with register
      AddMachineCode(0x0f);
      AddMachineCode(0x84);
      
      // clean up
      delete left;
      left = NULL;
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
    if(left->GetType() == REG_32) {
      ReleaseRegister(left->GetRegister());
    }

    // clean up
    delete left;
    left = NULL;
  }
}

void JitCompilerIA64::ProcessReturnParameters(bool is_int) {
  if(is_int) {
    ProcessIntCallParameter();
  }
  else {
    ProcessFloatCallParameter();
  }
}

void JitCompilerIA64::ProcessLoadByteElement(StackInstr* instr) {
  RegisterHolder* holder = GetRegister();
  RegisterHolder* elem_holder = ArrayIndex(instr, BYTE_ARY_TYPE);
  // CheckNilDereference(elem_holder->GetRegister());
  xor_reg_reg(holder->GetRegister(), holder->GetRegister());
  move_mem8_reg(0, elem_holder->GetRegister(), holder->GetRegister());
  ReleaseRegister(elem_holder);
  working_stack.push_front(new RegInstr(holder));
}

void JitCompilerIA64::ProcessLoadIntElement(StackInstr* instr) {
  RegisterHolder* elem_holder = ArrayIndex(instr, INT_TYPE);
  // CheckNilDereference(elem_holder->GetRegister());
  move_mem_reg(0, elem_holder->GetRegister(), elem_holder->GetRegister());
  working_stack.push_front(new RegInstr(elem_holder));
}

void JitCompilerIA64::ProcessLoadFloatElement(StackInstr* instr) {
  RegisterHolder* elem_holder = ArrayIndex(instr, FLOAT_TYPE);
  // CheckNilDereference(elem_holder->GetRegister());
  RegisterHolder* holder = GetXmmRegister();
  move_mem_xreg(0, elem_holder->GetRegister(), holder->GetRegister());
  working_stack.push_front(new RegInstr(holder));
  ReleaseRegister(elem_holder);
}

void JitCompilerIA64::ProcessStoreByteElement(StackInstr* instr) {
  RegisterHolder* elem_holder = ArrayIndex(instr, BYTE_ARY_TYPE);
  // CheckNilDereference(elem_holder->GetRegister());
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_32:
    move_imm_mem8(left->GetOperand(), 0, elem_holder->GetRegister());
    ReleaseRegister(elem_holder);
    break;

  case MEM_32: {    
    // movb can only use al, bl, cl and dl registers
    RegisterHolder* holder = GetRegister(false);
    move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
    move_reg_mem8(holder->GetRegister(), 0, elem_holder->GetRegister());
    ReleaseRegister(holder);
    ReleaseRegister(elem_holder);
  }
    break;

  case REG_32: {
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
  }
  
  delete left;
  left = NULL;
}

void JitCompilerIA64::ProcessStoreIntElement(StackInstr* instr) {
  RegisterHolder* elem_holder = ArrayIndex(instr, INT_TYPE);
  // CheckNilDereference(elem_holder->GetRegister());
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_32:
    move_imm_mem(left->GetOperand(), 0, elem_holder->GetRegister());
    break;

  case MEM_32: {
    RegisterHolder* holder = GetRegister();
    move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
    move_reg_mem(holder->GetRegister(), 0, elem_holder->GetRegister());
    ReleaseRegister(holder);
  }
    break;

  case REG_32: {
    RegisterHolder* holder = left->GetRegister();
    move_reg_mem(holder->GetRegister(), 0, elem_holder->GetRegister());
    ReleaseRegister(holder);
  }
    break;
  }
  ReleaseRegister(elem_holder);
  
  delete left;
  left = NULL;
}

void JitCompilerIA64::ProcessStoreFloatElement(StackInstr* instr) {
  RegisterHolder* elem_holder = ArrayIndex(instr, FLOAT_TYPE);
  // CheckNilDereference(elem_holder->GetRegister());
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_64:
    move_imm_memx(left, 0, elem_holder->GetRegister());
    break;

  case MEM_64: {
    RegisterHolder* holder = GetXmmRegister();
    move_mem_xreg(left->GetOperand(), 
		  RBP, holder->GetRegister());
    move_xreg_mem(holder->GetRegister(), 0, elem_holder->GetRegister());
    ReleaseXmmRegister(holder);
  }
    break;

  case REG_64: {
    RegisterHolder* holder = left->GetRegister();
    move_xreg_mem(holder->GetRegister(), 0, elem_holder->GetRegister());
    ReleaseXmmRegister(holder);
  }
    break;
  }
  ReleaseRegister(elem_holder);
  
  delete left;
  left = NULL;
}

void JitCompilerIA64::ProcessFloor(StackInstr* instr) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_64: {
    RegisterHolder* holder = GetXmmRegister();
    round_imm_xreg(left, holder->GetRegister(), true);
    working_stack.push_front(new RegInstr(holder));
    delete left;
    left = NULL;
  }
    break;
    
  case MEM_64: {
    RegisterHolder* holder = GetXmmRegister();
    round_mem_xreg(left->GetOperand(), RBP, holder->GetRegister(), true);
    working_stack.push_front(new RegInstr(holder));
    delete left;
    left = NULL;
  }
    break;
    
  case REG_64:
    round_xreg_xreg(left->GetRegister()->GetRegister(), 
		    left->GetRegister()->GetRegister(), true);
    working_stack.push_front(left);
    break;
  }
}

void JitCompilerIA64::ProcessCeiling(StackInstr* instr) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_64: {
    RegisterHolder* holder = GetXmmRegister();
    round_imm_xreg(left, holder->GetRegister(), false);
    working_stack.push_front(new RegInstr(holder));
    delete left;
    left = NULL;
  }
    break;
    
  case MEM_64: {
    RegisterHolder* holder = GetXmmRegister();
    round_mem_xreg(left->GetOperand(), RBP, holder->GetRegister(), false);
    working_stack.push_front(new RegInstr(holder));
    delete left;
    left = NULL;
  }
    break;
    
  case REG_64:
    round_xreg_xreg(left->GetRegister()->GetRegister(), 
		    left->GetRegister()->GetRegister(), false);
    working_stack.push_front(left);
    break;
  }
}

void JitCompilerIA64::ProcessFloatToInt(StackInstr* instr) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  RegisterHolder* holder = GetRegister();
  switch(left->GetType()) {
  case IMM_64:
    cvt_imm_reg(left, holder->GetRegister());
    break;
    
  case MEM_64:
    cvt_mem_reg(left->GetOperand(), 
		RBP, holder->GetRegister());
    break;

  case REG_64:
    cvt_xreg_reg(left->GetRegister()->GetRegister(), 
		 holder->GetRegister());
    ReleaseXmmRegister(left->GetRegister());
    break;
  }
  working_stack.push_front(new RegInstr(holder));

  delete left;
  left = NULL;
}

void JitCompilerIA64::ProcessIntToFloat(StackInstr* instr) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  RegisterHolder* holder = GetXmmRegister();
  switch(left->GetType()) {
  case IMM_32:
    cvt_imm_xreg(left, holder->GetRegister());
    break;
    
  case MEM_32:
    cvt_mem_xreg(left->GetOperand(), 
		 RBP, holder->GetRegister());
    break;

  case REG_32:
    cvt_reg_xreg(left->GetRegister()->GetRegister(), 
		 holder->GetRegister());
    ReleaseRegister(left->GetRegister());
    break;
  }
  working_stack.push_front(new RegInstr(holder));

  delete left;
  left = NULL;
}

void JitCompilerIA64::ProcessStore(StackInstr* instr) {
  Register dest;
  RegisterHolder* addr_holder = NULL;

  // instance/method memory
  if(instr->GetOperand2() == LOCL) {
    dest = RBP;
  }
  // class or instance memory
  else {
    RegInstr* left = working_stack.front();
    working_stack.pop_front();

    addr_holder = GetRegister();
    move_mem_reg(left->GetOperand(), RBP, addr_holder->GetRegister());
    // CheckNilDereference(addr_holder->GetRegister());
    
    dest = addr_holder->GetRegister();
    
    delete left;
    left = NULL;
  }
  
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
  case IMM_32:
    move_imm_mem(left->GetOperand(), instr->GetOperand3(), dest);
    break;

  case MEM_32: {
    RegisterHolder* holder = GetRegister();
    move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
    move_reg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    ReleaseRegister(holder);
  }
    break;
  case REG_32: {
    RegisterHolder* holder = left->GetRegister();
    move_reg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    ReleaseRegister(holder);
  }
    break;

  case IMM_64:
    move_imm_memx(left, instr->GetOperand3(), dest);
    break;
    
  case MEM_64: {
    RegisterHolder* holder = GetXmmRegister();
    move_mem_xreg(left->GetOperand(), RBP, holder->GetRegister());
    move_xreg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    ReleaseXmmRegister(holder);
  }
    break;
    
  case REG_64: {
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
  left = NULL;
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
    // CheckNilDereference(holder->GetRegister());
    dest = holder->GetRegister();
    ReleaseRegister(holder);
    
    delete left;
    left = NULL;
  }
  
  RegInstr* left = working_stack.front();
  switch(left->GetType()) {
  case IMM_32: {
    RegisterHolder* holder = GetRegister();
    move_imm_reg(left->GetOperand(), holder->GetRegister());
    move_reg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    // save register
    working_stack.pop_front();
    working_stack.push_front(new RegInstr(holder));
  }
    break;

  case MEM_32: {
    RegisterHolder* holder = GetRegister();
    move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
    move_reg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    // save register
    working_stack.pop_front();
    working_stack.push_front(new RegInstr(holder));
  }
    break;

  case REG_32: {
    RegisterHolder* holder = left->GetRegister();
    move_reg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
  }
    break;

  case IMM_64: {
    RegisterHolder* holder = GetXmmRegister();
    move_imm_xreg(left, holder->GetRegister());
    move_xreg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    // save register
    working_stack.pop_front();
    working_stack.push_front(new RegInstr(holder));
  }
    break;

  case MEM_64: {
    RegisterHolder* holder = GetXmmRegister();
    move_mem_xreg(left->GetOperand(), RBP, holder->GetRegister());
    move_xreg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
    // save register
    working_stack.pop_front();
    working_stack.push_front(new RegInstr(holder));
  }
    break;

  case REG_64: {
    RegisterHolder* holder = left->GetRegister();
    move_xreg_mem(holder->GetRegister(), instr->GetOperand3(), dest);
  }
    break;
  }
}

void JitCompilerIA64::ProcessStackCallback(long instr_id, StackInstr* instr,
					   long &instr_index, long params) {
  long non_params;
  if(params < 0) {
    non_params = 0;
  }
  else {
    non_params = working_stack.size() - params;
  }
  
#ifdef _DEBUG
  cout << "Return: params=" << params << ", non-params=" << non_params << endl;
#endif
  
  stack<RegInstr*> regs;
  stack<long> dirty_regs;
  long reg_offset = TMP_REG_0;  

  stack<RegInstr*> xmms;
  stack<long> dirty_xmms;
  long xmm_offset = TMP_XMM_0;
  
  long i = 0;     
  for(list<RegInstr*>::reverse_iterator iter = working_stack.rbegin();
      iter != working_stack.rend(); iter++) {
    RegInstr* left = (*iter);
    if(i < non_params) {
      switch(left->GetType()) {
      case REG_32:
	move_reg_mem(left->GetRegister()->GetRegister(), reg_offset, RBP);
	dirty_regs.push(reg_offset);
	regs.push(left);
	reg_offset -= sizeof(long);
	break;

      case REG_64:
	move_xreg_mem(left->GetRegister()->GetRegister(), xmm_offset, RBP);
	dirty_xmms.push(xmm_offset);
	xmms.push(left);
	xmm_offset -= sizeof(double) * 2;
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

  ProcessReturn(params);
  // push values
  push_imm(instr_index - 1);
  push_mem(STACK_POS, RBP);
  move_mem_reg(OP_STACK, RBP, R9);
  move_mem_reg(INSTANCE_MEM, RBP, R8);
  move_mem_reg(MTHD_ID, RBP, RCX);
  move_mem_reg(CLS_ID, RBP, RDX);
  move_imm_reg((long)instr, RSI);
  move_imm_reg(instr_id, RDI);
  // call function
  RegisterHolder* call_holder = GetRegister();
  move_imm_reg((long)JitCompilerIA64::StackCallback, call_holder->GetRegister());
  call_reg(call_holder->GetRegister());
  add_imm_reg(16, RSP);
  ReleaseRegister(call_holder);

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
    move_mem_reg(0, stack_pos_holder->GetRegister(), stack_pos_holder->GetRegister());
    shl_reg(stack_pos_holder->GetRegister(), 3);
    add_reg_reg(stack_pos_holder->GetRegister(), op_stack_holder->GetRegister());  

    long non_params;
    if(params < 0) {
      non_params = 0;
    }
    else {
      non_params = working_stack.size() - params;
    }
#ifdef _DEBUG
    cout << "Return: params=" << params << ", non-params=" << non_params << endl;
#endif
    
    long i = 0;     
    for(list<RegInstr*>::reverse_iterator iter = working_stack.rbegin(); 
	iter != working_stack.rend(); iter++) {
      // skip non-params... processed above
      RegInstr* left = (*iter);
      if(i < non_params) {
	i++;
      }
      else {
	move_mem_reg(STACK_POS, RBP, stack_pos_holder->GetRegister());            
	switch(left->GetType()) {
	case IMM_32:
	  move_imm_mem(left->GetOperand(), 0, op_stack_holder->GetRegister());
	  inc_mem(0, stack_pos_holder->GetRegister());
	  add_imm_reg(sizeof(long), op_stack_holder->GetRegister());
	  break;
	
	case MEM_32: {
	  RegisterHolder* temp_holder = GetRegister();
	  move_mem_reg(left->GetOperand(), RBP, temp_holder->GetRegister());
	  move_reg_mem(temp_holder->GetRegister(), 0, op_stack_holder->GetRegister());
	  inc_mem(0, stack_pos_holder->GetRegister());
	  add_imm_reg(sizeof(long), op_stack_holder->GetRegister()); 
	  ReleaseRegister(temp_holder);
	}
	  break;
	
	case REG_32:
	  move_reg_mem(left->GetRegister()->GetRegister(), 0, op_stack_holder->GetRegister());
	  inc_mem(0, stack_pos_holder->GetRegister());
	  add_imm_reg(sizeof(long), op_stack_holder->GetRegister()); 
	  break;
	
	case IMM_64:
	  move_imm_memx(left, 0, op_stack_holder->GetRegister());
	  add_imm_mem(2, 0, stack_pos_holder->GetRegister());
	  add_imm_reg(16, op_stack_holder->GetRegister()); 
	  break;
	
	case MEM_64: {
	  RegisterHolder* temp_holder = GetXmmRegister();
	  move_mem_xreg(left->GetOperand(), RBP, temp_holder->GetRegister());
	  move_xreg_mem(temp_holder->GetRegister(), 0, op_stack_holder->GetRegister());
	  add_imm_mem(2, 0, stack_pos_holder->GetRegister());
	  add_imm_reg(16, op_stack_holder->GetRegister());
	  ReleaseXmmRegister(temp_holder); 
	}
	  break;
	
	case REG_64:
	  move_xreg_mem(left->GetRegister()->GetRegister(), 0, op_stack_holder->GetRegister());
	  add_imm_mem(2, 0, stack_pos_holder->GetRegister());
	  add_imm_reg(16, op_stack_holder->GetRegister());
	  break;
	}    
      }
    }
    ReleaseRegister(op_stack_holder);
    ReleaseRegister(stack_pos_holder);
    
    // clean up working stack
    if(params < 0) {
      params = working_stack.size();
    }
    for(long i = 0; i < params; i++) {
      RegInstr* left = working_stack.front();
      working_stack.pop_front();

      // release register
      switch(left->GetType()) {
      case REG_32:
	ReleaseRegister(left->GetRegister());
	break;

      case REG_64:
	ReleaseXmmRegister(left->GetRegister());
	break;
      }
      // clean up
      delete left;
      left = NULL;
    }
  }
}

void JitCompilerIA64::ProcessIntCalculation(StackInstr* instruction) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();

  RegInstr* right = working_stack.front();
  working_stack.pop_front();

  switch(left->GetType()) {
    // intermidate
  case IMM_32:
    switch(right->GetType()) {
    case IMM_32: {
      RegisterHolder* holder = GetRegister();
      move_imm_reg(left->GetOperand(), holder->GetRegister());
      math_imm_reg(right->GetOperand(), holder->GetRegister(), 
		   instruction->GetType());
      working_stack.push_front(new RegInstr(holder));
    }
      break;

    case REG_32: {
      RegisterHolder* holder = right->GetRegister();
      math_imm_reg(left->GetOperand(), holder->GetRegister(), instruction->GetType());
      working_stack.push_front(new RegInstr(holder));
    }
      break;

    case MEM_32: {
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
    }	    
    break; 

    // register
  case REG_32:
    switch(right->GetType()) {
    case IMM_32: {
      RegisterHolder* holder = left->GetRegister();
      math_imm_reg(right->GetOperand(), holder->GetRegister(), 
		   instruction->GetType());
      working_stack.push_front(new RegInstr(holder));
    }
      break;
    case REG_32: {
      RegisterHolder* holder = right->GetRegister();
      math_reg_reg(holder->GetRegister(), left->GetRegister()->GetRegister(), instruction->GetType());
      working_stack.push_front(new RegInstr(left->GetRegister()));
      ReleaseRegister(holder);
    }
      break;
    case MEM_32: {
      RegisterHolder* lhs = left->GetRegister();
      RegisterHolder* rhs = GetRegister();
      move_mem_reg(right->GetOperand(), RBP, rhs->GetRegister());
      math_reg_reg(rhs->GetRegister(), lhs->GetRegister(), instruction->GetType());
      ReleaseRegister(rhs);
      working_stack.push_front(new RegInstr(lhs));
    }
      break;
    }
    break;

    // memory
  case MEM_32:
    switch(right->GetType()) {
    case IMM_32: {
      RegisterHolder* holder = GetRegister();
      move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
      math_imm_reg(right->GetOperand(), holder->GetRegister(),
		   instruction->GetType());
      working_stack.push_front(new RegInstr(holder));
    }
      break;
    case REG_32: {
      RegisterHolder* lhs = right->GetRegister();
      RegisterHolder* rhs = GetRegister();
      move_mem_reg(left->GetOperand(), RBP, rhs->GetRegister());
      math_reg_reg(lhs->GetRegister(), rhs->GetRegister(), instruction->GetType());
      ReleaseRegister(lhs);
      working_stack.push_front(new RegInstr(rhs));
    }
      break;
    case MEM_32: {
      RegisterHolder* holder = GetRegister();
      move_mem_reg(left->GetOperand(), RBP, holder->GetRegister());
      math_mem_reg(right->GetOperand(), holder->GetRegister(), instruction->GetType());
      working_stack.push_front(new RegInstr(holder));
    }
      break;
    }
    break;
  }

  delete left;
  left = NULL;
    
  delete right;
  right = NULL;
}

void JitCompilerIA64::ProcessFloatCalculation(StackInstr* instruction) {
  RegInstr* left = working_stack.front();
  working_stack.pop_front();
  
  RegInstr* right = working_stack.front();
  working_stack.pop_front();
  
  switch(left->GetType()) {
    // intermidate
  case IMM_64:
    switch(right->GetType()) {
    case IMM_64: {
      RegisterHolder* left_holder = GetXmmRegister();
      move_imm_xreg(left, left_holder->GetRegister());      
      RegisterHolder* right_holder = GetXmmRegister();
      move_imm_xreg(right, right_holder->GetRegister());      
      math_xreg_xreg(right_holder->GetRegister(), left_holder->GetRegister(), 
		     instruction->GetType());
      ReleaseXmmRegister(right_holder);
      working_stack.push_front(new RegInstr(left_holder));
    }
      break;
      
    case REG_64: {      
      RegisterHolder* imm_holder = GetXmmRegister();
      move_imm_xreg(left, imm_holder->GetRegister());
      InstructionType type = instruction->GetType();
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

    case MEM_64: {
      RegisterHolder* holder = GetXmmRegister();
      move_mem_xreg(right->GetOperand(), RBP, holder->GetRegister());

      RegisterHolder* imm_holder = GetXmmRegister();
      move_imm_xreg(left, imm_holder->GetRegister());

      InstructionType type = instruction->GetType();
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
    }	    
    break; 

    // register
  case REG_64:
    switch(right->GetType()) {
    case IMM_64: {
      RegisterHolder* left_holder = left->GetRegister();
      RegisterHolder* right_holder = GetXmmRegister();
      move_imm_xreg(right, right_holder->GetRegister());
      math_xreg_xreg(right_holder->GetRegister(), left_holder->GetRegister(), 
		     instruction->GetType());
      ReleaseXmmRegister(right_holder);      
      working_stack.push_front(new RegInstr(left_holder));
    }
      break;

    case REG_64: {
      RegisterHolder* holder = right->GetRegister();
      math_xreg_xreg(holder->GetRegister(), left->GetRegister()->GetRegister(), instruction->GetType());
      working_stack.push_front(new RegInstr(left->GetRegister()));
      ReleaseXmmRegister(holder);
    }
      break;
      
    case MEM_64: {
      RegisterHolder* holder = left->GetRegister();
      math_mem_xreg(right->GetOperand(), holder->GetRegister(), instruction->GetType());
      working_stack.push_front(new RegInstr(holder));
    }
      break;
    }
    break;

    // memory
  case MEM_64:
    switch(right->GetType()) {
    case IMM_64: {
      RegisterHolder* holder = GetXmmRegister();
      move_mem_xreg(left->GetOperand(), RBP, holder->GetRegister());

      RegisterHolder* imm_holder = GetXmmRegister();
      move_imm_xreg(right, imm_holder->GetRegister());

      InstructionType type = instruction->GetType();
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

    case REG_64: {
      RegisterHolder* holder = right->GetRegister();
      math_mem_xreg(left->GetOperand(), holder->GetRegister(), instruction->GetType());
      working_stack.push_front(new RegInstr(holder));
    }
      break;

    case MEM_64: {
      RegisterHolder* left_holder = GetXmmRegister();
      move_mem_xreg(left->GetOperand(), RBP, left_holder->GetRegister());

      RegisterHolder* right_holder = GetXmmRegister();
      move_mem_xreg(right->GetOperand(), RBP, right_holder->GetRegister());

      math_xreg_xreg(right_holder->GetRegister(), left_holder->GetRegister(),  
		     instruction->GetType());

      ReleaseXmmRegister(right_holder);
      working_stack.push_front(new RegInstr(left_holder));
    }
      break;
    }
    break;
  }

  delete left;
  left = NULL;
    
  delete right;
  right = NULL;
}

/////////////////// OPERATIONS ///////////////////

void JitCompilerIA64::move_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [movl %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x89);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::move_reg_mem8(Register src, long offset, Register dest) { 
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [movb %" << GetRegisterName(src) 
       << ", " << offset << "(%" << GetRegisterName(dest) << ")" << "]" 
       << endl;
#endif
  // encode
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x88);
  AddMachineCode(ModRM(dest, src));
  // write value
  AddImm(offset);
}
    
void JitCompilerIA64::move_reg_mem(Register src, long offset, Register dest) { 
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [movl %" << GetRegisterName(src) 
       << ", " << offset << "(%" << GetRegisterName(dest) << ")" << "]" 
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
  cout << "  " << (++instr_count) << ": [movb " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest)
       << "]" << endl;
#endif
  // encode
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0xb6);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::move_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [movl " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest)
       << "]" << endl;
#endif
  // encode
  AddMachineCode(RXB(dest, src));
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
  cout << "  " << (++instr_count) << ": [movb $" << imm << ", " << offset 
       << "(%" << GetRegisterName(dest) << ")" << "]" << endl;
#endif
  // encode
  AddMachineCode(XB(dest));
  AddMachineCode(0xc6);
  BYTE_VALUE code = 0x80;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  // write value
  AddImm(offset);
  AddMachineCode(imm);
}

void JitCompilerIA64::move_imm_mem(long imm, long offset, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [movl $" << imm << ", " << offset 
       << "(%" << GetRegisterName(dest) << ")" << "]" << endl;
#endif
  // encode
  AddMachineCode(XB(dest));
  AddMachineCode(0xc7);    
  BYTE_VALUE code = 0x80;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  // write value
  AddImm(offset);
  AddImm(imm);
}

void JitCompilerIA64::move_imm_reg(long imm, Register reg) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [movl $" << imm << ", %" 
       << GetRegisterName(reg) << "]" << endl;
#endif
  // encode
  AddMachineCode(XB(reg));
  BYTE_VALUE code = 0xb8;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  // write value
  AddImm64(imm);
}

void JitCompilerIA64::move_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());  
  move_mem_xreg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}
    
void JitCompilerIA64::move_mem_xreg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [movsd " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest)
       << "]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x10);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}
    
void JitCompilerIA64::move_xreg_mem(Register src, long offset, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [movsd %" << GetRegisterName(src) 
       << ", " << offset << "(%" << GetRegisterName(dest) << ")" << "]" 
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
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [movsd %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(src, dest));
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x11);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

bool JitCompilerIA64::cond_jmp(InstructionType type) {
  if(instr_index >= method->GetInstructionCount()) {
    return false;
  }
  
  StackInstr* next_instr = method->GetInstruction(instr_index);
  if(next_instr->GetType() == JMP && next_instr->GetOperand2() > -1) {
    // if(false) {
#ifdef _DEBUG
    cout << "JMP: id=" << next_instr->GetOperand() << ", regs=" << aval_regs.size() << "," << aux_regs.size() << endl;
#endif
    AddMachineCode(0x0f);

    //
    // jump if true
    //
    if(next_instr->GetOperand2() == 1) {
      switch(type) {
      case LES_INT:	
#ifdef _DEBUG
        cout << "  " << (++instr_count) << ": [jl]" << endl;
#endif
        AddMachineCode(0x8c);
        break;

      case GTR_INT:
#ifdef _DEBUG
        cout << "  " << (++instr_count) << ": [jg]" << endl;
#endif
        AddMachineCode(0x8f);
        break;

      case EQL_INT:
#ifdef _DEBUG
        cout << "  " << (++instr_count) << ": [je]" << endl;
#endif
        AddMachineCode(0x84);
        break;

      case NEQL_INT:
#ifdef _DEBUG
        cout << "  " << (++instr_count) << ": [jne]" << endl;
#endif
        AddMachineCode(0x85);
        break;

      case LES_EQL_INT:
#ifdef _DEBUG
        cout << "  " << (++instr_count) << ": [jle]" << endl;
#endif
        AddMachineCode(0x8e);
        break;
        
      case GTR_EQL_INT:
#ifdef _DEBUG
        cout << "  " << (++instr_count) << ": [jge]" << endl;
#endif
        AddMachineCode(0x8d);
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
        cout << "  " << (++instr_count) << ": [jge]" << endl;
#endif
        AddMachineCode(0x8d);
        break;

      case GTR_INT:
#ifdef _DEBUG
        cout << "  " << (++instr_count) << ": [jle]" << endl;
#endif
        AddMachineCode(0x8e);
        break;

      case EQL_INT:
#ifdef _DEBUG
        cout << "  " << (++instr_count) << ": [jne]" << endl;
#endif
        AddMachineCode(0x85);
        break;

      case NEQL_INT:
#ifdef _DEBUG
        cout << "  " << (++instr_count) << ": [je]" << endl;
#endif
        AddMachineCode(0x84);
        break;

      case LES_EQL_INT:
#ifdef _DEBUG
        cout << "  " << (++instr_count) << ": [jg]" << endl;
#endif
        AddMachineCode(0x8f);
        break;
        
      case GTR_EQL_INT:
#ifdef _DEBUG
        cout << "  " << (++instr_count) << ": [jl]" << endl;
#endif
        AddMachineCode(0x8c);
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

void JitCompilerIA64::math_imm_reg(long imm, Register reg, InstructionType type) {
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
  }
}

void JitCompilerIA64::math_reg_reg(Register src, Register dest, InstructionType type) {
  switch(type) {
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
  }
}

void JitCompilerIA64::math_mem_reg(long offset, Register reg, InstructionType type) {
  switch(type) {
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
    cmp_imm_xreg(instr, reg);
    break;
    
  case GTR_FLOAT:
  case EQL_FLOAT:
  case NEQL_FLOAT:
  case GTR_EQL_FLOAT:
    cmp_imm_xreg(instr, reg);
    break;
  }
}

void JitCompilerIA64::math_mem_xreg(long offset, Register dest, InstructionType type) {
  RegisterHolder* holder = GetXmmRegister();
  move_mem_xreg(offset, RBP, holder->GetRegister());
  math_xreg_xreg(dest, holder->GetRegister(), type);
  move_xreg_xreg(holder->GetRegister(), dest);
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
    cmp_xreg_xreg(dest, src);
    break;

  case GTR_FLOAT:
  case EQL_FLOAT:
  case NEQL_FLOAT:
  case GTR_EQL_FLOAT:
    cmp_xreg_xreg(src, dest);
    break;
  }
}    

void JitCompilerIA64::cmp_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [cmpll %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x39);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::cmp_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [cmpl " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest) 
       << "]" << endl;
#endif
  // encode
  AddMachineCode(RXB(dest, src));
  AddMachineCode(0x3b);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}
    
void JitCompilerIA64::cmp_imm_reg(long imm, Register reg) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [cmpl $" << imm << ", %"
       << GetRegisterName(reg) << "]" << endl;
#endif
  // encode
  AddMachineCode(XB(reg));
  AddMachineCode(0x81);
  BYTE_VALUE code = 0xf8;
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
  cout << "  " << (++instr_count) << ": [cmovl %" 
       << GetRegisterName(reg) << ", %" 
       << GetRegisterName(true_holder->GetRegister()) << " ]" << endl;
#endif
  // encode
  AddMachineCode(ROB(true_holder->GetRegister(), reg));
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
    AddMachineCode(0x42);
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
    AddMachineCode(0x46);
    break;

  case GTR_EQL_FLOAT:
    AddMachineCode(0x43);
    break;

  default:
    cerr << ">>> Unknown compare! <<<" << endl;
    exit(1);
    break;
  }
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, reg);
  RegisterEncode3(code, 5, true_holder->GetRegister());
  AddMachineCode(code);
  ReleaseRegister(true_holder);
}

void JitCompilerIA64::add_imm_mem(long imm, long offset, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [addl $" << imm << ", " 
       << offset << "(%"<< GetRegisterName(dest) << ")]" << endl;
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
  cout << "  " << (++instr_count) << ": [addl $" << imm << ", %"
       << GetRegisterName(reg) << "]" << endl;
#endif
  // encode
  AddMachineCode(B(reg));
  AddMachineCode(0x81);
  BYTE_VALUE code = 0xc0;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  // write value
  AddImm(imm);
}
    
void JitCompilerIA64::add_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
  add_mem_xreg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::sub_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
  sub_mem_xreg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::div_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
  div_mem_xreg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::mul_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
  mul_mem_xreg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::add_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [addl %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x01);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::sub_xreg_xreg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [subsd %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x5c);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);	     
}

void JitCompilerIA64::mul_xreg_xreg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [mulsd %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x59);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);
}

void JitCompilerIA64::div_xreg_xreg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [divsd %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x5e);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);
}

void JitCompilerIA64::add_xreg_xreg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [addsd %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x58);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);
}
    
void JitCompilerIA64::add_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [addl " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest) 
       << "]" << endl;
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
  cout << "  " << (++instr_count) << ": [addsd " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest) 
       << "]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
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
  cout << "  " << (++instr_count) << ": [mulsd " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest) 
       << "]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
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
  cout << "  " << (++instr_count) << ": [subl $" << imm << ", %"
       << GetRegisterName(reg) << "]" << endl;
#endif
  // encode
  AddMachineCode(B(reg));
  AddMachineCode(0x81);
  BYTE_VALUE code = 0xe8;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  AddImm(imm);
}

void JitCompilerIA64::sub_imm_mem(long imm, long offset, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [subl $" << imm << ", " 
       << offset << "(%"<< GetRegisterName(dest) << ")]" << endl;
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
  cout << "  " << (++instr_count) << ": [subl %" << GetRegisterName(src)
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x29);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::sub_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [subl " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest) 
       << "]" << endl;
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
  cout << "  " << (++instr_count) << ": [imul $" << imm 
       << ", %"<< GetRegisterName(reg) << "]" << endl;
#endif
  // encode
  AddMachineCode(XB(reg));
  AddMachineCode(0x69);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, reg);
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  // write value
        floats = new FLOAT_VALUE[MAX_DBLS];
	floats_index = instr_index = code_index = instr_count = 0;

  AddImm(imm);
}

void JitCompilerIA64::mul_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [imul %" 
       << GetRegisterName(src) << ", %"<< GetRegisterName(dest) 
       << "]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0xaf);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::mul_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [imul " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest) 
       << "]" << endl;
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

void JitCompilerIA64::div_mem_reg(long offset, Register src,
				  Register dest, bool is_mod) {
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
  move_reg_reg(RAX, RDX);
  shr_reg(RDX, 31);
  
  // encode
  AddMachineCode(XB(src));
  AddMachineCode(0xf7);
  AddMachineCode(ModRM(src, RDI));
  // write value
  AddImm(offset);
  
#ifdef _DEBUG
  if(is_mod) {
    cout << "  " << (++instr_count) << ": [imod " << offset << "(%" 
	 << GetRegisterName(src) << ")]" << endl;
  }
  else {
    cout << "  " << (++instr_count) << ": [idiv " << offset << "(%" 
	 << GetRegisterName(src) << ")]" << endl;
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
  move_reg_reg(RAX, RDX);
  shr_reg(RDX, 31);
  
  if(src != RAX && src != RDX) {
    // encode
    AddMachineCode(B(src));
    AddMachineCode(0xf7);
    BYTE_VALUE code = 0xf8;
    // write value
    RegisterEncode3(code, 5, src);
    AddMachineCode(code);
    
#ifdef _DEBUG
    if(is_mod) {
      cout << "  " << (++instr_count) << ": [imod %" 
	   << GetRegisterName(src) << "]" << endl;
    }
    else {
      cout << "  " << (++instr_count) << ": [idiv %" 
	   << GetRegisterName(src) << "]" << endl;
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
      cout << "  " << (++instr_count) << ": [imod " << TMP_REG_0 << "(%" 
	   << GetRegisterName(RBP) << ")]" << endl;
    }
    else {
      cout << "  " << (++instr_count) << ": [idiv " << TMP_REG_0 << "(%" 
	   << GetRegisterName(RBP) << ")]" << endl;
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
  BYTE_VALUE code = 0x48;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [decl %" 
       << GetRegisterName(dest) << "]" << endl;
#endif
}

void JitCompilerIA64::dec_mem(long offset, Register dest) {
  AddMachineCode(XB(dest));
  AddMachineCode(0xff);
  BYTE_VALUE code = 0x88;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  AddImm(offset);
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [decl " << offset << "(%" 
       << GetRegisterName(dest) << ")" << "]" << endl;
#endif
}

void JitCompilerIA64::inc_mem(long offset, Register dest) {
  AddMachineCode(XB(dest));
  AddMachineCode(0xff);
  BYTE_VALUE code = 0x80;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  AddImm(offset);
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [incl " << offset << "(%" 
       << GetRegisterName(dest) << ")" << "]" << endl;
#endif
}

void JitCompilerIA64::shl_reg(Register dest, long value) {
  AddMachineCode(B(dest));
  AddMachineCode(0xc1);
  BYTE_VALUE code = 0xe0;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  AddMachineCode(value);
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [shl $" << value << ", %" 
       << GetRegisterName(dest) << "]" << endl;
#endif
}

void JitCompilerIA64::shl_mem(long offset, Register src, long value) {
  AddMachineCode(XB(src));
  AddMachineCode(0xc1);
  AddMachineCode(ModRM(src, RSP));
  AddImm(offset);
  AddMachineCode(value);

#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [shl " << offset << "(%" 
       << GetRegisterName(src) << "), $" << value << "]" << endl;
#endif
}

void JitCompilerIA64::shr_reg(Register dest, long value) {
  AddMachineCode(B(dest));
  AddMachineCode(0xc1);
  BYTE_VALUE code = 0xe8;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  AddMachineCode(value);
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [shr $" << value << ", %" 
       << GetRegisterName(dest) << "]" << endl;
#endif
}

void JitCompilerIA64::shr_mem(long offset, Register src, long value) {
  AddMachineCode(XB(src));
  AddMachineCode(0xc1);
  AddMachineCode(ModRM(src, RBP));
  AddImm(offset);
  AddMachineCode(value);
  
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [shr " << offset << "(%" 
       << GetRegisterName(src) << "), $" << value << "]" << endl;
#endif
}

void JitCompilerIA64::push_mem(long offset, Register dest) {
  AddMachineCode(B(dest));
  AddMachineCode(0xff);
  BYTE_VALUE code = 0xb0;
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
  AddImm(offset);
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [pushl " << offset << "(%" 
       << GetRegisterName(dest) << ")" << "]" << endl;
#endif
}

void JitCompilerIA64::push_reg(Register reg) {
  AddMachineCode(B(reg));
  BYTE_VALUE code = 0x50;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [pushl %" << GetRegisterName(reg) 
       << "]" << endl;
#endif
}

void JitCompilerIA64::push_imm(long value) {
  AddMachineCode(0x68);
  AddImm(value);
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [pushl $" << value << "]" << endl;
#endif
}

void JitCompilerIA64::pop_reg(Register reg) {
  AddMachineCode(B(reg));  
  BYTE_VALUE code = 0x58;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [popl %" << GetRegisterName(reg) 
       << "]" << endl;
#endif
}

void JitCompilerIA64::call_reg(Register reg) {
  AddMachineCode(B(reg));  
  AddMachineCode(0xff);
  BYTE_VALUE code = 0xd0;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [call %" << GetRegisterName(reg)
       << "]" << endl;
#endif
}

void JitCompilerIA64::cmp_xreg_xreg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [ucomisd %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(0x66);
  AddMachineCode(0x0f);
  AddMachineCode(0x2e);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);
}

void JitCompilerIA64::cmp_mem_xreg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [ucomisd " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest) 
       << "]" << endl;
#endif
  // encode
  AddMachineCode(0x66);
  AddMachineCode(0x0f);
  AddMachineCode(0x2e);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::cmp_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
  cmp_mem_xreg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::cvt_xreg_reg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [cvtsd2si %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x2c);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);
}

void JitCompilerIA64::round_imm_xreg(RegInstr* instr, Register reg, bool is_floor) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
  round_mem_xreg(0, imm_holder->GetRegister(), reg, is_floor);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::round_mem_xreg(long offset, Register src, Register dest, bool is_floor) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << (is_floor ? ": [floor " : ": [ceil ") 
       << offset << "(%" << GetRegisterName(src) << "), %" << GetRegisterName(dest) 
       << "]" << endl;
#endif
  
  AddMachineCode(0x66);
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
  cout << "  " << (++instr_count) << (is_floor ? ": [floor %" : ": [ceil %") 
       << GetRegisterName(src) << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  
  AddMachineCode(0x66);
  AddMachineCode(0x0f);
  AddMachineCode(0x3a);
  AddMachineCode(0x0b);
  // registers
  BYTE_VALUE code = 0xc0;
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
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
  cvt_mem_reg(0, imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

///////////////////////// TODO /////////////////////////

void JitCompilerIA64::cvt_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [cvtsd2di " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest) 
       << "]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x2d);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::cvt_reg_xreg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [cvtsi2sd %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x2a);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, dest);
  RegisterEncode3(code, 5, src);
  AddMachineCode(code);
}

void JitCompilerIA64::cvt_imm_xreg(RegInstr* instr, Register reg) {
  // copy address of imm value
  RegisterHolder* imm_holder = GetRegister();
  move_imm_reg(instr->GetOperand(), imm_holder->GetRegister());
  cvt_reg_xreg(imm_holder->GetRegister(), reg);
  ReleaseRegister(imm_holder);
}

void JitCompilerIA64::cvt_mem_xreg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [cvtsi2sd " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest) 
       << "]" << endl;
#endif
  // encode
  AddMachineCode(0xf2);
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x0f);
  AddMachineCode(0x2a);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::and_imm_reg(long imm, Register reg) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [andl $" << imm << ", %"
       << GetRegisterName(reg) << "]" << endl;
#endif
  // encode
  AddMachineCode(B(reg));
  AddMachineCode(0x81);
  BYTE_VALUE code = 0xe0;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  // write value
  AddImm(imm);
}

void JitCompilerIA64::and_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [addl %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x21);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::and_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [andl " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest) 
       << "]" << endl;
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
  cout << "  " << (++instr_count) << ": [orl $" << imm << ", %"
       << GetRegisterName(reg) << "]" << endl;
#endif
  // encode
  AddMachineCode(B(reg));
  AddMachineCode(0x81);
  BYTE_VALUE code = 0xc8;
  RegisterEncode3(code, 5, reg);
  AddMachineCode(code);
  // write value
  AddImm(imm);
}

void JitCompilerIA64::or_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [orl %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x09);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

void JitCompilerIA64::or_mem_reg(long offset, Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [orl " << offset << "(%" 
       << GetRegisterName(src) << "), %" << GetRegisterName(dest) 
       << "]" << endl;
#endif
  // encode
  AddMachineCode(RXB(src, dest));
  AddMachineCode(0x0b);
  AddMachineCode(ModRM(src, dest));
  // write value
  AddImm(offset);
}

void JitCompilerIA64::xor_reg_reg(Register src, Register dest) {
#ifdef _DEBUG
  cout << "  " << (++instr_count) << ": [xor %" << GetRegisterName(src) 
       << ", %" << GetRegisterName(dest) << "]" << endl;
#endif
  // encode
  AddMachineCode(ROB(src, dest));
  AddMachineCode(0x31);
  BYTE_VALUE code = 0xc0;
  // write value
  RegisterEncode3(code, 2, src);
  RegisterEncode3(code, 5, dest);
  AddMachineCode(code);
}

/********************************
 * JitExecutorIA32 class
 ********************************/
StackProgram* JitExecutorIA32::program;
void JitExecutorIA32::Initialize(StackProgram* p) {
  program = p;
}

long JitExecutorIA32::ExecuteMachineCode(long cls_id, long mthd_id, long* inst, 
					 BYTE_VALUE* code, const long code_size, 
					 long* op_stack, long *stack_pos) {
  // create function
  jit_fun_ptr jit_fun = (jit_fun_ptr)code;
  
  // execute
  long rtrn_value;
  jit_fun(cls_id, mthd_id, (long*)method->GetClass()->GetClassMemory(), 
	  inst, op_stack, stack_pos, rtrn_value);
  
  return rtrn_value;
}
