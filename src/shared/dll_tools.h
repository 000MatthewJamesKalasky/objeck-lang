/***************************************************************************
 * DLL tools
 *
 * Copyright (c) 2008-2011, Randy Hollines
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

#ifndef __DLL_TOOLS_H__
#define __DLL_TOOLS_H__

#include <string.h>
#include "../vm/common.h"
#include "../vm/interpreter.h"

int DLLTools_GetArraySize(long* array) {
  if(array) {
    return array[0];
  }

  return 0;
}

int DLLTools_GetIntValue(long* array, int index) {
  if(array && index < array[0]) {
    array += 3;
    long* int_holder = (long*)array[index];
    return int_holder[0];
  }

  return 0.0;
}

void DLLTools_SetIntValue(long* array, int index, int value) {
  if(array && index < array[0]) {
    array += 3;
    long* int_holder = (long*)array[index];
    int_holder[0] = value;
  }
}

double DLLTools_GetFloatValue(long* array, int index) {
  if(array && index < array[0]) {
    array += 3;
    long* float_holder = (long*)array[index];
		
    double value;
    memcpy(&value, float_holder, sizeof(value));
    return value;
  }

  return -1.0;
} 

void DLLTools_SetFloatValue(long* array, int index, double value) {
  if(array && index < array[0]) {
    array += 3;
    long* float_holder = (long*)array[index];
    memcpy(float_holder, &value, sizeof(value));
  }
}

//////////////////////////

long DLLTools_PopInt(long* op_stack, long *stack_pos) {
  long value = op_stack[--(*stack_pos)];
#ifdef _DEBUG
  cout << "\t[pop_i: value=" << (long*)value << "(" << value << ")]" << "; pos=" << (*stack_pos) << endl;
#endif

  return value;
}

void DLLTools_PushInt(long* op_stack, long *stack_pos, long value) {
  op_stack[(*stack_pos)++] = value;
#ifdef _DEBUG
  cout << "\t[push_i: value=" << (long*)value << "(" << value << ")]" << "; pos=" << (*stack_pos) << endl;
#endif
}

#endif
