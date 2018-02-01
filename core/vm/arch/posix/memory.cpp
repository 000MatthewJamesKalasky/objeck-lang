/***************************************************************************
 * VM memory manager. Implements a "mark and sweep" collection algorithm.
 *
 * Copyright (c) 2008-2015, Randy Hollines
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

#include "memory.h"

bool MemoryManager::initialized;
StackProgram* MemoryManager::prgm;
unordered_map<StackFrameMonitor*, StackFrameMonitor*> MemoryManager::pda_monitors;
set<StackFrame**> MemoryManager::pda_frames;
vector<StackFrame*> MemoryManager::jit_frames;
stack<char*> MemoryManager::cache_pool_16;
stack<char*> MemoryManager::cache_pool_32;
stack<char*> MemoryManager::cache_pool_64;
stack<char*> MemoryManager::cache_pool_256;
stack<char*> MemoryManager::cache_pool_512;
vector<size_t*> MemoryManager::allocated_memory;
long MemoryManager::allocation_size;
long MemoryManager::mem_max_size;
long MemoryManager::uncollected_count;
long MemoryManager::collected_count;
#ifndef _GC_SERIAL
pthread_mutex_t MemoryManager::pda_monitor_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MemoryManager::pda_frame_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MemoryManager::jit_frame_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MemoryManager::allocated_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MemoryManager::marked_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MemoryManager::marked_sweep_mutex = PTHREAD_MUTEX_INITIALIZER;

#endif

void MemoryManager::Initialize(StackProgram* p)
{
  prgm = p;
  allocation_size = 0;
  mem_max_size = MEM_MAX;
  uncollected_count = 0;
  
  for(int i = 0; i < POOL_SIZE; i++) {
    cache_pool_16.push((char*)calloc(16, sizeof(char)));
  }

  for(int i = 0; i < POOL_SIZE; i++) {
    cache_pool_32.push((char*)calloc(32, sizeof(char)));
  }
  
  for(int i = 0; i < POOL_SIZE; i++) {
    cache_pool_64.push((char*)calloc(64, sizeof(char)));
  }

  for(int i = 0; i < POOL_SIZE; i++) {
    cache_pool_256.push((char*)calloc(256, sizeof(char)));
  }

  for(int i = 0; i < POOL_SIZE; i++) {
    cache_pool_512.push((char*)calloc(512, sizeof(char)));
  }

  initialized = true;
}

// if return true, trace memory otherwise do not
inline bool MemoryManager::MarkMemory(size_t* mem)
{
  if(mem) {
    // check if memory has been marked
    if(mem[MARKED_FLAG]) {
      return false;
    }
    
    // mark & add to list
#ifndef _GC_SERIAL
    pthread_mutex_lock(&marked_mutex);
#endif
    mem[MARKED_FLAG] = 1L;
#ifndef _GC_SERIAL
    pthread_mutex_unlock(&marked_mutex);      
#endif

    return true;
  }

  return false;
}

// if return true, trace memory otherwise do not
inline bool MemoryManager::MarkValidMemory(size_t* mem)
{
  if(mem) {
#ifndef _GC_SERIAL
    pthread_mutex_lock(&allocated_mutex);
#endif
    if(std::binary_search(allocated_memory.begin(), allocated_memory.end(), mem)) {
      // check if memory has been marked
      if(mem[MARKED_FLAG]) {
#ifndef _GC_SERIAL
        pthread_mutex_unlock(&allocated_mutex);
#endif
        return false;
      }

      // mark & add to list
#ifndef _GC_SERIAL
      pthread_mutex_lock(&marked_mutex);
#endif
      mem[MARKED_FLAG] = 1L;
#ifndef _GC_SERIAL
      pthread_mutex_unlock(&marked_mutex);      
      pthread_mutex_unlock(&allocated_mutex);
#endif
      return true;
    } 
    else {
#ifndef _GC_SERIAL
      pthread_mutex_unlock(&allocated_mutex);
#endif
      return false;
    }
  }
  
  return false;
}

void MemoryManager::AddPdaMethodRoot(StackFrame** frame)
{
  if(!initialized) {
    return;
  }

#ifdef _DEBUG
//  wcout << L"adding PDA frame: addr=" << frame << endl;
#endif

#ifndef _GC_SERIAL
  pthread_mutex_lock(&pda_frame_mutex);
#endif
  pda_frames.insert(frame);
#ifndef _GC_SERIAL
  pthread_mutex_unlock(&pda_frame_mutex);
#endif
}

void MemoryManager::RemovePdaMethodRoot(StackFrame** frame)
{
#ifdef _DEBUG
//  wcout << L"removing PDA frame: addr=" << frame << endl;
#endif
  
#ifndef _GC_SERIAL
  pthread_mutex_lock(&pda_frame_mutex);
#endif
  pda_frames.erase(frame);
#ifndef _GC_SERIAL
  pthread_mutex_unlock(&pda_frame_mutex);
#endif
}

void MemoryManager::AddPdaMethodRoot(StackFrameMonitor* monitor)
{
  if(!initialized) {
    return;
  }

#ifdef _DEBUG
//  wcout << L"adding PDA monitor: addr=" << monitor << endl;
#endif

#ifndef _GC_SERIAL
  pthread_mutex_lock(&pda_monitor_mutex);
#endif
  pda_monitors.insert(pair<StackFrameMonitor*, StackFrameMonitor*>(monitor, monitor));
#ifndef _GC_SERIAL
  pthread_mutex_unlock(&pda_monitor_mutex);
#endif
}

void MemoryManager::RemovePdaMethodRoot(StackFrameMonitor* monitor)
{
#ifdef _DEBUG
//  wcout << L"removing PDA monitor: addr=" << monitor << endl;
#endif

#ifndef _GC_SERIAL
  pthread_mutex_lock(&pda_monitor_mutex);
#endif
  pda_monitors.erase(monitor);
#ifndef _GC_SERIAL
  pthread_mutex_unlock(&pda_monitor_mutex);
#endif
}

size_t* MemoryManager::AllocateObject(const long obj_id, size_t* op_stack, 
                                    long stack_pos, bool collect)
{
  StackClass* cls = prgm->GetClass(obj_id);
#ifdef _DEBUG
  assert(cls);
#endif

  size_t* mem = NULL;
  if(cls) {
    long size = cls->GetInstanceMemorySize();
#ifdef _X64
    // TODO: memory size is doubled the compiler assumes that integers are 4-bytes.
    // In 64-bit mode integers and floats are 8-bytes.  This approach allocates more
    // memory for floats (a.k.a doubles) than needed.
    size *= 2;
#endif

    // collect memory
    if(collect && allocation_size + size > mem_max_size) {
      CollectMemory(op_stack, stack_pos);
    }

#ifdef _DEBUG
    bool is_cached = false;
#endif
    
    // allocate memory
    const long alloc_size = size * 2 + sizeof(long) * EXTRA_BUF_SIZE;

    if(cache_pool_512.size() > 0 && alloc_size <= 512 && alloc_size > 256) {
      mem = (size_t*)cache_pool_512.top();
      cache_pool_512.pop();
      mem[EXTRA_BUF_SIZE + CACHE_SIZE] = 512;
#ifdef _DEBUG
      is_cached = true;
#endif
    }
    else if(cache_pool_256.size() > 0 && alloc_size <= 256 && alloc_size > 64) {
      mem = (size_t*)cache_pool_256.top();
      cache_pool_256.pop();
      mem[EXTRA_BUF_SIZE + CACHE_SIZE] = 256;
#ifdef _DEBUG
      is_cached = true;
#endif
    }
    else if(cache_pool_64.size() > 0 && alloc_size <= 64 && alloc_size > 32) {
      mem = (size_t*)cache_pool_64.top();
      cache_pool_64.pop();
      mem[EXTRA_BUF_SIZE + CACHE_SIZE] = 64;
#ifdef _DEBUG
      is_cached = true;
#endif
    }
    else if(cache_pool_32.size() > 0 && alloc_size <= 32 && alloc_size > 16) {
      mem = (size_t*)cache_pool_32.top();
      cache_pool_32.pop();
      mem[EXTRA_BUF_SIZE + CACHE_SIZE] = 32;
#ifdef _DEBUG
      is_cached = true;
#endif
    }
    else if(cache_pool_16.size() > 0 && alloc_size <= 16) {
      mem = (size_t*)cache_pool_16.top();
      cache_pool_16.pop();
      mem[EXTRA_BUF_SIZE + CACHE_SIZE] = 16;
#ifdef _DEBUG
      is_cached = true;
#endif
    } 
    else {
      mem = (size_t*)calloc(alloc_size, sizeof(char));
      mem[EXTRA_BUF_SIZE + CACHE_SIZE] = -1;
    }
    mem[EXTRA_BUF_SIZE + TYPE] = NIL_TYPE;
    mem[EXTRA_BUF_SIZE + SIZE_OR_CLS] = (long)cls;
    mem += EXTRA_BUF_SIZE;
    
    // record
#ifndef _GC_SERIAL
    pthread_mutex_lock(&allocated_mutex);
#endif
    allocation_size += size;
    allocated_memory.push_back(mem);
#ifndef _GC_SERIAL
    pthread_mutex_unlock(&allocated_mutex);
#endif
   
/* 
#ifdef _DEBUG
    wcout << L"# allocating object: cached=" << (is_cached ? "true" : "false")  
          << ", addr=" << mem << L"(" << (long)mem << L"), size="
          << size << L" byte(s), used=" << allocation_size << L" byte(s) #" << endl;
#endif
*/
  }

  return mem;
}

