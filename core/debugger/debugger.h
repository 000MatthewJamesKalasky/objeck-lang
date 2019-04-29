/***************************************************************************
* Runtime debugger
*
* Copyright (c) 2010-2019 Randy Hollines
* All rights reserved.
*
* Reistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*tree.o scanner.o parser.o test.o
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

#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include "../vm/common.h"
#include "../vm/loader.h"
#include "../vm/interpreter.h"
#include "../vm/../shared/version.h"
#include "tree.h"
#include "parser.h"
#include <iomanip>
#ifdef _WIN32
#include "windows.h"
#include <fcntl.h>
#include <io.h>
#else
#include <dirent.h>
#endif

using namespace std;

namespace Runtime {
  class StackInterpreter;

  typedef struct _UserBreak {
    int line_num;
    wstring file_name;
  } UserBreak;

  /********************************
  * Source file
  ********************************/
  class SourceFile {
    wstring file_name;
    vector<wstring> lines;
    int cur_line_num;

  public:
    SourceFile(const wstring &fn, int l) {
      file_name = fn;
      cur_line_num = l;

      const string name = UnicodeToBytes(fn);
      ifstream file_in (name.c_str());
      while(file_in.good()) {
        string line;
        getline(file_in, line);
        lines.push_back(BytesToUnicode(line));
      }
      file_in.close();
    }

    ~SourceFile() {
    }

    bool IsLoaded() {
      return lines.size() > 0;
    }

    bool Print(int start);

    const wstring& GetFileName() {
      return file_name;
    }
  };

  /********************************
  * Interactive command line
  * debugger
  ********************************/
  class Debugger {
    wstring program_file;
    wstring base_path;
    bool quit;
    // break info
    list<UserBreak*> breaks;
    int cur_line_num;
    wstring cur_file_name;
    StackFrame** cur_call_stack;
    long cur_call_stack_pos;
    bool is_error;
    bool is_next;
    bool is_next_line;
    bool is_jmp_out;
    size_t* ref_mem;
    StackClass* ref_klass;
    // interpreter variables
    vector<wstring> arguments;
    StackInterpreter* interpreter;
    StackProgram* cur_program;
    StackFrame* cur_frame;
    size_t* op_stack;
    long* stack_pos;

    // pretty prints a method
    wstring PrintMethod(StackMethod* method);

    // checks to see if a file exists
    bool FileExists(const wstring &file_name, bool is_exe = false);

    // checks to see if a directory exists
    bool DirectoryExists(const wstring &wdir_name);

    // deletes a break point
    bool DeleteBreak(int line_num, const wstring &file_name);

    // searches for a valid breakpoint based upon the line number provided
    UserBreak* FindBreak(int line_num, const wstring &file_name);

    // adds a break
    bool AddBreak(int line_num, const wstring &file_name);

    // lists all breaks
    void ListBreaks();

    // prints declarations
    void PrintDeclarations(StackDclr** dclrs, int dclrs_num);

    Command* ProcessCommand(const wstring &line);
    void ProcessRun();
    void ProcessExe(Load* load);
    void ProcessSrc(Load* load);
    void ProcessArgs(Load* load);
    void ProcessInfo(Info* info);
    void ProcessBreak(FilePostion* break_command);
    void ProcessBreaks();
    void ProcessDelete(FilePostion* break_command);
    void ProcessList(FilePostion* break_command);
    void ProcessPrint(Print* print);
    void ClearProgram();
    void ClearBreaks();

    void EvaluateExpression(Expression* expression);
    void EvaluateReference(Reference* &reference, MemoryContext context);
    void EvaluateInstanceReference(Reference* reference, int index);
    void EvaluateClassReference(Reference* reference, StackClass* klass, int index);
    void EvaluateByteReference(Reference* reference, int index);
    void EvaluateCharReference(Reference* reference, int index);
    void EvaluateIntFloatReference(Reference* reference, int index, bool is_float);
    void EvaluateCalculation(CalculatedExpression* expression);

  public:
    Debugger(const wstring &fn, const wstring &bp) {
      program_file = fn;
      base_path = bp;
      quit = false;
      // clear program
      interpreter = nullptr;
      op_stack = nullptr;
      stack_pos = nullptr;
      cur_line_num = -2;
      cur_frame = nullptr;
      cur_program = nullptr;
      cur_call_stack = nullptr;
      cur_call_stack_pos = 0;
      is_error = false;
      ref_mem = nullptr;
      ref_mem = nullptr;
      is_jmp_out = false;
    }

    ~Debugger() {
      ClearProgram();
      ClearBreaks();
    }

    // start debugger
    void Debug();

    // runtime callback
    void ProcessInstruction(StackInstr* instr, long ip, StackFrame** call_stack,
      long call_stack_pos, StackFrame* frame);
  };
}
#endif
