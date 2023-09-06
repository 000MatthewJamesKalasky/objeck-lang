/***************************************************************************
 * Starting point for the VM.
 *
 * Copyright (c) 2023, Randy Hollines
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

#include "vm.h"
#include <codecvt>

#define SUCCESS 0
#define USAGE_ERROR -1

// common execution point for all platforms
#ifdef _WIN32
int Execute(const int argc, const char* argv[], bool is_stdio_binary)
#else
int Execute(const int argc, const char* argv[])
#endif
{
  if(argc > 1) {
    wchar_t** commands = ProcessCommandLine(argc, argv);
    Loader loader(argc, commands);
    loader.Load();

    // execute
    size_t* op_stack = new size_t[OP_STACK_SIZE];
    long* stack_pos = new long;
    (*stack_pos) = 0;
    
#ifdef _TIMING
    clock_t start = clock();
#endif
    // start the interpreter...
#ifdef _WIN32
    Runtime::StackInterpreter::SetBinaryStdio(is_stdio_binary);
#endif
    Runtime::StackInterpreter* intpr = new Runtime::StackInterpreter(Loader::GetProgram());
    Runtime::StackInterpreter::AddThread(intpr);
    intpr->Execute(op_stack, stack_pos, 0, loader.GetProgram()->GetInitializationMethod(), nullptr, false);
    
#ifdef _DEBUG
    std::wcout << L"# final std::stack: pos=" << (*stack_pos) << L" #" << std::endl;
    if((*stack_pos) > 0) {
      for(int i = 0; i < (*stack_pos); ++i) {
        std::wcout << L"dump: value=" << op_stack[i] << std::endl;
      }
    }
#endif

#ifdef _SANITIZE
#ifdef _DEBUG
    std::wcout << L"# final std::stack: pos=" << (*stack_pos) << L" #" << std::endl;
    if((*stack_pos) > 0) {
      for(int i = 0; i < (*stack_pos); ++i) {
        std::wcout << L"dump: value=" << *(stack_pos + 1) << std::endl;
      }
    }
    assert(!(*stack_pos));
#endif
    

    Runtime::StackInterpreter::RemoveThread(intpr);
    Runtime::StackInterpreter::HaltAll();

    Runtime::StackInterpreter::Clear();
    MemoryManager::Clear(op_stack, *stack_pos);

    delete intpr;
    intpr = nullptr;
#endif

    // clean up
    delete[] op_stack;
    op_stack = nullptr;

    delete stack_pos;
    stack_pos = nullptr;
    
#ifdef _TIMING    
    clock_t end = clock();
    std::wcout << L"---------------------------" << std::endl;
    std::wcout << L"CPU Time: " << (double)(end - start) / CLOCKS_PER_SEC << L" second(s)." << std::endl;
#endif

    CleanUpCommandLine(argc, commands);
    
    return SUCCESS;
  } 
  else {
    return USAGE_ERROR;
  }
}

wchar_t** ProcessCommandLine(const int argc, const char* argv[])
{
  wchar_t** wide_args = new wchar_t* [argc];
  for(int i = 0; i < argc; ++i) {
    const char* arg = argv[i];
    const int len = (int)strlen(arg);
    wchar_t* wide_arg = new wchar_t[len + 1];
    for(int j = 0; j < len; ++j) {
      wide_arg[j] = arg[j];
    }
    wide_arg[len] = L'\0';
    wide_args[i] = wide_arg;
  }

  return wide_args;
}

void CleanUpCommandLine(const int argc, wchar_t** wide_args)
{
  for(int i = 0; i < argc; ++i) {
    wchar_t* wide_arg = wide_args[i];
    delete[] wide_arg;
    wide_arg = nullptr;
  }

  delete[] wide_args;
  wide_args = nullptr;
}

void SetEnv() {
#ifdef _WIN32
#ifdef _MSYS2_CLANG
  std::ios_base::sync_with_stdio(false);
  std::locale utf8(std::locale(), new std::codecvt_utf8_utf16<wchar_t>);
  std::wcout.imbue(utf8);
  std::wcin.imbue(utf8);
#else
  if (_setmode(_fileno(stdin), _O_U8TEXT) < 0) {
    std::wcerr << "Unable to initialize I/O subsystem" << std::endl;
    exit(1);
  }

  if (_setmode(_fileno(stdout), _O_U8TEXT) < 0) {
    std::wcerr << "Unable to initialize I/O subsystem" << std::endl;
    exit(1);
  }
#endif
#else
#if defined(_X64)
  char* locale = setlocale(LC_ALL, "");
  std::locale lollocale(locale);
  std::setlocale(LC_ALL, locale);
  std::wcout.imbue(lollocale);
#elif defined(_ARM64)
  char* locale = setlocale(LC_ALL, "");
  std::locale lollocale(locale);
  std::setlocale(LC_ALL, locale);
  std::wcout.imbue(lollocale);
  std::setlocale(LC_ALL, "en_US.utf8");
#else    
  setlocale(LC_ALL, "en_US.utf8");
#endif
#endif
}