size_t* MemoryManager::AllocateArray(const long size, const MemoryType type,
                                   size_t* op_stack, long stack_pos, bool collect)
{
  long calc_size;
  size_t* mem;
  switch(type) {
  case BYTE_ARY_TYPE:
    calc_size = size * sizeof(char);
    break;
    
  case CHAR_ARY_TYPE:
    calc_size = size * sizeof(wchar_t);
    break;

  case INT_TYPE:
    calc_size = size * sizeof(long);
    break;

  case FLOAT_TYPE:
    calc_size = size * sizeof(FLOAT_VALUE);
    break;

  default:
    wcerr << L"internal error" << endl;
    exit(1);
    break;
  }
  // collect memory
  if(collect && allocation_size + calc_size > mem_max_size) {
    CollectMemory(op_stack, stack_pos);
  }
  
#ifdef _DEBUG
  bool is_cached = false;
#endif
  
  // allocate memory
  const long alloc_size = calc_size + sizeof(long) * EXTRA_BUF_SIZE;
  if(cache_pool_512.size() > 0 && alloc_size <= 512 && alloc_size > 256) {
    mem = (size_t*)cache_pool_512.top();
    cache_pool_512.pop();
    mem[EXTRA_BUF_SIZE + CACHE_SIZE] = 512;
#ifdef _DEBUG
    is_cached = true;
#endif
  }
  else if(cache_pool_256.size() > 0 && alloc_size <= 256 && alloc_size > 64) {
    mem = (size_t*)cache_pool_256.top();
    cache_pool_256.pop();
    mem[EXTRA_BUF_SIZE + CACHE_SIZE] = 256;
#ifdef _DEBUG
    is_cached = true;
#endif
  }
  else   if(cache_pool_64.size() > 0 && alloc_size <= 64 && alloc_size > 32) {
    mem = (size_t*)cache_pool_64.top();
    cache_pool_64.pop();
    mem[EXTRA_BUF_SIZE + CACHE_SIZE] = 64;
#ifdef _DEBUG
    is_cached = true;
#endif
  }
  else if(cache_pool_32.size() > 0 && alloc_size <= 32 && alloc_size > 16) {
    mem = (size_t*)cache_pool_32.top();
    cache_pool_32.pop();
    mem[EXTRA_BUF_SIZE + CACHE_SIZE] = 32;
#ifdef _DEBUG
    is_cached = true;
#endif
  }
  else if(cache_pool_16.size() > 0 && alloc_size <= 16) {
    mem = (size_t*)cache_pool_16.top();
    cache_pool_16.pop();
    mem[EXTRA_BUF_SIZE + CACHE_SIZE] = 16;
#ifdef _DEBUG
    is_cached = true;
#endif
  } 
  else {    
    mem = (size_t*)calloc(alloc_size, sizeof(char));
    mem[EXTRA_BUF_SIZE + CACHE_SIZE] = -1;
  }
  mem[EXTRA_BUF_SIZE + TYPE] = type;
  mem[EXTRA_BUF_SIZE + SIZE_OR_CLS] = calc_size;
  mem += EXTRA_BUF_SIZE;
  
#ifndef _GC_SERIAL
  pthread_mutex_lock(&allocated_mutex);
#endif
  allocation_size += calc_size;
  allocated_memory.push_back(mem);
#ifndef _GC_SERIAL
  pthread_mutex_unlock(&allocated_mutex);
#endif
 
/* 
#ifdef _DEBUG
  wcout << L"# allocating array: cached=" << (is_cached ? "true" : "false") 
        << ", addr=" << mem << L"(" << (long)mem << L"), size=" << calc_size
        << L" byte(s), used=" << allocation_size << L" byte(s) #" << endl;
#endif
*/

  return mem;
}

