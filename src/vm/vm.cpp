/***************************************************************************
 * Starting point for the VM.
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

// #include "vld.h"
#include "vm.h"

#define SUCCESS 0
#define USAGE_ERROR -1

int Execute(const int argc, const char* argv[])
{
  if(argc > 1) {
    // loader; when this goes out of scope program memory is released
    srand((unsigned int)time(NULL)); rand(); // calling rand() once improves random number generation
    Loader loader(argc, argv);
    loader.Load();

    // execute
    long* op_stack = new long[STACK_SIZE];
    long* stack_pos = new long;
    (*stack_pos) = 0;

#ifdef _TIMING
    clock_t start = clock();
#endif
    Runtime::StackInterpreter intpr(Loader::GetProgram());
    intpr.Execute(op_stack, stack_pos, 0, loader.GetProgram()->GetInitializationMethod(), NULL, false);
#ifdef _TIMING
    clock_t end = clock();
    cout << "# final stack: pos=" << (*stack_pos) << " #" << endl;
    if((*stack_pos) > 0) {
      for(int i = 0; i < (*stack_pos); i++) {
	cout << "dump: value=" << (void*)(*stack_pos) << endl;
      } 
    }
    cout << "---------------------------" << endl;
    cout << "CPU Time: " << (double)(end - start) / CLOCKS_PER_SEC
         << " second(s)." << endl;
#endif

#ifdef _DEBUG
    cout << "# final stack: pos=" << (*stack_pos) << " #" << endl;
    if((*stack_pos) > 0) {
      for(int i = 0; i < (*stack_pos); i++) {
	cout << "dump: value=" << (void*)(*stack_pos) << endl;
      } 
    }
#endif
    
    // wait for outstanding threads
#ifdef _WIN32        
    list<HANDLE> thread_ids = loader.GetProgram()->GetThreads();
    for(list<HANDLE>::iterator iter = thread_ids.begin();
      iter != thread_ids.end(); iter++) {
      HANDLE id = (*iter);
      if(WaitForSingleObject(id, INFINITE) != WAIT_OBJECT_0) {
        cerr << "Unable to join garbage collection threads!" << endl;
        exit(-1);
      }
      CloseHandle(id);
    }
#else
    void* status;
    list<pthread_t> thread_ids = loader.GetProgram()->GetThreads();
    for(list<pthread_t>::iterator iter = thread_ids.begin();
	iter != thread_ids.end(); iter++) {
      if(pthread_join((*iter), &status)) {
	cerr << "Unable to join program thread!" << endl;
	exit(-1);
      }
    }
#endif

    // clean up
    delete[] op_stack;
    op_stack = NULL;

    delete stack_pos;
    stack_pos = NULL;

    MemoryManager::Instance()->Clear(); 

    return SUCCESS;
  } 
  else {
    return USAGE_ERROR;
  }
}
