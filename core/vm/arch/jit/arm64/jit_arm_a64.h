/**
 * JIT compiler for ARMv8 architecture (A1 instruction encoding)
 *
 * Copyright (c) 2020-2021f, Randy Hollines
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyrightx
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
 */

#ifndef __JIT_COMPILER__
#define __JIT_COMPILER__

#include "../../memory.h"
#include "../../../arch/posix/posix.h"
#include <sys/mman.h>
#include <errno.h>

#include "../../../common.h"
#include "../../../interpreter.h"

using namespace std;

namespace Runtime {
  // realease +16

  // offsets for ARM A64 addresses
#define INT_CONSTS 96
#define CLS_ID 88
#define MTHD_ID 80
#define CLASS_MEM 72
#define INSTANCE_MEM 64
#define OP_STACK 56
#define OP_STACK_POS 48
#define CALL_STACK 40
#define CALL_STACK_POS 32
#define JIT_MEM 24
#define JIT_OFFSET 16
  // float temps
#define TMP_D_0 104
#define TMP_D_1 112
#define TMP_D_2 120
#define TMP_D_3 128
  // integer temps
#define TMP_REG_0 136
#define TMP_REG_1 144
#define TMP_REG_2 152
#define TMP_REG_3 160
  // holds $lr for callbacks
#define TMP_REG_LR 168

#define MAX_INTS 256
#define MAX_DBLS 128
#define BUFFER_SIZE 512
#define PAGE_SIZE 4096
  
  // register type
  enum RegType {
    IMM_INT = -4000,
    REG_INT,
    MEM_INT,
    IMM_FLOAT,
    REG_FLOAT,
    MEM_FLOAT,
  };
  
  // general and float registers
  enum Register {
    X0 = 0,
    X1,
    X2,
    X3,
    X4,
    X5,
    X6,
    X7,
    XS0,
    X9,
    X10,
    X11,
    X12,
    X13,
    X14,
    X15,
    XS1,
    XS2,
    XS3,
    X19,
    X20,
    X21,
    X22,
    X23,
    X24,
    X25,
    X26,
    X27,
    X28,
    FP,
    LR,
    SP,
    // floating point
    D0 = 0,
    D1,
    D2,
    D3,
    D4,
    D5,
    D6,
    D7
  };

  /**
   * RegisterHolder class
   */
  class RegisterHolder {
    Register reg;
    bool is_float;

  public:
    RegisterHolder(Register r, bool f) {
      reg = r;
      is_float = f;
    }

    ~RegisterHolder() {
    }

    Register GetRegister() {
      return reg;
    }
    
    inline bool IsDouble() {
      return is_float;
    }
  };

  /**
   * RegInstr class
   */
  class RegInstr {
    RegType type;
    long operand;
    RegisterHolder* holder;
    StackInstr* instr;

  public:
    RegInstr(RegisterHolder* h) {
      if(h->IsDouble()) {
        type = REG_FLOAT;
      }
      else {
        type = REG_INT;
      }
      holder = h;
      instr = nullptr;
    }

    RegInstr(StackInstr* si, double* da) {
      type = IMM_FLOAT;
      operand = (long)da;
      holder = nullptr;
      instr = nullptr;
    }

    RegInstr(RegType t, long o) {
      type = t;
      operand = o;
    }

    RegInstr(StackInstr* si) {
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
        throw runtime_error("Invalid load instruction!");
        break;
      }
      instr = si;
      holder = nullptr;
    }

    ~RegInstr() {
    }

    StackInstr* GetInstruction() {
      return instr;
    }

    RegisterHolder* GetRegister() {
      return holder;
    }

    void SetType(RegType t) {
      type = t;
    }

    RegType GetType() {
      return type;
    }