size_t* MemoryManager::ValidObjectCast(size_t* mem, long to_id, int* cls_hierarchy, int** cls_interfaces)
{
  // invalid array cast  
  long id = GetObjectID(mem);
  if(id < 0) {
    return NULL;
  }
  
  // upcast
  int cls_id = id;
  while(cls_id != -1) {
    if(cls_id == to_id) {
      return mem;
    }
    // update
    cls_id = cls_hierarchy[cls_id];
  }
  
  // check interfaces
  cls_id = id;
  while(cls_id != -1) {
    int* interfaces = cls_interfaces[cls_id];
    if(interfaces) {
      int i = 0;
      int inf_id = interfaces[i];
      while(inf_id > -1) {
        if(inf_id == to_id) {
          return mem;
        }
        inf_id = interfaces[++i];
      }
    }
    // update
    cls_id = cls_hierarchy[cls_id];
  }
  
  return NULL;
}

void MemoryManager::CollectMemory(size_t* op_stack, long stack_pos)
{
#ifndef _GC_SERIAL
  // only one thread at a time can invoke the gargabe collector
  if(pthread_mutex_trylock(&marked_sweep_mutex)) {
    return;
  }
#endif
  
  CollectionInfo* info = new CollectionInfo;
  info->op_stack = op_stack;
  info->stack_pos = stack_pos;
  
#ifndef _GC_SERIAL
  pthread_attr_t attrs;
  pthread_attr_init(&attrs);
  pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);
  
  pthread_t collect_thread;
  if(pthread_create(&collect_thread, &attrs, CollectMemory, (void*)info)) {
    cerr << L"Unable to create garbage collection thread!" << endl;
    exit(-1);
  }
#else
  CollectMemory(info);
#endif
  
#ifndef _GC_SERIAL
  // wait until collection is complete before perceeding
  void* status;
  if(pthread_join(collect_thread, &status)) {
    cerr << L"Unable to join garbage collection threads!" << endl;
    exit(-1);
  }
  pthread_attr_destroy(&attrs);
  pthread_mutex_unlock(&marked_sweep_mutex);
#endif
}

