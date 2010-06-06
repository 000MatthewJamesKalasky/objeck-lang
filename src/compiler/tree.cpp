/***************************************************************************
 * Language parse tree.
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

#include "tree.h"

using namespace frontend;

/****************************
 * Expression class
 ****************************/
void Expression::SetMethodCall(MethodCall* call)
{
  if(call) {
    method_call = call;
    call->SetPreviousExpression(this);
  }
}

/****************************
 * TreeFactory class
 ****************************/
TreeFactory* TreeFactory::instance;

TreeFactory* TreeFactory::Instance()
{
  if(!instance) {
    instance = new TreeFactory;
  }

  return instance;
}

/****************************
 * Sets the id for all
 * references to a variable.
 ****************************/
void SymbolEntry::SetId(int i)
{
  id = i;
  for(unsigned int j = 0; j < variables.size(); j++) {
    variables[j]->SetId(i);
  }
}

/****************************
 * Encodes a method parameter
 ****************************/
string Method::EncodeType(Type* type, ParsedProgram* program, Linker* linker)
{
  string name;
  if(type) {
    // type
    switch(type->GetType()) {
    case BOOLEAN_TYPE:
      name = 'l';
      break;

    case BYTE_TYPE:
      name = 'b';
      break;

    case INT_TYPE:
      name = 'i';
      break;

    case FLOAT_TYPE:
      name = 'f';
      break;

    case CHAR_TYPE:
      name = 'c';
      break;

    case NIL_TYPE:
      name = 'n';
      break;

    case VAR_TYPE:
      name = 'v';
      break;

    case CLASS_TYPE: {
      name = "o.";

      // search program
      string klass_name = type->GetClassName();
      Class* klass = program->GetClass(klass_name);
      if(!klass) {
        vector<string> uses = program->GetUses();
        for(unsigned int i = 0; !klass && i < uses.size(); i++) {
          klass = program->GetClass(uses[i] + "." + klass_name);
        }
      }
      if(klass) {
        name += klass->GetName();
      }
      // search libaraires
      else {
        LibraryClass* lib_klass = linker->SearchClassLibraries(klass_name, program->GetUses());
        if(lib_klass) {
          name += lib_klass->GetName();
        } else {
          name += type->GetClassName();
        }
      }
    }
    break;
    }
    // dimension
    for(int i = 0; i < type->GetDimension(); i++) {
      name += '*';
    }
  }

  return name;
}

/****************************
 * Validates a static array
 ****************************/
void StaticArray::Validate(StaticArray* array) {
  vector<Expression*> static_array = array->GetElements()->GetExpressions();
  for(int i = 0; i < static_array.size(); i++) { 
    if(static_array[i]) {
      if(static_array[i]->GetExpressionType() == STAT_ARY_EXPR) {
	dim = static_array.size();  
	Validate(static_cast<StaticArray*>(static_array[i]));
      }
      else {
	// check lengths
	if(cur_length == -1) {
	  cur_length = static_array.size();
	}
	if(cur_length != static_array.size()) {
	  matching_lengths = false;
	}      
	// check types
	if(cur_type == VAR_EXPR) {
	  cur_type = static_array[i]->GetExpressionType();
	}
	else if(cur_type != static_array[i]->GetExpressionType()) {
	  matching_types = false;
	}
      }
    }
  }
}

/****************************
 * Get all static elements
 ****************************/
ExpressionList* StaticArray::GetAllElements() {
  if(!all_elements) {
    all_elements = TreeFactory::Instance()->MakeExpressionList();  
    GetAllElements(this, all_elements);
  }
  
  return all_elements;
}

int StaticArray::GetSize(int d) {
  vector<Expression*> expressions = elements->GetExpressions();
  if(expressions[0] && expressions[0]->GetExpressionType() == STAT_ARY_EXPR) {
    vector<Expression*> static_array = 
      static_cast<StaticArray*>(expressions[0])->GetElements()->GetExpressions();
    if(d < static_array.size()) {
      if(static_array[d]) {
	cout << "HELLO" << endl;
	if(static_array[d]->GetExpressionType() == STAT_ARY_EXPR) {
	  return static_cast<StaticArray*>(static_array[d])->GetElements()->GetExpressions().size();
	}
      }
    }
  }
  
  return -1;
}