    int32_t GetOperand() {
      return operand;
    }
  };
  
  /**
   * Manage executable buffers of memory
   */
  class PageHolder {
    uint32_t* buffer;
    uint32_t available, index;

  public:
    PageHolder(int32_t size) {
      index = 0;

      const uint32_t byte_size = size * sizeof(uint32_t);
      int factor = byte_size / PAGE_SIZE + 1;
      const uint32_t alloc_size = PAGE_SIZE * factor;
      
      buffer = (uint32_t*)mmap(nullptr, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, 0, 0);
      if(buffer == MAP_FAILED) {
        cerr << "unable to mmap!" << endl;
        exit(1);
      }
      
      available = alloc_size;
    }

    ~PageHolder() {
      free(buffer);
      buffer = nullptr;
    }

    inline bool CanAddCode(int32_t size) {
      const int32_t size_diff = available - size * sizeof(uint32_t);
      if(size_diff > 0) {
        return true;
      }
      
      return false;
    }
    
    uint32_t* AddCode(uint32_t* code, int32_t size);
  };
  
  class PageManager {
    vector<PageHolder*> holders;
    
  public:
    PageManager();
    ~PageManager();

    uint32_t* GetPage(uint32_t* code, int32_t size);
  };
  
  /**
   * JitCompilerA64 class
   */
  class JitCompilerA64 {
    static StackProgram* program;
    static PageManager* page_manager;
    deque<RegInstr*> working_stack;
    vector<RegisterHolder*> aval_regs;
    list<RegisterHolder*> used_regs;
    stack<RegisterHolder*> aux_regs;
    RegisterHolder* reg_eax;
    vector<RegisterHolder*> aval_xregs;
    list<RegisterHolder*> used_xregs;
    unordered_map<long, StackInstr*> jump_table;
    multimap<long, long> const_int_pool;
    vector<long> deref_offsets;          // -1
    vector<long> bounds_less_offsets;    // -2
    vector<long> bounds_greater_offsets; // -3
    long local_space;
    bool realign_stack;
    StackMethod* method;
    int32_t instr_count;
    uint32_t* code;
    long code_index;
    long epilog_index;
    long* ints;
    double* float_consts;
    long floats_index;
    long instr_index;
    long code_buf_max;
    bool compile_success;
    bool skip_jump;
    
    // setup and teardown
    void Prolog();
    void Epilog();

    // stack conversion operations
    void ProcessParameters(long count);
    void RegisterRoot();
    void ProcessInstructions();
    void ProcessLoad(StackInstr* instr);
    void ProcessStore(StackInstr* instruction);
    void ProcessCopy(StackInstr* instr);
    RegInstr* ProcessIntFold(long left_imm, long right_imm, InstructionType type);
    void ProcessIntCalculation(StackInstr* instruction);
    void ProcessFloatCalculation(StackInstr* instruction);
    void ProcessFloatOperation(StackInstr* instruction);
    void ProcessFloatOperation2(StackInstr* instruction);
    void ProcessReturn(long params = -1);
    void ProcessStackCallback(long instr_id, StackInstr* instr, long &instr_index, long params);
    void ProcessIntCallParameter();
    void ProcessFloatCallParameter();
    void ProcessFunctionCallParameter();
    void ProcessReturnParameters(MemoryType type);
    void ProcessLoadByteElement(StackInstr* instr);
    void ProcessLoadCharElement(StackInstr* instr);
    void ProcessStoreByteElement(StackInstr* instr);
    void ProcessStoreCharElement(StackInstr* instr);
    void ProcessLoadIntElement(StackInstr* instr);
    void ProcessStoreIntElement(StackInstr* instr);
    void ProcessLoadFloatElement(StackInstr* instr);
    void ProcessStoreFloatElement(StackInstr* instr);
    void ProcessJump(StackInstr* instr);
    void ProcessFloor(StackInstr* instr);
    void ProcessCeiling(StackInstr* instr);
    void ProcessFloatToInt(StackInstr* instr);
    void ProcessIntToFloat(StackInstr* instr);
    
    // Add byte code to buffer
    inline void AddMachineCode(uint32_t i) {
      if(code_index * sizeof(uint32_t) >= (uint32_t)code_buf_max) {
        code_buf_max *= 2;
        code = (uint32_t*)realloc(code, code_buf_max);
        if(!code) {
          wcerr << L"Unable to allocate JIT memory!" << endl;
          exit(1);
        }
      }
      code[code_index++] = i;
    }
    
    // Encodes and writes out a 32-bit integer value
    inline void AddImm(int32_t imm) {
      AddMachineCode(imm);
    }
    
    // Returns the name of a register
    wstring GetRegisterName(Register reg);
        
    /**
     * Check for 'Nil' dereferencing
     */
    inline void CheckNilDereference(Register reg) {
      // less than zero
      cmp_imm_reg(0, reg);
      AddMachineCode(0x0a000000);
      deref_offsets.push_back(code_index);
      // jump to exit
    }

    /**
     * Checks array bounds
     */
    inline void CheckArrayBounds(Register reg, Register max_reg) {
      // less than zero
      cmp_imm_reg(0, reg);
      AddMachineCode(0xba000000);
      bounds_less_offsets.push_back(code_index);
      // jump to exit

      // greater-equal than max
      cmp_reg_reg(max_reg, reg);
      AddMachineCode(0xaa000000);
      bounds_greater_offsets.push_back(code_index);
      // jump to exit
    }
    
    /**
     * Gets an avaiable register from
     */
    RegisterHolder* GetRegister(bool use_aux = true) {
      RegisterHolder* holder;
      if(aval_regs.empty()) {
        if(use_aux && !aux_regs.empty()) {
          holder = aux_regs.top();
          aux_regs.pop();
        }
        else {
          compile_success = false;
#ifdef _DEBUG
          wcout << L">>> No general registers avaiable! <<<" << endl;
#endif
          aux_regs.push(new RegisterHolder(X0, false));
          holder = aux_regs.top();
          aux_regs.pop();
        }
      }
      else {
        holder = aval_regs.back();
        aval_regs.pop_back();
        used_regs.push_back(holder);
      }
#ifdef _VERBOSE
      wcout << L"\t * allocating " << GetRegisterName(holder->GetRegister())
            << L" *" << endl;
#endif

      return holder;
    }

    // Returns a register to the pool
    void ReleaseRegister(RegisterHolder* h) {
#ifdef _VERBOSE
      wcout << L"\t * releasing " << GetRegisterName(h->GetRegister())
            << L" *" << endl;
#endif

#ifdef _DEBUG
      assert(!h->IsDouble());
      for(size_t i  = 0; i < aval_regs.size(); ++i) {
        assert(h != aval_regs[i]);
      }
#endif

      if(h->GetRegister() >= X4 && h->GetRegister() <= X7) {
        aux_regs.push(h);
      }
      else {
        aval_regs.push_back(h);
        used_regs.remove(h);
      }
    }

    // Gets an avaiable register from
    // the pool of registers
    RegisterHolder* GetFpRegister() {
      RegisterHolder* holder;
      if(aval_xregs.empty()) {
        compile_success = false;
#ifdef _DEBUG
        wcout << L">>> No D registers avaiable! <<<" << endl;
#endif
        aval_xregs.push_back(new RegisterHolder(D0, true));
        holder = aval_xregs.back();
        aval_xregs.pop_back();
        used_xregs.push_back(holder);
      }
      else {
        holder = aval_xregs.back();
        aval_xregs.pop_back();
        used_xregs.push_back(holder);
      }
#ifdef _VERBOSE
      wcout << L"\t * allocating " << GetRegisterName(holder->GetRegister())
            << L" *" << endl;
#endif

      return holder;
    }

    // Returns a register to the pool
    void ReleaseFpRegister(RegisterHolder* h) {
#ifdef _DEBUG
      assert(h->IsDouble());
      for(size_t i = 0; i < aval_xregs.size(); ++i) {
        assert(h != aval_xregs[i]);
      }
#endif

#ifdef _VERBOSE
      wcout << L"\t * releasing: " << GetRegisterName(h->GetRegister())
            << L" * " << endl;
#endif
      aval_xregs.push_back(h);
      used_xregs.remove(h);
    }

    RegisterHolder* GetStackPosRegister() {
      RegisterHolder* op_stack_holder = GetRegister();
      move_mem_reg(OP_STACK, FP, op_stack_holder->GetRegister());
      return op_stack_holder;
    }

    // move instructions
    void move_reg_mem8(Register src, long offset, Register dest);
    void move_mem8_reg(long offset, Register src, Register dest);
    void move_imm_mem8(long imm, long offset, Register dest);
    void move_reg_reg(Register src, Register dest);
    void move_reg_mem(Register src, long offset, Register dest);
    void move_mem_reg(long offset, Register src, Register dest);
    void move_imm_memx(RegInstr* instr, long offset, Register dest);
    void move_imm_mem(long imm, long offset, Register dest);
    void move_imm_reg(long imm, Register reg);
    void move_imm_xreg(RegInstr* instr, Register reg);
    void move_mem_xreg(long offset, Register src, Register dest);
    void move_xreg_mem(Register src, long offset, Register dest);
    void move_xreg_xreg(Register src, Register dest);

    // math instructions
    void math_imm_reg(long imm, Register reg, InstructionType type);
    void math_reg_reg(Register src, Register dest, InstructionType type);
    void math_mem_reg(long offset, Register reg, InstructionType type);
    void math_imm_xreg(RegInstr *instr, RegisterHolder *&reg, InstructionType type);
    void math_mem_xreg(long offset, RegisterHolder *&reg, InstructionType type);
    void math_xreg_xreg(Register src, RegisterHolder *&dest, InstructionType type);
    
    // logical
    void and_imm_reg(long imm, Register reg);
    void and_reg_reg(Register src, Register dest);
    void and_mem_reg(long offset, Register src, Register dest);
    void or_imm_reg(long imm, Register reg);
    void or_reg_reg(Register src, Register dest);
    void or_mem_reg(long offset, Register src, Register dest);
    void xor_imm_reg(long imm, Register reg);
    void xor_reg_reg(Register src, Register dest);
    void xor_mem_reg(long offset, Register src, Register dest);
    
    // add instructions
    void add_imm_mem(long imm, long offset, Register dest);
    void add_imm_reg(long imm, Register reg);
    void add_imm_xreg(RegInstr* instr, Register reg);
    void add_xreg_xreg(Register src, Register dest);
    void add_mem_reg(long offset, Register src, Register dest);
    void add_mem_xreg(long offset, Register src, Register dest);
    void add_reg_reg(Register src, Register dest);

    // sub instructions
    void sub_imm_xreg(RegInstr* instr, Register reg);
    void sub_xreg_xreg(Register src, Register dest);
    void sub_mem_xreg(long offset, Register src, Register dest);
    void sub_imm_reg(long imm, Register reg);
    void sub_imm_mem(long imm, long offset, Register dest);
    void sub_reg_reg(Register src, Register dest);
    void sub_mem_reg(long offset, Register src, Register dest);

    // mul instructions
    void mul_imm_xreg(RegInstr* instr, Register reg);
    void mul_xreg_xreg(Register src, Register dest);
    void mul_mem_xreg(long offset, Register src, Register dest);
    void mul_imm_reg(long imm, Register reg);
    void mul_reg_reg(Register src, Register dest);
    void mul_mem_reg(long offset, Register src, Register dest);

    // div instructions
    void div_imm_xreg(RegInstr* instr, Register reg);
    void div_xreg_xreg(Register src, Register dest);
    void div_mem_xreg(long offset, Register src, Register dest);
    void div_imm_reg(long imm, Register reg, bool is_mod = false);
    void div_reg_reg(Register src, Register dest, bool is_mod = false);
    void div_mem_reg(long offset, Register src, Register dest, bool is_mod = false);

    // compare instructions
    void cmp_reg_reg(Register src, Register dest);
    void cmp_mem_reg(long offset, Register src, Register dest);
    void cmp_imm_reg(long imm, Register reg);
    
    void cmp_xreg_xreg(Register src, Register dest);
    void cmp_mem_xreg(long offset, Register src, Register dest);
    void cmp_imm_xreg(RegInstr* instr, Register reg);
    
    void cmov_reg(Register reg, InstructionType oper);

    // inc/dec instructions
    void dec_reg(Register dest);
    void dec_mem(long offset, Register dest);
    void inc_mem(long offset, Register dest);

    // shift instructions
    void shl_reg_reg(Register src, Register dest);
    void shl_mem_reg(long offset, Register src, Register dest);
    void shl_imm_reg(long value, Register dest);

    void shr_reg_reg(Register src, Register dest);
    void shr_mem_reg(long offset, Register src, Register dest);
    void shr_imm_reg(long value, Register dest);

    // push/pop instructions
    void push_imm(int32_t value);
    void push_reg(Register reg);
    void pop_reg(Register reg);
    void push_mem(long offset, Register src);

    // type conversion instructions
    void round_imm_xreg(RegInstr* instr, Register reg, bool is_floor);
    void round_mem_xreg(long offset, Register src, Register dest, bool is_floor);
    void round_xreg_xreg(Register src, Register dest, bool is_floor);
    void vcvt_xreg_reg(Register src, Register dest);
    void vcvt_imm_reg(RegInstr* instr, Register reg);
    void vcvt_mem_reg(long offset, Register src, Register dest);
    void vcvt_reg_xreg(Register src, Register dest);
    void vcvt_imm_xreg(RegInstr* instr, Register reg);
    void vcvt_mem_xreg(long offset, Register src, Register dest);

    // function call instruction
    void call_reg(Register reg);
    
    // generates a conditional jump
    bool cond_jmp(InstructionType type);
    void loop(long offset);
    
    static size_t PopInt(size_t* op_stack, long *stack_pos) {
      const size_t value = op_stack[--(*stack_pos)];
#ifdef _DEBUG_JIT
      wcout << L"\t[pop_i: value=" << (size_t*)value << L"(" << value << L")]" << L"; pos=" << (*stack_pos) << endl;
#endif

      return value;
    }

    static void PushInt(size_t* op_stack, long *stack_pos, size_t value) {
      op_stack[(*stack_pos)++] = value;
#ifdef _DEBUG_JIT
      wcout << L"\t[push_i: value=" << (size_t*)value << L"(" << value << L")]" << L"; pos=" << (*stack_pos) << endl;
#endif
    }

    inline static FLOAT_VALUE PopFloat(size_t* op_stack, long* stack_pos) {
      (*stack_pos)--;
      
#ifdef _DEBUG_JIT
      FLOAT_VALUE v = *((FLOAT_VALUE*)(&op_stack[(*stack_pos)]));
      wcout << L"  [pop_f: stack_pos=" << (*stack_pos) << L"; value=" << L"]; pos=" << (*stack_pos) << endl;
      return v;
#endif
      
      return *((FLOAT_VALUE*)(&op_stack[(*stack_pos)]));
    }

    inline static void PushFloat(const FLOAT_VALUE v, size_t* op_stack, long* stack_pos) {
#ifdef _DEBUG_JIT
      wcout << L"  [push_f: stack_pos=" << (*stack_pos) << L"; value=" << v
            << L"]; call_pos=" << (*stack_pos) << endl;
#endif
      *((FLOAT_VALUE*)(&op_stack[(*stack_pos)])) = v;
      (*stack_pos)++;
    }
    
    // Process call backs from ASM code
    static void JitStackCallback(const long instr_id, StackInstr* instr, const long cls_id,
                                 const long mthd_id, size_t* inst, size_t* op_stack, long *stack_pos,
                                 StackFrame** call_stack, long* call_stack_pos, const long ip);

    // Calculates array element offset.
    // Note: this code must match up
    // with the interpreter's 'ArrayIndex'
    // method. Bounds checks are not done on
    // JIT code.
    RegisterHolder* ArrayIndex(StackInstr* instr, MemoryType type);

    // Caculates the indices for
    // memory references.
    void ProcessIndices();

  public:
    static void Initialize(StackProgram* p);

    JitCompilerA64() {
    }

    ~JitCompilerA64() {
      while(!working_stack.empty()) {
        RegInstr* instr = working_stack.front();
        working_stack.pop_front();
        if(instr) {
          delete instr;
          instr = nullptr;
        }
      }

      while(!aval_regs.empty()) {
        RegisterHolder* holder = aval_regs.back();
        aval_regs.pop_back();
        if(holder) {
          delete holder;
          holder = nullptr;
        }
      }

      while(!aval_xregs.empty()) {
        RegisterHolder* holder = aval_xregs.back();
        aval_xregs.pop_back();
        if(holder) {
          delete holder;
          holder = nullptr;
        }
      }

      while(!used_regs.empty()) {
        RegisterHolder* holder = used_regs.front();
        if(holder) {
          delete holder;
          holder = nullptr;
        }
        // next
        used_regs.pop_front();
      }
      used_regs.clear();

      while(!used_xregs.empty()) {
        RegisterHolder* holder = used_xregs.front();
        if(holder) {
          delete holder;
          holder = nullptr;
        }
        // next
        used_xregs.pop_front();
      }
      used_xregs.clear();

      while(!aux_regs.empty()) {
        RegisterHolder* holder = aux_regs.top();
        if(holder) {
          delete holder;
          holder = nullptr;
        }
        aux_regs.pop();
      }
    }

    //
    // Compiles stack code into IA-32 machine code
    //
    bool Compile(StackMethod* cm);
  };

  /**
   * Prototype for jit function
   */
  typedef long (*jit_fun_ptr)(long cls_id, long mthd_id, size_t* cls_mem, size_t *inst,
                              size_t *op_stack, long *stack_pos, StackFrame **call_stack,
                              long *call_stack_pos, size_t **jit_mem, long *offset, long *ints);
  
  
  /**
   * JitExecutor class
   */
  class JitExecutor {
    static StackProgram* program;

  public:
    static void Initialize(StackProgram* p);
    
    // Executes machine code
    long Execute(StackMethod* method, size_t* inst, size_t* op_stack, long* stack_pos,
                 StackFrame** call_stack, long* call_stack_pos, StackFrame* frame);
  };
}
#endif