void* MemoryManager::CollectMemory(void* arg)
{
#ifdef _TIMING
  clock_t start = clock();
#endif
  
  CollectionInfo* info = (CollectionInfo*)arg;
  
#ifndef _GC_SERIAL
  pthread_mutex_lock(&allocated_mutex);
#endif
  std::sort(allocated_memory.begin(), allocated_memory.end());
#ifndef _GC_SERIAL
  pthread_mutex_unlock(&allocated_mutex);
#endif  
  
#ifdef _DEBUG
  long start = allocation_size;
  wcout << dec << endl << L"=========================================" << endl;
  wcout << L"Starting Garbage Collection; thread=" << pthread_self() << endl;
  wcout << L"=========================================" << endl;
  wcout << L"## Marking memory ##" << endl;
#endif

#ifndef _GC_SERIAL
  // multi-threaded mark
  pthread_attr_t attrs;
  pthread_attr_init(&attrs);
  pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);
  
  pthread_t static_thread;
  if(pthread_create(&static_thread, &attrs, CheckStatic, (void*)info)) {
    cerr << L"Unable to create garbage collection thread!" << endl;
    exit(-1);
  }

  pthread_t stack_thread;
  if(pthread_create(&stack_thread, &attrs, CheckStack, (void*)info)) {
    cerr << L"Unable to create garbage collection thread!" << endl;
    exit(-1);
  }

  pthread_t pda_thread;
  if(pthread_create(&pda_thread, &attrs, CheckPdaRoots, NULL)) {
    cerr << L"Unable to create garbage collection thread!" << endl;
    exit(-1);
  }
  
  pthread_attr_destroy(&attrs);
  
  // join all of the mark threads
  void *status;

  if(pthread_join(static_thread, &status)) {
    cerr << L"Unable to join garbage collection threads!" << endl;
    exit(-1);
  }
  
  if(pthread_join(stack_thread, &status)) {
    cerr << L"Unable to join garbage collection threads!" << endl;
    exit(-1);
  }

  if(pthread_join(pda_thread, &status)) {
    cerr << L"Unable to join garbage collection threads!" << endl;
    exit(-1);
  }
  
#else
  CheckStatic(NULL);
  CheckStack(info);
  CheckPdaRoots(NULL);
#endif

#ifdef _TIMING
  clock_t end = clock();
  wcout << dec << endl << L"=========================================" << endl;
  wcout << L"Mark time: " << (double)(end - start) / CLOCKS_PER_SEC 
        << L" second(s)." << endl;
  wcout << L"=========================================" << endl;
  start = clock();
#endif
  
  // sweep memory
#ifdef _DEBUG
  wcout << L"## Sweeping memory ##" << endl;
#endif
  
  // sort and search
#ifndef _GC_SERIAL
  pthread_mutex_lock(&marked_mutex);
#endif
  
#ifdef _DEBUG
  wcout << L"-----------------------------------------" << endl;
  wcout << L"Sweeping..." << endl;
  wcout << L"-----------------------------------------" << endl;
#endif
  
#ifndef _GC_SERIAL
  pthread_mutex_lock(&allocated_mutex);
#endif
  vector<size_t*> live_memory;
  live_memory.reserve(allocated_memory.size());
  for(size_t i = 0; i < allocated_memory.size(); ++i) {
    size_t* mem = allocated_memory[i];
    
    // check dynamic memory
    bool found = false;
    if(mem[MARKED_FLAG]) {
      mem[MARKED_FLAG] = 0L;
      found = true;
    }
    
    // live
    if(found) {
      live_memory.push_back(mem);
    }
    // will be collected
    else {
      // object or array
      long mem_size;
      if(mem[TYPE] == NIL_TYPE) {
        StackClass* cls = (StackClass*)mem[SIZE_OR_CLS];
#ifdef _DEBUG
        assert(cls);
#endif
        if(cls) {
          mem_size = cls->GetInstanceMemorySize();
        }
        else {
          mem_size = mem[SIZE_OR_CLS];
        }
      } 
      // array
      else {
        mem_size = mem[SIZE_OR_CLS];
      }
            
      // account for deallocated memory
      allocation_size -= mem_size;
      
      // cache or free memory
      size_t* tmp = mem - EXTRA_BUF_SIZE;
      switch(mem[CACHE_SIZE]) {
      case 512:	  
        if(cache_pool_512.size() < POOL_SIZE + 1) {
          memset(tmp, 0, 512);
          cache_pool_512.push((char*)tmp);
#ifdef _DEBUG
          wcout << L"# caching memory: addr=" << mem << L"(" << (long)mem
                << L"), size=" << mem_size << L" byte(s) #" << endl;
#endif
        }
        break;
	
      case 256:
        if(cache_pool_256.size() < POOL_SIZE + 1) {
          memset(tmp, 0, 256);
          cache_pool_256.push((char*)tmp);
#ifdef _DEBUG
          wcout << L"# caching memory: addr=" << mem << L"(" << (long)mem
                << L"), size=" << mem_size << L" byte(s) #" << endl;
#endif
        }
        break;

      case 64:
        if(cache_pool_64.size() < POOL_SIZE + 1) {
          memset(tmp, 0, 64);
          cache_pool_64.push((char*)tmp);
#ifdef _DEBUG
          wcout << L"# caching memory: addr=" << mem << L"(" << (long)mem
                << L"), size=" << mem_size << L" byte(s) #" << endl;
#endif
        }
        break;

      case 32:
        if(cache_pool_32.size() < POOL_SIZE + 1) {
          memset(tmp, 0, 32);
          cache_pool_32.push((char*)tmp);
#ifdef _DEBUG
          wcout << L"# caching memory: addr=" << mem << L"(" << (long)mem
                << L"), size=" << mem_size << L" byte(s) #" << endl;
#endif
        }
        break;

      case 16:
        if(cache_pool_16.size() < POOL_SIZE + 1) {
          memset(tmp, 0, 16);
          cache_pool_16.push((char*)tmp);
#ifdef _DEBUG
          wcout << L"# caching memory: addr=" << mem << L"(" << (long)mem
                << L"), size=" << mem_size << L" byte(s) #" << endl;
#endif
        }
        break;

      default:
        free(tmp);
        tmp = NULL;
#ifdef _DEBUG
        wcout << L"# freeing memory: addr=" << mem << L"(" << (long)mem
              << L"), size=" << mem_size << L" byte(s) #" << endl;
#endif
        break;
      }
    }
  }
  
#ifndef _GC_SERIAL
  pthread_mutex_unlock(&marked_mutex);
#endif  
  
  // did not collect memory; ajust constraints
  if(live_memory.size() == allocated_memory.size()) {
    if(uncollected_count < UNCOLLECTED_COUNT) {
      uncollected_count++;
    } 
    else {
      mem_max_size <<= 4;
      uncollected_count = 0;
    }
  }
  // collected memory; ajust constraints
  else if(mem_max_size != MEM_MAX) {
    if(collected_count < COLLECTED_COUNT) {
      collected_count++;
    } 
    else {
      mem_max_size >>= 1;
      collected_count = 0;
    }
  }
  
  // copy live memory to allocated memory
  allocated_memory = live_memory;
  
#ifndef _GC_SERIAL
  pthread_mutex_unlock(&allocated_mutex);
#endif
  
#ifdef _DEBUG
  wcout << L"===============================================================" << endl;
  wcout << L"Finished Collection: collected=" << (start - allocation_size)
        << L" of " << start << L" byte(s) - " << showpoint << setprecision(3)
        << (((double)(start - allocation_size) / (double)start) * 100.0)
        << L"%" << endl;
  wcout << L"===============================================================" << endl;
#endif

#ifdef _TIMING
  end = clock();
  wcout << dec << endl << L"=========================================" << endl;
  wcout << L"Sweep time: " << (double)(end - start) / CLOCKS_PER_SEC 
        << L" second(s)." << endl;
  wcout << L"=========================================" << endl;
#endif

#ifndef _GC_SERIAL
  pthread_exit(NULL);
#endif
}

void* MemoryManager::CheckStatic(void* arg)
{
  StackClass** clss = prgm->GetClasses();
  int cls_num = prgm->GetClassNumber();
  
  for(int i = 0; i < cls_num; i++) {
    StackClass* cls = clss[i];
    CheckMemory(cls->GetClassMemory(), cls->GetClassDeclarations(), 
                cls->GetNumberClassDeclarations(), 0);
  }
  
  return NULL;
}

void* MemoryManager::CheckStack(void* arg)
{
  CollectionInfo* info = (CollectionInfo*)arg;
#ifdef _DEBUG
  wcout << L"----- Marking Stack: stack: pos=" << info->stack_pos 
        << L"; thread=" << pthread_self() << L" -----" << endl;
#endif
  while(info->stack_pos > -1) {
    CheckObject((size_t*)info->op_stack[info->stack_pos--], false, 1);
  }
  delete info;
  info = NULL;
  
#ifndef _GC_SERIAL
  pthread_exit(NULL);
#endif
}

void* MemoryManager::CheckJitRoots(void* arg)
{
#ifndef _GC_SERIAL
  pthread_mutex_lock(&jit_frame_mutex);
#endif  
  
#ifdef _DEBUG
  wcout << L"---- Marking JIT method root(s): num=" << jit_frames.size() 
        << L"; thread=" << pthread_self() << L" ------" << endl;
  wcout << L"memory types: " << endl;
#endif
  
  for(size_t i = 0; i < jit_frames.size(); ++i) {
    StackFrame* frame = jit_frames[i];    
    size_t* mem = frame->jit_mem;
    size_t* self = (size_t*)frame->mem[0];
    StackMethod* mthd = frame->method;
    const long dclrs_num = mthd->GetNumberDeclarations();

#ifdef _DEBUG
    wcout << L"\t===== JIT method: name=" << mthd->GetName() << L", id=" << mthd->GetClass()->GetId()
	  << L"," << mthd->GetId() << L"; addr=" << mthd << L"; mem=" << mem << L"; self=" << self 
          << L"; num=" << mthd->GetNumberDeclarations() << L" =====" << endl;
#endif
    
    // check self
    CheckObject(self, true, 1);

    StackDclr** dclrs = mthd->GetDeclarations();
    for(int j = dclrs_num - 1; j > -1; j--) {            
      // update address based upon type
      switch(dclrs[j]->type) {
      case FUNC_PARM:
#ifdef _DEBUG
        wcout << L"\t" << j << L": FUNC_PARM: value=" << (*mem) 
              << L"," << *(mem + 1) << endl;
#endif
        // update
        mem += 2;
        break;
        
      case CHAR_PARM:
      case INT_PARM:
#ifdef _DEBUG
        wcout << L"\t" << j << L": CHAR_PARM/INT_PARM: value=" << (*mem) << endl;
#endif
        // update
        mem++;
        break;

      case FLOAT_PARM: {
#ifdef _DEBUG
        FLOAT_VALUE value;
        memcpy(&value, mem, sizeof(FLOAT_VALUE));
        wcout << L"\t" << j << L": FLOAT_PARM: value=" << value << endl;
#endif
        // update
#ifdef _X64
        // mapped such that all 64-bit values the same size
        mem++;
#else
        mem += 2;
#endif
      }
        break;

      case BYTE_ARY_PARM:
#ifdef _DEBUG
        wcout << L"\t" << j << L": BYTE_ARY_PARM: addr=" 
              << (long*)(*mem) << L"(" << (long)(*mem) 
              << L"), size=" << ((*mem) ? ((long*)(*mem))[SIZE_OR_CLS] : 0)
              << L" byte(s)" << endl;
#endif
        // mark data
        MarkMemory((size_t*)(*mem));
        // update
        mem++;
        break;

      case CHAR_ARY_PARM:
#ifdef _DEBUG
        wcout << L"\t" << j << L": CHAR_ARY_PARM: addr=" << (long*)(*mem) << L"(" << (long)(*mem) 
              << L"), size=" << ((*mem) ? ((long*)(*mem))[SIZE_OR_CLS] : 0)
              << L" byte(s)" << endl;
#endif
        // mark data
        MarkMemory((size_t*)(*mem));
        // update
        mem++;
        break;

      case INT_ARY_PARM:
#ifdef _DEBUG
        wcout << L"\t" << j << L": INT_ARY_PARM: addr=" << (long*)(*mem)
              << L"(" << (long)(*mem) << L"), size=" 
              << ((*mem) ? ((long*)(*mem))[SIZE_OR_CLS] : 0) 
              << L" byte(s)" << endl;
#endif
        // mark data
        MarkMemory((size_t*)(*mem));
        // update
        mem++;
        break;

      case FLOAT_ARY_PARM:
#ifdef _DEBUG
        wcout << L"\t" << j << L": FLOAT_ARY_PARM: addr=" << (long*)(*mem)
              << L"(" << (long)(*mem) << L"), size=" << L" byte(s)" 
              << ((*mem) ? ((long*)(*mem))[SIZE_OR_CLS] : 0) << endl;
#endif
        // mark data
        MarkMemory((size_t*)(*mem));
        // update
        mem++;
        break;

      case OBJ_PARM: {
#ifdef _DEBUG
        wcout << L"\t" << j << L": OBJ_PARM: addr=" << (size_t*)(*mem)
              << L"(" << (long)(*mem) << L"), id=";
        if(*mem) {
          StackClass* tmp = (StackClass*)((size_t*)(*mem))[SIZE_OR_CLS];
          wcout << L"'" << tmp->GetName() << L"'" << endl;
        }
        else {
          wcout << L"Unknown" << endl;
        }
#endif
        // check object
        CheckObject((size_t*)(*mem), true, 1);
        // update
        mem++;
      }
        break;

        // TODO: test the code below
      case OBJ_ARY_PARM:
#ifdef _DEBUG
        wcout << L"\t" << j << L": OBJ_ARY_PARM: addr=" << (size_t*)(*mem) << L"("
              << (long)(*mem) << L"), size=" << ((*mem) ? ((size_t*)(*mem))[SIZE_OR_CLS] : 0) 
              << L" byte(s)" << endl;
#endif
        // mark data
        if(MarkValidMemory((size_t*)(*mem))) {
          size_t* array = (size_t*)(*mem);
          const long size = array[0];
          const long dim = array[1];
          size_t* objects = (size_t*)(array + 2 + dim);
          for(long k = 0; k < size; k++) {
            CheckObject((size_t*)objects[k], true, 2);
          }
        }
        // update
        mem++;
        break;

      default:
        break;
      }
    }

    // NOTE: this marks temporary variables that are stored in JIT memory
    // during some method calls. there are 3 integer temp addresses
    for(int i = 0; i < 8; i++) {
      CheckObject((size_t*)mem[i], false, 1);
    }
  }

  jit_frames.empty();
  
#ifndef _GC_SERIAL
  pthread_mutex_unlock(&jit_frame_mutex);  
  pthread_exit(NULL);
#endif
}

void* MemoryManager::CheckPdaRoots(void* arg)
{
  vector<StackFrame*> frames;
  
#ifndef _GC_SERIAL
  pthread_mutex_lock(&pda_frame_mutex);
#endif
  
#ifdef _DEBUG
  wcout << L"----- PDA frames(s): num=" << pda_frames.size() 
        << L"; thread=" << pthread_self() << L" -----" << endl;
  wcout << L"memory types:" <<  endl;
#endif
  
  // separate frames
  set<StackFrame**, StackFrame**>::iterator iter;
  for(iter = pda_frames.begin(); iter != pda_frames.end(); ++iter) {
    StackFrame** frame = *iter;
    if(*frame) {
      if((*frame)->jit_mem) {
#ifndef _GC_SERIAL
	pthread_mutex_lock(&jit_frame_mutex);
#endif
	jit_frames.push_back(*frame);
#ifndef _GC_SERIAL
	pthread_mutex_unlock(&jit_frame_mutex);
#endif
      }
      else {
	frames.push_back(*frame);
      }
    }
  }
#ifndef _GC_SERIAL
  pthread_mutex_unlock(&pda_frame_mutex);
#endif  
  
#ifndef _GC_SERIAL
  pthread_mutex_lock(&pda_monitor_mutex);
#endif

#ifdef _DEBUG
  wcout << L"----- PDA monitor(s): num=" << pda_monitors.size() 
        << L"; thread=" << pthread_self()<< L" -----" << endl;
  wcout << L"memory types:" <<  endl;
#endif
  
  // separate monitor frames
  unordered_map<StackFrameMonitor*, StackFrameMonitor*>::iterator pda_iter;
  for(pda_iter = pda_monitors.begin(); pda_iter != pda_monitors.end(); ++pda_iter) {
    StackFrameMonitor* monitor = pda_iter->first;
    long call_stack_pos = *(monitor->call_stack_pos);
    
    if(call_stack_pos > 0) {
      StackFrame** call_stack = monitor->call_stack;
      StackFrame* cur_frame = *(monitor->cur_frame);
      
      if(cur_frame->jit_mem) {
#ifndef _GC_SERIAL
	pthread_mutex_lock(&jit_frame_mutex);
#endif
	jit_frames.push_back(cur_frame);
#ifndef _GC_SERIAL
	pthread_mutex_unlock(&jit_frame_mutex);
#endif
      }
      else {
	frames.push_back(cur_frame);
      }
      
      while(--call_stack_pos > -1) {
	StackFrame* frame = call_stack[call_stack_pos];
	if(frame->jit_mem) {
#ifndef _GC_SERIAL
	pthread_mutex_lock(&jit_frame_mutex);
#endif	  
	  jit_frames.push_back(frame);
#ifndef _GC_SERIAL
	  pthread_mutex_unlock(&jit_frame_mutex);
#endif
	}
	else {
	  frames.push_back(frame);
	}
      }
    }
  }
#ifndef _GC_SERIAL
  pthread_mutex_unlock(&pda_monitor_mutex);
  pthread_exit(NULL);
#endif
  
  // check JIT roots in separate thread
  pthread_attr_t attrs;
  pthread_attr_init(&attrs);
  pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_JOINABLE);
  
  pthread_t jit_thread;
  if(pthread_create(&jit_thread, &attrs, CheckJitRoots, NULL)) {
    cerr << L"Unable to create garbage collection thread!" << endl;
    exit(-1);
  }
  
  // check PDA roots
  for(size_t i = 0; i < frames.size(); ++i) {
    StackFrame* frame = frames[i];
    StackMethod* mthd = frame->method;      
    size_t* mem = frame->mem;
    
#ifdef _DEBUG
    wcout << L"\t===== PDA method: name=" << mthd->GetName() << L", addr="
	  << mthd << L", num=" << mthd->GetNumberDeclarations() << L" =====" << endl;
#endif
    
    // mark self
    CheckObject((size_t*)(*mem), true, 1);
    
    if(mthd->HasAndOr()) {
      mem += 2;
    } 
    else {
      mem++;
    }
      
    // mark rest of memory
    CheckMemory(mem, mthd->GetDeclarations(), mthd->GetNumberDeclarations(), 0);
  }
  
  void *status;
  if(pthread_join(jit_thread, &status)) {
    cerr << L"Unable to join garbage collection threads!" << endl;
    exit(-1);
  }
  
#ifndef _GC_SERIAL
  pthread_exit(NULL);
#endif  
}

void MemoryManager::CheckMemory(size_t* mem, StackDclr** dclrs, const long dcls_size, long depth)
{
  // check method
  for(long i = 0; i < dcls_size; i++) {            
#ifdef _DEBUG
    for(int j = 0; j < depth; j++) {
      wcout << L"\t";
    }
#endif
    
    // update address based upon type
    switch(dclrs[i]->type) {
    case FUNC_PARM:
#ifdef _DEBUG
      wcout << L"\t" << i << L": FUNC_PARM: value=" << (*mem) 
            << L"," << *(mem + 1)<< endl;
#endif
      // update
      mem += 2;
      break;
      
    case CHAR_PARM:
    case INT_PARM:
#ifdef _DEBUG
      wcout << L"\t" << i << L": CHAR_PARM/INT_PARM: value=" << (*mem) << endl;
#endif
      // update
      mem++;
      break;

    case FLOAT_PARM: {
#ifdef _DEBUG
      FLOAT_VALUE value;
      memcpy(&value, mem, sizeof(FLOAT_VALUE));
      wcout << L"\t" << i << L": FLOAT_PARM: value=" << value << endl;
#endif
      // update
      mem += 2;
    }
      break;

    case BYTE_ARY_PARM:
#ifdef _DEBUG
      wcout << L"\t" << i << L": BYTE_ARY_PARM: addr=" << (size_t*)(*mem) << L"("
            << (long)(*mem) << L"), size=" << ((*mem) ? ((size_t*)(*mem))[SIZE_OR_CLS] : 0)
            << L" byte(s)" << endl;
#endif
      // mark data
      MarkMemory((size_t*)(*mem));
      // update
      mem++;
      break;

    case CHAR_ARY_PARM:
#ifdef _DEBUG
      wcout << L"\t" << i << L": CHAR_ARY_PARM: addr=" << (size_t*)(*mem) << L"("
            << (long)(*mem) << L"), size=" << ((*mem) ? ((size_t*)(*mem))[SIZE_OR_CLS] : 0) 
            << L" byte(s)" << endl;
#endif
      // mark data
      MarkMemory((size_t*)(*mem));
      // update
      mem++;
      break;

    case INT_ARY_PARM:
#ifdef _DEBUG
      wcout << L"\t" << i << L": INT_ARY_PARM: addr=" << (size_t*)(*mem) << L"("
            << (long)(*mem) << L"), size=" << ((*mem) ? ((size_t*)(*mem))[SIZE_OR_CLS] : 0) 
            << L" byte(s)" << endl;
#endif
      // mark data
      MarkMemory((size_t*)(*mem));
      // update
      mem++;
      break;

    case FLOAT_ARY_PARM:
#ifdef _DEBUG
      wcout << L"\t" << i << L": FLOAT_ARY_PARM: addr=" << (size_t*)(*mem) << L"("
            << (long)(*mem) << L"), size=" << ((*mem) ? ((size_t*)(*mem))[SIZE_OR_CLS] : 0) 
            << L" byte(s)" << endl;
#endif
      // mark data
      MarkMemory((size_t*)(*mem));
      // update
      mem++;
      break;

    case OBJ_PARM: {
#ifdef _DEBUG
      wcout << L"\t" << i << L": OBJ_PARM: addr=" << (size_t*)(*mem) << L"("
            << (long)(*mem) << L"), id=";
      if(*mem) {
        StackClass* tmp = (StackClass*)((size_t*)(*mem))[SIZE_OR_CLS];
        wcout << L"'" << tmp->GetName() << L"'" << endl;
      }
      else {
        wcout << L"Unknown" << endl;
      }
#endif
      // check object
      CheckObject((size_t*)(*mem), true, depth + 1);
      // update
      mem++;
    }
      break;
      
    case OBJ_ARY_PARM:
#ifdef _DEBUG
      wcout << L"\t" << i << L": OBJ_ARY_PARM: addr=" << (size_t*)(*mem) << L"("
            << (long)(*mem) << L"), size=" << ((*mem) ? ((size_t*)(*mem))[SIZE_OR_CLS] : 0) 
            << L" byte(s)" << endl;
#endif
      // mark data
      if(MarkValidMemory((size_t*)(*mem))) {
        size_t* array = (size_t*)(*mem);
        const long size = array[0];
        const long dim = array[1];
        size_t* objects = (size_t*)(array + 2 + dim);
        for(long k = 0; k < size; k++) {
          CheckObject((size_t*)objects[k], true, 2);
        }
      }
      // update
      mem++;
      break;

    default:
      break;
    }
  }
}

void MemoryManager::CheckObject(size_t* mem, bool is_obj, long depth)
{
  if(mem) {
    StackClass* cls;
    if(is_obj) {
      cls = GetClass(mem);
    }
    else {
      cls = GetClassMapping(mem);
    }
    
    if(cls) {
#ifdef _DEBUG
      for(int i = 0; i < depth; i++) {
        wcout << L"\t";
      }
      wcout << L"\t----- object: addr=" << mem << L"(" << (long)mem << L"), num="
            << cls->GetNumberInstanceDeclarations() << L" -----" << endl;
#endif

      // mark data
      if(MarkMemory(mem)) {
        CheckMemory(mem, cls->GetInstanceDeclarations(), cls->GetNumberInstanceDeclarations(), depth);
      }
    } 
    else {
      // NOTE: this happens when we are trying to mark unidentified memory
      // segments. these segments may be parts of that stack or temp for
      // register variables
#ifdef _DEBUG
      for(int i = 0; i < depth; i++) {
        wcout << L"\t";
      }
      wcout <<"$: addr/value=" << mem << endl;
      if(is_obj) {
        assert(cls);
      }
#endif
      // primitive or object array
      if(MarkValidMemory(mem)) {
        // ensure we're only checking int and obj arrays
        if(std::binary_search(allocated_memory.begin(), allocated_memory.end(), mem) && 
           (mem[TYPE] == NIL_TYPE || mem[TYPE] == INT_TYPE)) {
          size_t* array = mem;
          const long size = array[0];
          const long dim = array[1];
          size_t* objects = (size_t*)(array + 2 + dim);
          for(long k = 0; k < size; k++) {
            CheckObject((size_t*)objects[k], false, 2);
          }
        }
      }
    }
  }
}
