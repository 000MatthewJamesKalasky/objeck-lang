/***************************************************************************

 * Translates a parse tree into an intermediate format.  This format
 * is used for optimizations and target output.
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
 * the documentation and/cor other materials provided with the distribution.
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

#include "intermediate.h"
#include "linker.h"
/****************************
 * CaseArrayTree constructor
 ****************************/
SelectArrayTree::SelectArrayTree(Select* s, IntermediateEmitter* e)
{
  select = s;
  emitter = e;
  map<int, StatementList*> label_statements = select->GetLabelStatements();
  values = new int[label_statements.size()];
  // map and sort values
  int i = 0;
  map<int, StatementList*>::iterator iter;
  for(iter = label_statements.begin(); iter != label_statements.end(); iter++) {
    values[i] = iter->first;
    value_label_map[iter->first] = ++emitter->conditional_label;
    i++;
  }
  // map other
  if(select->GetOther()) {
    other_label = ++emitter->conditional_label;
  }
  // create tree
  root = divide(0, (int)label_statements.size() - 1);
}

/****************************
 * Divides value array
 ****************************/
SelectNode* SelectArrayTree::divide(int start, int end)
{
  const int size =  end - start + 1;
  if(size < 4) {
    if(size == 2) {
      SelectNode* node = new SelectNode(++emitter->conditional_label, values[start + 1],
					new SelectNode(++emitter->conditional_label, values[start]),
					new SelectNode(++emitter->conditional_label, values[start + 1]));
      return node;	
    }
    else {
      SelectNode* node = new SelectNode(++emitter->conditional_label, 
					values[start + 1], values[start + 2],
					new SelectNode(++emitter->conditional_label, values[start]),
					new SelectNode(++emitter->conditional_label, values[start + 2]));
      return node;	
    }
  }
  else {
    SelectNode* node;
    const int middle = size / 2 + start;
    if(size % 2 == 0) {
      SelectNode* left = divide(start, middle - 1);
      SelectNode* right = divide(middle, end);
      node = new SelectNode(++emitter->conditional_label, 
			    values[middle], left, right);
    }
    else {
      SelectNode* left = divide(start, middle - 1);
      SelectNode* right = divide(middle + 1, end);
      node = new SelectNode(++emitter->conditional_label, values[middle], 
			    values[middle], left, right);
    }

    return node;
  }  
}

/****************************
 * Emits code for a case statement
 ****************************/
void SelectArrayTree::Emit()
{
  emitter->cur_line_num = select->GetLineNumber();
  
  int end_label = ++emitter->unconditional_label;
  Emit(root, end_label);
  // write statements
  map<int, StatementList*> label_statements = select->GetLabelStatements();
  map<int, StatementList*>::iterator iter;
  for(iter = label_statements.begin(); iter != label_statements.end(); iter++) {
    emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, LBL, value_label_map[iter->first]));
    StatementList* statement_list = iter->second;
    vector<Statement*> statements = statement_list->GetStatements();
    for(unsigned int i = 0; i < statements.size(); i++) {
      emitter->EmitStatement(statements[i]);
    }
    emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, JMP, end_label, -1));
  }

  if(select->GetOther()) {
    emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, LBL, other_label));
    StatementList* statement_list = select->GetOther();
    vector<Statement*> statements = statement_list->GetStatements();
    for(unsigned int i = 0; i < statements.size(); i++) {
      emitter->EmitStatement(statements[i]);
    }
    emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, JMP, end_label, -1));
  }
  emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, LBL, end_label));
}

/****************************
 * Emits code for a case statement
 ****************************/
void SelectArrayTree::Emit(SelectNode* node, int end_label)
{
  if(node != NULL) {
    SelectNode* left = node->GetLeft();
    SelectNode* right = node->GetRight();

    emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, LBL, node->GetId()));
    if(node->GetOperation() == CASE_LESS) {
      const int value = node->GetValue();
      // evaluate less then
      emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, LOAD_INT_LIT, value));
      emitter->EmitExpression(select->GetExpression());
      emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, LES_INT));
    } 
    else if(node->GetOperation() == CASE_EQUAL) {
      const int value = node->GetValue();
      // evaluate equal to
      emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, LOAD_INT_LIT, value));
      emitter->EmitExpression(select->GetExpression());
      emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, EQL_INT));
      // true
      emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, JMP, value_label_map[value], true));
      // false
      if(select->GetOther()) {
        emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, JMP, other_label, -1));
      } 
      else {
        emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, JMP, end_label, -1));
      }
    } 
    else {
      // evaluate equal to
      const int value = node->GetValue();
      emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, LOAD_INT_LIT, value));
      emitter->EmitExpression(select->GetExpression());
      emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, EQL_INT));
      // true
      emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, JMP, value_label_map[value], true));
      // evaluate less then
      emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, LOAD_INT_LIT, node->GetValue2()));
      emitter->EmitExpression(select->GetExpression());
      emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, LES_INT));
    }

    if(left != NULL && right != NULL) {
      emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, JMP, left->GetId(), true));
      emitter->imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(emitter->cur_line_num, JMP, right->GetId(), -1));
    }

    Emit(left, end_label);
    Emit(right, end_label);
  }
}

/****************************
 * Starts the process of
 * translating the parse tree.
 ****************************/
void IntermediateEmitter::Translate()
{
  parsed_program->GetLinker()->ResloveExternalClasses();
  int class_id = 0;
  
#ifndef _SYSTEM
  vector<LibraryClass*> lib_classes = parsed_program->GetLinker()->GetAllClasses();
  for(unsigned int i = 0; i < lib_classes.size(); i++) {
    if(is_lib || lib_classes[i]->GetCalled()) {
      lib_classes[i]->SetId(class_id++);
    }
  }
#endif
  
  // process bundles
  vector<ParsedBundle*> bundles = parsed_program->GetBundles();
  for(unsigned int i = 0; i < bundles.size(); i++) {
    vector<Class*> classes = bundles[i]->GetClasses();
    for(unsigned int j = 0; j < classes.size(); j++) {
      if(is_lib || classes[i]->GetCalled()) {
        classes[j]->SetId(class_id++);
      }
    }
  }

  // emit strings
  EmitStrings();
  // emit libraries
  EmitLibraries(parsed_program->GetLinker());
  // emit program
  EmitBundles();
  Class* start_class = parsed_program->GetStartClass();
  Method* start_method = parsed_program->GetStartMethod();
  imm_program->SetStartIds((start_class ? start_class->GetId() : -1),
                           (start_method ? start_method->GetId() : -1));
  
#ifdef _DEBUG
  assert(break_labels.empty());
#endif

  // free parse tree
  delete parsed_program;
  parsed_program = NULL;
}

/****************************
 * Translates libraries
 ****************************/
void IntermediateEmitter::EmitLibraries(Linker* linker)
{
  if(linker && !is_lib) {
    // resolve external libraries
    linker->ResloveExternalMethodCalls();
    // write enums
    vector<LibraryEnum*> lib_enums = linker->GetAllEnums();
    for(unsigned int i = 0; i < lib_enums.size(); i++) {
      imm_program->AddEnum(new IntermediateEnum(lib_enums[i]));
    }
    // write classes
    vector<LibraryClass*> lib_classes = linker->GetAllClasses();
    for(unsigned int i = 0; i < lib_classes.size(); i++) {
      if(is_lib || lib_classes[i]->GetCalled()) {
        imm_program->AddClass(new IntermediateClass(lib_classes[i]));
      }
    }
  }
}

/****************************
 * Emits strings
 ****************************/
void IntermediateEmitter::EmitStrings()
{
  vector<string> char_string_values = parsed_program->GetCharStrings();
  vector<IntStringHolder*> int_string_values = parsed_program->GetIntStrings();
  vector<FloatStringHolder*> float_string_values = parsed_program->GetFloatStrings();
  
  Linker* linker = parsed_program->GetLinker();
  if(linker && !is_lib) {
    map<const string, Library*> libraries = linker->GetAllLibraries();
    map<const string, Library*>::iterator iter;
    
    // create master list of library strings
    vector<string> lib_char_string_values;
    vector<IntStringHolder*> lib_int_string_values;
    vector<FloatStringHolder*> lib_float_string_values;
    for(iter = libraries.begin(); iter != libraries.end(); iter++) {
      // char string processing
      vector<CharStringInstruction*> char_str_insts = iter->second->GetCharStringInstructions();
      for(unsigned int i = 0; i < char_str_insts.size(); i++) {
	// check for dups
	bool found = false;
	for(unsigned int j = 0; !found && j < lib_char_string_values.size(); j++) {
	  if(char_str_insts[i]->value == lib_char_string_values[j]) {
	    found = true;
	  }
	}
	// add string
	if(!found) {
	  lib_char_string_values.push_back(char_str_insts[i]->value);
	}
      }      
      // int string processing
      vector<IntStringInstruction*> int_str_insts = iter->second->GetIntStringInstructions();
      for(unsigned int i = 0; i < int_str_insts.size(); i++) {
	// check for dups
	bool found = false;
	for(unsigned int j = 0; !found && j < lib_int_string_values.size(); j++) {
	  if(IntStringHolderEqual(int_str_insts[i]->value, lib_int_string_values[j])) {
	    found = true;
	  }
	}
	// add string
	if(!found) {
	  lib_int_string_values.push_back(int_str_insts[i]->value);
	}
      }
      // float string processing
      vector<FloatStringInstruction*> float_str_insts = iter->second->GetFloatStringInstructions();
      for(unsigned int i = 0; i < float_str_insts.size(); i++) {
	// check for dups
	bool found = false;
	for(unsigned int j = 0; !found && j < lib_float_string_values.size(); j++) {
	  if(FloatStringHolderEqual(float_str_insts[i]->value, lib_float_string_values[j])) {
	    found = true;
	  }
	}
	// add string
	if(!found) {
	  lib_float_string_values.push_back(float_str_insts[i]->value);
	}
      }  
    }

    // merge in library strings
    for(unsigned int i = 0; i < lib_char_string_values.size(); i++) {
      // check for dups
      bool found = false;
      for(unsigned int j = 0; !found && j < char_string_values.size(); j++) {
	if(lib_char_string_values[i] == char_string_values[j]) {
	  found = true;
	}
      }
      // add string
      if(!found) {
	char_string_values.push_back(lib_char_string_values[i]);
      }
    }
    for(unsigned int i = 0; i < lib_int_string_values.size(); i++) {
      // check for dups
      bool found = false;
      for(unsigned int j = 0; !found && j < int_string_values.size(); j++) {
	if(IntStringHolderEqual(lib_int_string_values[i], int_string_values[j])) {
	  found = true;
	}
      }
      // add string
      if(!found) {
	int_string_values.push_back(lib_int_string_values[i]);
      }
    }
    for(unsigned int i = 0; i < lib_float_string_values.size(); i++) {
      // check for dups
      bool found = false;
      for(unsigned int j = 0; !found && j < float_string_values.size(); j++) {
	if(FloatStringHolderEqual(lib_float_string_values[i], float_string_values[j])) {
	  found = true;
	}
      }
      // add string
      if(!found) {
	float_string_values.push_back(lib_float_string_values[i]);
      }
    }
    
    // update indices
    for(iter = libraries.begin(); iter != libraries.end(); iter++) {
      // char string processing
      vector<CharStringInstruction*> char_str_insts = iter->second->GetCharStringInstructions();
      for(unsigned int i = 0; i < char_str_insts.size(); i++) {
	bool found = false;
	for(unsigned int j = 0; !found && j < char_string_values.size(); j++) {
	  if(char_str_insts[i]->value == char_string_values[j]) {
	    vector<LibraryInstr*> instrs = char_str_insts[i]->instrs;
	    for(unsigned int k = 0; k < instrs.size(); k++) {
	      instrs[k]->SetOperand(j);
	    }
	    found = true;
	  }
	}
#ifdef _DEBUG
	assert(found);
#endif
      }
      // int string processing
      vector<IntStringInstruction*> int_str_insts = iter->second->GetIntStringInstructions();
      for(unsigned int i = 0; i < int_str_insts.size(); i++) {
	bool found = false;
	for(unsigned int j = 0; !found && j < int_string_values.size(); j++) {
	  if(IntStringHolderEqual(int_str_insts[i]->value, int_string_values[j])) {
	    vector<LibraryInstr*> instrs = int_str_insts[i]->instrs;
	    for(int k = 0; k < instrs.size(); k++) {
	      instrs[k]->SetOperand(j);
	    }
	    found = true;
	  }
	}
#ifdef _DEBUG
	assert(found);
#endif
      }
      // float string processing
      vector<FloatStringInstruction*> float_str_insts = iter->second->GetFloatStringInstructions();
      for(unsigned int i = 0; i < float_str_insts.size(); i++) {
	bool found = false;
	for(unsigned int j = 0; !found && j < float_string_values.size(); j++) {
	  if(FloatStringHolderEqual(float_str_insts[i]->value, float_string_values[j])) {
	    vector<LibraryInstr*> instrs = float_str_insts[i]->instrs;
	    for(int k = 0; k < instrs.size(); k++) {
	      instrs[k]->SetOperand(j);
	    }
	    found = true;
	  }
	}
#ifdef _DEBUG
	assert(found);
#endif
      }
    }
  }
  // set static strings
  imm_program->SetCharStrings(char_string_values);
  imm_program->SetIntStrings(int_string_values);
  imm_program->SetFloatStrings(float_string_values);
}

/****************************
 * Translates bundles
 ****************************/
void IntermediateEmitter::EmitBundles()
{
  // translate program into intermediate form process bundles
  vector<string> bundle_names;
  vector<ParsedBundle*> bundles = parsed_program->GetBundles();
  for(unsigned int i = 0; i < bundles.size(); i++) {
    parsed_bundle = bundles[i];
    bundle_names.push_back(parsed_bundle->GetName());
    // emit enums
    vector<Enum*> enums = parsed_bundle->GetEnums();
    for(unsigned int j = 0; j < enums.size(); j++) {
      IntermediateEnum* eenum = EmitEnum(enums[j]);
      if(eenum) {
        imm_program->AddEnum(eenum);
      }
    }
    // emit classes
    vector<Class*> classes = parsed_bundle->GetClasses();
    for(unsigned int j = 0; j < classes.size(); j++) {
      if(is_lib || classes[j]->GetCalled()) {
        IntermediateClass* klass = EmitClass(classes[j]);
        if(klass) {
          imm_program->AddClass(klass);
        }
      }
    }
  }
  imm_program->SetBundleNames(bundle_names);
}

/****************************
 * Translates a enum
 ****************************/
IntermediateEnum* IntermediateEmitter::EmitEnum(Enum* eenum)
{
  cur_line_num = eenum->GetLineNumber();
  
  IntermediateEnum* imm_eenum = new IntermediateEnum(eenum->GetName(), eenum->GetOffset());
  map<const string, EnumItem*>items =  eenum->GetItems();
  // copy items
  map<const string, EnumItem*>::iterator iter;
  for(iter = items.begin(); iter != items.end(); iter++) {
    imm_eenum->AddItem(new IntermediateEnumItem(iter->second->GetName(), iter->second->GetId()));
  }

  return imm_eenum;
}

/****************************
 * Translates a class
 ****************************/
IntermediateClass* IntermediateEmitter::EmitClass(Class* klass)
{
  cur_line_num = klass->GetLineNumber();
  imm_klass = NULL;

  current_class = klass;
  current_table = current_class->GetSymbolTable();
  current_method = NULL;

  // entries
  IntermediateDeclarations* entries = new IntermediateDeclarations;
#ifdef _DEBUG
  cout << "---------- Intermediate ---------" << endl;
  cout << "Class variables (class): name=" << klass->GetName() << endl;
#endif
  int cls_space = CalculateEntrySpace(entries, true);

#ifdef _DEBUG
  cout << "Class variables (instance): name=" << klass->GetName() << endl;
#endif
  int inst_space = CalculateEntrySpace(entries, false);

  // set parent id
  string parent_name;
  int pid = -1;
  Class* parent = current_class->GetParent();
  if(parent) {
    pid = parent->GetId();
    parent_name = parent->GetName();
  } 
  else {
    LibraryClass* lib_parent = current_class->GetLibraryParent();
    if(lib_parent) {
      pid = lib_parent->GetId();
      parent_name = lib_parent->GetName();
    }
  }

  // get short file name
  const string &file_name = current_class->GetFileName();
  int offset = file_name.find_last_of("/\\");
  string short_file_name;
  if(offset < 0) {
    short_file_name = file_name;
  }
  else {
    short_file_name = file_name.substr(offset + 1);
  }
  
  imm_klass = new IntermediateClass(current_class->GetId(), current_class->GetName(),
                                    pid, parent_name, current_class->GetInterfaceNames(), 
				    current_class->IsVirtual(), cls_space, inst_space, 
				    entries, short_file_name, is_debug);
  // block
  NewBlock();
  
  // declarations
  vector<Statement*> statements = klass->GetStatements();
  for(unsigned int i = 0; i < statements.size(); i++) {
    EmitDeclaration(static_cast<Declaration*>(statements[i]));
  }

  // methods
  vector<Method*> methods = klass->GetMethods();
  for(unsigned int i = 0; i < methods.size(); i++) {
    imm_klass->AddMethod(EmitMethod(methods[i]));
  }

  current_class = NULL;

  return imm_klass;
}

/****************************
 * Translates a method
 ****************************/
IntermediateMethod* IntermediateEmitter::EmitMethod(Method* method)
{
  cur_line_num = method->GetLineNumber();
  
  current_method = method;
  current_table = current_method->GetSymbolTable();

  // gather information
  IntermediateDeclarations* entries = new IntermediateDeclarations;

#ifdef _DEBUG
  cout << "Class variables (local): name=" << method->GetName() << endl;
#endif
  int space = CalculateEntrySpace(entries, false);
  vector<Declaration*> declarations = method->GetDeclarations()->GetDeclarations();
  int num_params = 0;
  for(int i = 0; i < declarations.size(); i++) {
    if(declarations[i]->GetEntry()->GetType()->GetType() == frontend::FUNC_TYPE) {
      num_params += 2;
    }
    else {
      num_params++;
    }
  }
  // int num_params = (int)declarations.size();
  imm_method = new IntermediateMethod(method->GetId(), method->GetEncodedName(),
                                      method->IsVirtual(), method->HasAndOr(), method->GetEncodedReturn(),
                                      method->GetMethodType(), method->IsNative(), method->IsStatic(),
                                      space, num_params, entries, imm_klass);

  if(!method->IsVirtual()) {
    // block
    NewBlock();

    // declarations
    for(int i = (int)declarations.size() - 1; i >= 0; i--) {
      SymbolEntry* entry = declarations[i]->GetEntry();
      if(!entry->IsSelf()) {
        switch(entry->GetType()->GetType()) {
        case frontend::BOOLEAN_TYPE:
        case frontend::BYTE_TYPE:
        case frontend::CHAR_TYPE:
        case frontend::INT_TYPE:
        case frontend::CLASS_TYPE:
          imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_INT_VAR, entry->GetId(), LOCL));
          break;

        case frontend::FLOAT_TYPE:
          if(entry->GetType()->GetDimension() > 0) {
            imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_INT_VAR, entry->GetId(), LOCL));
          } 
	  else {
            imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_FLOAT_VAR, entry->GetId(), LOCL));
          }
          break;
	  
	case frontend::FUNC_TYPE:
          imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_FUNC_VAR, entry->GetId(), LOCL));
          break;
        }
      }
    }

    // statements
    bool end_return = false;
    StatementList* statement_list = method->GetStatements();
    if(statement_list) {
      vector<Statement*> statements = statement_list->GetStatements();
      for(unsigned int i = 0; i < statements.size(); i++) {
        EmitStatement(statements[i]);
      }
      // check to see if the last statement was a return statement
      if(statements.size() > 0 && statements.back()->GetStatementType() == RETURN_STMT) {
        end_return = true;
      }
    }

    // return instance if this is constructor call
    if(method->GetMethodType() == NEW_PUBLIC_METHOD ||
       method->GetMethodType() == NEW_PRIVATE_METHOD) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    }

    // add return statement if one hasn't been added
    if(!end_return) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, RTRN));
    }
  }
  
  current_method = NULL;
  current_table = NULL;

  return imm_method;
}

/****************************
 * Translates a statement
 ****************************/
void IntermediateEmitter::EmitStatement(Statement* statement)
{
  cur_line_num = statement->GetLineNumber();
  
  switch(statement->GetStatementType()) {
  case DECLARATION_STMT:
    EmitDeclaration(static_cast<Declaration*>(statement));
    break;

  case METHOD_CALL_STMT: {
    // find end of nested call
    MethodCall* method_call = static_cast<MethodCall*>(statement);
    if(method_call->IsFunctionDefinition()) {
      if(method_call->GetMethod()) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, method_call->GetMethod()->GetId()));
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, method_call->GetMethod()->GetClass()->GetId()));
      }
      else {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, method_call->GetLibraryMethod()->GetId()));
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, method_call->GetLibraryMethod()->GetLibraryClass()->GetId()));
      }
    }
    else if(method_call->IsDynamicFunctionCall()) {
      MethodCall* tail = method_call;
      while(tail->GetMethodCall()) {
	tail = tail->GetMethodCall();
      }

      // emit parameters for nested call
      MethodCall* temp = static_cast<MethodCall*>(tail);
      while(temp) {
	EmitMethodCallParameters(temp);
	// update
	temp = static_cast<MethodCall*>(temp->GetPreviousExpression());
      }

      // emit function variable
      MemoryContext mem_context;
      SymbolEntry* entry = method_call->GetDynamicFunctionEntry();
      if(entry->IsLocal()) {
	mem_context = LOCL;
      } 
      else if(entry->IsStatic()) {
	mem_context = CLS;
      } 
      else {
	mem_context = INST;
      }
      //
      if(mem_context == INST) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
      } 
      else if(mem_context == CLS) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
      }      
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FUNC_VAR, entry->GetId(), mem_context));
      
      // emit dynamic call
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
      switch(OrphanReturn(method_call)) {
      case 0:
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, DYN_MTHD_CALL, entry->GetType()->GetFunctionParameterCount(), instructions::INT_TYPE));
	if(!method_call->GetMethodCall()) {
	  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));
	}
	break;
	
      case 1:
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, DYN_MTHD_CALL, entry->GetType()->GetFunctionParameterCount(), instructions::FLOAT_TYPE));
	if(!method_call->GetMethodCall()) {
	  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_FLOAT));
	}
	break;
	
      case 2:
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, DYN_MTHD_CALL, entry->GetType()->GetFunctionParameterCount(), instructions::FUNC_TYPE));
	if(!method_call->GetMethodCall()) {
	  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));
	  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));	
	}
	break;
	
      default:
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, DYN_MTHD_CALL, entry->GetType()->GetFunctionParameterCount(), instructions::NIL_TYPE));
	break;
      }
      
      // emit nested method calls
      bool is_nested = false; // fuction call
      method_call = method_call->GetMethodCall();
      while(method_call) {
	EmitMethodCall(method_call, is_nested, method_call->GetCastType() != NULL);
	// pop return value if not used
	if(!method_call->GetMethodCall()) {
	  switch(OrphanReturn(method_call)) {
	  case 0:
	    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));
	    break;

	  case 1:
	    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_FLOAT));
	    break;

	  case 2:
	    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));
	    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));
	    break;
	  }
	}
	// next call
	if(method_call->GetMethod()) {
	  Method* method = method_call->GetMethod();
	  if(method->GetReturn()->GetType() == CLASS_TYPE) {
	    is_nested = true;
	  } 
	  else {
	    is_nested = false;
	  }
	} 
	else if(method_call->GetLibraryMethod()) {
	  LibraryMethod* lib_method = method_call->GetLibraryMethod();
	  if(lib_method->GetReturn()->GetType() == CLASS_TYPE) {
	    is_nested = true;
	  } 
	  else {
	    is_nested = false;
	  }
	} 
	else {
	  is_nested = false;
	}
	method_call = method_call->GetMethodCall();
      } 
    }
    else {
      MethodCall* tail = method_call;
      while(tail->GetMethodCall()) {
	tail = tail->GetMethodCall();
      }

      // emit parameters for nested call
      MethodCall* temp = static_cast<MethodCall*>(tail);
      while(temp) {
	EmitMethodCallParameters(temp);
	// update
	temp = static_cast<MethodCall*>(temp->GetPreviousExpression());
      }

      // emit method calls
      bool is_nested = false;
      do {
	EmitMethodCall(method_call, is_nested, method_call->GetCastType() != NULL);
	// pop return value if not used
	if(!method_call->GetMethodCall()) {
	  switch(OrphanReturn(method_call)) {
	  case 0:
	    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));
	    break;

	  case 1:
	    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_FLOAT));
	    break;

	  case 2:
	    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));
	    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));
	    break;
	  }
	}
	// next call
	if(method_call->GetMethod()) {
	  Method* method = method_call->GetMethod();
	  if(method->GetReturn()->GetType() == CLASS_TYPE) {
	    is_nested = true;
	  } 
	  else {
	    is_nested = false;
	  }
	} 
	else if(method_call->GetLibraryMethod()) {
	  LibraryMethod* lib_method = method_call->GetLibraryMethod();
	  if(lib_method->GetReturn()->GetType() == CLASS_TYPE) {
	    is_nested = true;
	  } 
	  else {
	    is_nested = false;
	  }
	} 
	else {
	  is_nested = false;
	}
	method_call = method_call->GetMethodCall();
      } 
      while(method_call);
    }
  }
    break;

  case ASSIGN_STMT:
  case ADD_ASSIGN_STMT:
  case SUB_ASSIGN_STMT:
  case MUL_ASSIGN_STMT:
  case DIV_ASSIGN_STMT:
    EmitAssignment(static_cast<Assignment*>(statement));
    break;
    
  case SIMPLE_STMT:
    EmitExpression(static_cast<SimpleStatement*>(statement)->GetExpression());
    break;

  case RETURN_STMT: {
    Expression* expression = static_cast<Return*>(statement)->GetExpression();
    if(expression) {
      EmitExpression(expression);
    }
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, RTRN));
    break;
  }
  case IF_STMT:
    EmitIf(static_cast<If*>(statement));
    break;

  case SELECT_STMT:
    EmitSelect(static_cast<Select*>(statement));
    break;

  case DO_WHILE_STMT:
    EmitDoWhile(static_cast<DoWhile*>(statement));
    break;
    
  case WHILE_STMT:
    EmitWhile(static_cast<While*>(statement));
    break;

  case FOR_STMT:
    EmitFor(static_cast<For*>(statement));
    break;

  case BREAK_STMT: {
    int break_label = break_labels.top();
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, break_label, -1));
  }
    break;

  case CRITICAL_STMT:
    EmitCriticalSection(static_cast<CriticalSection*>(statement));
    break;
    
  case SYSTEM_STMT:
    EmitSystemDirective(static_cast<SystemStatement*>(statement));
    break;
  }
}

/****************************
 * Translates a system
 * directive. Only used in
 * bootstrap.
 ****************************/
void IntermediateEmitter::EmitSystemDirective(SystemStatement* statement)
{
  cur_line_num = statement->GetLineNumber();
  
  switch(statement->GetId()) {
  
    
  case instructions::LOAD_INST_UID:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    break;
    
  case instructions::FLOR_FLOAT:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, FLOR_FLOAT));
    break;
    
  case instructions::CEIL_FLOAT:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, CEIL_FLOAT));
    break;
    
  case instructions::SQRT_FLOAT:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, SQRT_FLOAT));
    break;
    
  case instructions::SIN_FLOAT:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, SIN_FLOAT));
    break;
    
  case instructions::COS_FLOAT:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, COS_FLOAT));
    break;
    
  case instructions::TAN_FLOAT:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TAN_FLOAT));
    break;
    
  case instructions::LOG_FLOAT:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOG_FLOAT));
    break;

  case instructions::RAND_FLOAT:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, RAND_FLOAT));
    break;
    
  case instructions::POW_FLOAT:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_VAR, 2, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POW_FLOAT));
    break;
        
  case ASYNC_MTHD_CALL:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, ASYNC_MTHD_CALL, -1, 1L, 1L));
    break;
    
  case THREAD_MUTEX:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, THREAD_MUTEX));
    break;
    
  case THREAD_JOIN:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, THREAD_JOIN));
    break;
    
  case THREAD_SLEEP:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, THREAD_SLEEP));
    break;
    
  case CRITICAL_START:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, CRITICAL_START));
    break;
    
  case CRITICAL_END:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, CRITICAL_END));
    break;
    
    /////////////////////////////////////////
    // -------------- traps -------------- //
    /////////////////////////////////////////
    
  case instructions::LOAD_ARY_SIZE:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::LOAD_ARY_SIZE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP_RTRN, 2));
    break;
    
  case instructions::LOAD_CLS_INST_ID:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::LOAD_CLS_INST_ID));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP_RTRN, 2));
    break;

  case SYS_TIME:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::SYS_TIME));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;
    
  case PLTFRM:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::PLTFRM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;
    
  case TIMER_START:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::TIMER_START));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case TIMER_END:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::TIMER_END));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;
    
    // -------------- standard i/o --------------
  case instructions::STD_OUT_BOOL:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::STD_OUT_BOOL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case instructions::STD_OUT_BYTE:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::STD_OUT_BYTE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case instructions::STD_OUT_CHAR:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::STD_OUT_CHAR));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case instructions::STD_OUT_INT:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::STD_OUT_INT));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case instructions::STD_OUT_FLOAT:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT,
									       (INT_VALUE)instructions::STD_OUT_FLOAT));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case instructions::STD_OUT_CHAR_ARY:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::STD_OUT_CHAR_ARY));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case instructions::STD_IN_STRING:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::STD_IN_STRING));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;
    
    //----------- ip socket methods -----------
  case instructions::SOCK_TCP_HOST_NAME: {
    // copy and create Char[]
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)255 + 1));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_BYTE_ARY, (INT_VALUE)1));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::SOCK_TCP_HOST_NAME));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP_RTRN, 2));
#ifdef _SYSTEM
    Class* klass = SearchProgramClasses("System.String");
    assert(klass);
    int string_cls_id = klass->GetId();
#else
    LibraryClass* lib_klass = parsed_program->GetLinker()->SearchClassLibraries("System.String");
    assert(lib_klass);
    int string_cls_id = lib_klass->GetId();
#endif
    
    // create 'System.String' instance
    if(is_lib) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LIB_NEW_OBJ_INST, "System.String"));
    } 
    else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_OBJ_INST, (INT_VALUE)string_cls_id));
    }
    // note: method ID is position dependant
    if(is_lib) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LIB_MTHD_CALL, 0, 
										 "System.String", "System.String:New:c*,"));
    }
    else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, MTHD_CALL, (INT_VALUE)string_cls_id, 2L, 0L)); 
    }
        
  }    
    break;
    
  case instructions::SOCK_TCP_CONNECT:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 1, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::SOCK_TCP_CONNECT));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 4));
    break;

  case instructions::SOCK_TCP_IS_CONNECTED:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::SOCK_TCP_IS_CONNECTED));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;
    
  case instructions::SOCK_TCP_CLOSE:    
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::SOCK_TCP_CLOSE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case instructions::SOCK_TCP_IN_BYTE:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::SOCK_TCP_IN_BYTE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case instructions::SOCK_TCP_IN_BYTE_ARY:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 1, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 2, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::SOCK_TCP_IN_BYTE_ARY));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 5));
    break;

  case instructions::SOCK_TCP_OUT_BYTE:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::SOCK_TCP_OUT_BYTE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 3));
    break;

  case instructions::SOCK_TCP_OUT_BYTE_ARY:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 1, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 2, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::SOCK_TCP_OUT_BYTE_ARY));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 5));
    break;
    
  case instructions::SOCK_TCP_OUT_STRING:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::SOCK_TCP_OUT_STRING));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 3));
    break;

  case instructions::SOCK_TCP_IN_STRING:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::SOCK_TCP_IN_STRING));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 3));
    break;
    
    //----------- file methods -----------
  case instructions::FILE_OPEN_READ:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_OPEN_READ));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 3));
    break;

  case FILE_OPEN_WRITE:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_OPEN_WRITE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 3));
    break;

  case FILE_OPEN_READ_WRITE:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_OPEN_READ_WRITE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 3));
    break;

  case instructions::FILE_IN_BYTE:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_IN_BYTE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case instructions::FILE_IN_BYTE_ARY:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 1, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 2, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_IN_BYTE_ARY));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 5));
    break;

  case instructions::FILE_IN_STRING:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_IN_STRING));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 3));
    break;

  case instructions::FILE_CLOSE:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_CLOSE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case FILE_OUT_BYTE:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_OUT_BYTE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 3));
    break;

  case FILE_OUT_BYTE_ARY:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 1, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 2, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_OUT_BYTE_ARY));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 5));
    break;

  case FILE_OUT_STRING:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_OUT_STRING));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 3));
    break;

  case FILE_SEEK:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_SEEK));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 3));
    break;

  case FILE_EOF:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_EOF));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case FILE_REWIND:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_REWIND));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case FILE_IS_OPEN:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_IS_OPEN));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

    //----------- file functions -----------
  case FILE_EXISTS:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_EXISTS));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case FILE_SIZE:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_SIZE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case FILE_DELETE:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_DELETE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case FILE_RENAME:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 1, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::FILE_RENAME));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 3));
    break;

    //----------- directory functions -----------
  case DIR_CREATE:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::DIR_CREATE));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case DIR_EXISTS:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::DIR_EXISTS));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;

  case DIR_LIST:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::DIR_LIST));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP, 2));
    break;
  }
}

/****************************
 * Translates an 'select' statement
 ****************************/
void IntermediateEmitter::EmitSelect(Select* select_stmt)
{
  cur_line_num = select_stmt->GetLineNumber();
  
  if(select_stmt->GetLabelStatements().size() > 1) {
    SelectArrayTree tree(select_stmt, this);
    tree.Emit();
  } else {
    // get statement and value
    map<int, StatementList*> label_statements = select_stmt->GetLabelStatements();
    map<int, StatementList*>::iterator iter = label_statements.begin();
    int value = iter->first;
    StatementList* statement_list = iter->second;

    // set labels
    int end_label = ++unconditional_label;
    int other_label;
    if(select_stmt->GetOther()) {
      other_label = ++conditional_label;
    }

    // emit code
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, value));
    EmitExpression(select_stmt->GetExpression());
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, EQL_INT));
    if(select_stmt->GetOther()) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, other_label, false));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, end_label, false));
    }

    // label statements
    vector<Statement*> statements = statement_list->GetStatements();
    for(unsigned int i = 0; i < statements.size(); i++) {
      EmitStatement(statements[i]);
    }
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, end_label, -1));
    

    // other statements
    if(select_stmt->GetOther()) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, other_label));
      StatementList* statement_list = select_stmt->GetOther();
      vector<Statement*> statements = statement_list->GetStatements();
      for(unsigned int i = 0; i < statements.size(); i++) {
        EmitStatement(statements[i]);
      }
    }
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, end_label));
  }
}

/****************************
 * Translates a 'while' statement
 ****************************/
void IntermediateEmitter::EmitDoWhile(DoWhile* do_while_stmt)
{
  cur_line_num = do_while_stmt->GetLineNumber();
  
  // conditional expression
  int conditional = ++conditional_label;

  int break_label = ++unconditional_label;
  break_labels.push(break_label);
  
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, conditional));

  // statements
  vector<Statement*> do_while_statements = do_while_stmt->GetStatements()->GetStatements();
  for(unsigned int i = 0; i < do_while_statements.size(); i++) {
    EmitStatement(do_while_statements[i]);
  }
  
  EmitExpression(do_while_stmt->GetExpression());
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, conditional, true));
  
  break_label = break_labels.top();
  break_labels.pop();
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, break_label));
  
}

/****************************
 * Translates a 'while' statement
 ****************************/
void IntermediateEmitter::EmitWhile(While* while_stmt)
{
  cur_line_num = while_stmt->GetLineNumber();
  
  // conditional expression
  int unconditional = ++unconditional_label;
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, unconditional));
  EmitExpression(while_stmt->GetExpression());
  
  int break_label = ++conditional_label;
  break_labels.push(break_label);
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, break_label, false));
  

  // statements
  vector<Statement*> while_statements = while_stmt->GetStatements()->GetStatements();
  for(unsigned int i = 0; i < while_statements.size(); i++) {
    EmitStatement(while_statements[i]);
  }
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, unconditional, -1));
  
  
  break_label = break_labels.top();
  break_labels.pop();  
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, break_label));
}

/****************************
 * Translates an 'for' statement
 ****************************/
void IntermediateEmitter::EmitCriticalSection(CriticalSection* critical_stmt) 
{
  cur_line_num = critical_stmt->GetLineNumber();
  
  StatementList* statement_list = critical_stmt->GetStatements();
  vector<Statement*> statements = statement_list->GetStatements();
  
  EmitVariable(critical_stmt->GetVariable());
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, CRITICAL_START));
  
  for(unsigned int i = 0; i < statements.size(); i++) {
    EmitStatement(statements[i]);
  }

  EmitVariable(critical_stmt->GetVariable());
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, CRITICAL_END));
}

/****************************
 * Translates an 'for' statement
 ****************************/
void IntermediateEmitter::EmitFor(For* for_stmt)
{
  cur_line_num = for_stmt->GetLineNumber();
  
  // pre statement
  EmitStatement(for_stmt->GetPreStatement());

  // conditional expression
  int unconditional = ++unconditional_label;
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, unconditional));
  EmitExpression(for_stmt->GetExpression());
  int break_label = ++conditional_label;
  break_labels.push(break_label);
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, break_label, false));
  

  // statements
  vector<Statement*> for_statements = for_stmt->GetStatements()->GetStatements();
  for(unsigned int i = 0; i < for_statements.size(); i++) {
    EmitStatement(for_statements[i]);
  }

  // update statement
  EmitStatement(for_stmt->GetUpdateStatement());

  // conditional jump
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, unconditional, -1));
  
  
  break_label = break_labels.top();
  break_labels.pop();
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, break_label));
}

/****************************
 * Translates an 'if' statement
 ****************************/
void IntermediateEmitter::EmitIf(If* if_stmt)
{
  cur_line_num = if_stmt->GetLineNumber();
  
  int end_label = ++unconditional_label;
  EmitIf(if_stmt, ++conditional_label, end_label);
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, end_label));
}

void IntermediateEmitter::EmitIf(If* if_stmt, int next_label, int end_label)
{
  if(if_stmt) {
    // expression
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, next_label));
    EmitExpression(if_stmt->GetExpression());

    // if-else
    int conditional = ++conditional_label;
    if(if_stmt->GetNext() || if_stmt->GetElseStatements()) {

      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, conditional, false));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, end_label, false));
    }
    

    // statements
    vector<Statement*> if_statements = if_stmt->GetIfStatements()->GetStatements();
    for(unsigned int i = 0; i < if_statements.size(); i++) {
      EmitStatement(if_statements[i]);
    }
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, end_label, -1));
    

    // if-else
    if(if_stmt->GetNext()) {
      EmitIf(if_stmt->GetNext(), conditional, end_label);

    }
    // else
    if(if_stmt->GetElseStatements()) {
      // label
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, conditional));
      // statements
      vector<Statement*> else_statements = if_stmt->GetElseStatements()->GetStatements();
      for(unsigned int i = 0; i < else_statements.size(); i++) {
        EmitStatement(else_statements[i]);
      }
    }
  }
}

/****************************
 * Translates an expression
 ****************************/
void IntermediateEmitter::EmitExpression(Expression* expression)
{
  cur_line_num = expression->GetLineNumber();
  
  switch(expression->GetExpressionType()) {
  case CHAR_STR_EXPR:
    EmitCharacterString(static_cast<CharacterString*>(expression));
    break;

  case STAT_ARY_EXPR:
    EmitStaticArray(static_cast<StaticArray*>(expression));
    break;
    
  case METHOD_CALL_EXPR: {
    // find end of nested call
    MethodCall* method_call = static_cast<MethodCall*>(expression);
    if(method_call->IsFunctionDefinition()) {
      if(method_call->GetMethod()) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, method_call->GetMethod()->GetId()));
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, method_call->GetMethod()->GetClass()->GetId()));
      }
      else {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, method_call->GetLibraryMethod()->GetId()));
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, method_call->GetLibraryMethod()->GetLibraryClass()->GetId()));
      }
    }
    else if(method_call->IsDynamicFunctionCall()) {
      MethodCall* tail = method_call;
      while(tail->GetMethodCall()) {
	tail = tail->GetMethodCall();
      }
      
      // emit parameters for nested call
      MethodCall* temp = static_cast<MethodCall*>(tail);
      while(temp) {
	EmitMethodCallParameters(temp);
	// update
	temp = static_cast<MethodCall*>(temp->GetPreviousExpression());
      }

      // emit function variable
      MemoryContext mem_context;
      SymbolEntry* entry = method_call->GetDynamicFunctionEntry();
      if(entry->IsLocal()) {
	mem_context = LOCL;
      } 
      else if(entry->IsStatic()) {
	mem_context = CLS;
      } 
      else {
	mem_context = INST;
      }
      //
      if(mem_context == INST) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
      } 
      else if(mem_context == CLS) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
      }      
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FUNC_VAR, entry->GetId(), mem_context));
      
      // emit dynamic call
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));

      switch(OrphanReturn(method_call)) {
      case 0:
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, DYN_MTHD_CALL, entry->GetType()->GetFunctionParameterCount(), instructions::INT_TYPE));
	break;
	
      case 1:
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, DYN_MTHD_CALL, entry->GetType()->GetFunctionParameterCount(), instructions::FLOAT_TYPE));
	break;

      case 2:
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, DYN_MTHD_CALL, entry->GetType()->GetFunctionParameterCount(), instructions::FUNC_TYPE));
	if(!method_call->GetMethodCall()) {
	  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));
	  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, POP_INT));	
	}
	break;
	
      default:
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, DYN_MTHD_CALL, entry->GetType()->GetFunctionParameterCount(), instructions::NIL_TYPE));
	break;
      }
      
      // emit nested calls
      bool is_nested = false; // fuction call
      method_call = method_call->GetMethodCall();
      while(method_call) {
	EmitMethodCall(method_call, is_nested, method_call->GetCastType() != NULL);
	EmitCast(method_call);
	// next call
	if(method_call->GetMethod()) {
	  Method* method = method_call->GetMethod();
	  if(method->GetReturn()->GetType() == CLASS_TYPE) {
	    is_nested = true;
	  } 
	  else {
	    is_nested = false;
	  }
	} 
	else if(method_call->GetLibraryMethod()) {
	  LibraryMethod* lib_method = method_call->GetLibraryMethod();
	  if(lib_method->GetReturn()->GetType() == CLASS_TYPE) {
	    is_nested = true;
	  } 
	  else {
	    is_nested = false;
	  }
	} 
	else {
	  is_nested = false;
	}
	method_call = method_call->GetMethodCall();
      } 





    }
    else {
      MethodCall* tail = method_call;
      while(tail->GetMethodCall()) {
	tail = tail->GetMethodCall();
      }

      // emit parameters for nested call
      MethodCall* temp = static_cast<MethodCall*>(tail);
      while(temp) {
	EmitMethodCallParameters(temp);
	// update
	temp = static_cast<MethodCall*>(temp->GetPreviousExpression());
      }

      bool is_nested = false;
      do {
	EmitMethodCall(method_call, is_nested, method_call->GetCastType() != NULL);
	EmitCast(method_call);
	// next call
	if(method_call->GetMethod()) {
	  Method* method = method_call->GetMethod();
	  if(method->GetReturn()->GetType() == CLASS_TYPE) {
	    is_nested = true;
	  } 
	  else {
	    is_nested = false;
	  }
	} 
	else if(method_call->GetLibraryMethod()) {
	  LibraryMethod* lib_method = method_call->GetLibraryMethod();
	  if(lib_method->GetReturn()->GetType() == CLASS_TYPE) {
	    is_nested = true;
	  } 
	  else {
	    is_nested = false;
	  }
	} 
	else {
	  is_nested = false;
	}
	method_call = method_call->GetMethodCall();
      } 
      while(method_call);
    }
  }
    break;
    
  case BOOLEAN_LIT_EXPR:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, static_cast<BooleanLiteral*>(expression)->GetValue()));
    break;

  case CHAR_LIT_EXPR:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, static_cast<CharacterLiteral*>(expression)->GetValue()));
    EmitCast(expression);
    break;

  case INT_LIT_EXPR:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, static_cast<IntegerLiteral*>(expression)->GetValue()));
    EmitCast(expression);
    break;

  case FLOAT_LIT_EXPR:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_LIT, static_cast<FloatLiteral*>(expression)->GetValue()));
    EmitCast(expression);
    break;

  case NIL_LIT_EXPR:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, 0));
    break;

  case VAR_EXPR:
    EmitVariable(static_cast<Variable*>(expression));
    EmitCast(expression);
    break;

  case AND_EXPR:
  case OR_EXPR:
    EmitAndOr(static_cast<CalculatedExpression*>(expression));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, 0, LOCL));
    break;

  case EQL_EXPR:
  case NEQL_EXPR:
  case LES_EXPR:
  case GTR_EXPR:
  case LES_EQL_EXPR:
  case GTR_EQL_EXPR:
  case ADD_EXPR:
  case SUB_EXPR:
  case MUL_EXPR:
  case DIV_EXPR:
  case MOD_EXPR:
  case SHL_EXPR:
  case SHR_EXPR:
  case BIT_AND_EXPR:
  case BIT_OR_EXPR:
  case BIT_XOR_EXPR:
    EmitCalculation(static_cast<CalculatedExpression*>(expression));
    break;
  }

  // class cast
  if(expression->GetToClass()) {
    if(is_lib) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LIB_OBJ_INST_CAST, expression->GetToClass()->GetName()));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, OBJ_INST_CAST, expression->GetToClass()->GetId()));
    }
  } else if(expression->GetToLibraryClass()) {
    if(is_lib) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LIB_OBJ_INST_CAST, expression->GetToLibraryClass()->GetName()));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, OBJ_INST_CAST, expression->GetToLibraryClass()->GetId()));
    }
  }

  // note: all nested method calls of type METHOD_CALL_EXPR
  // are processed above
  if(expression->GetExpressionType() != METHOD_CALL_EXPR) {
    // method call
    MethodCall* method_call = expression->GetMethodCall();

    // note: this is not clean (drinking bordeaux)... literals are
    // considered expressions and turned into instances of 'System.String'
    // objects are considered nested if a method call is present
    bool is_nested = false || expression->GetExpressionType() == CHAR_STR_EXPR;
    while(method_call) {
      // declarations
      vector<Expression*> expressions = method_call->GetCallingParameters()->GetExpressions();
      for(unsigned int i = 0; i < expressions.size(); i++) {
        EmitExpression(expressions[i]);
      }
      // emit call
      EmitMethodCall(method_call, is_nested, expression->GetCastType() != NULL);
      // next call
      if(method_call->GetMethod()) {
        Method* method = method_call->GetMethod();
        if(method->GetReturn()->GetType() == CLASS_TYPE) {
          is_nested = true;
        } else {
          is_nested = false;
        }
      } else if(method_call->GetLibraryMethod()) {
        LibraryMethod* lib_method = method_call->GetLibraryMethod();
        if(lib_method->GetReturn()->GetType() == CLASS_TYPE) {
          is_nested = true;
        } else {
          is_nested = false;
        }
      } else {
        is_nested = false;
      }
      method_call = method_call->GetMethodCall();
    }
  }
}

/****************************
 * Translates an element array.
 * This creates a new array
 * and copies content.
 ****************************/
void IntermediateEmitter::EmitStaticArray(StaticArray* array) {
  cur_line_num = array->GetLineNumber();
  
  vector<Expression*> all_elements = array->GetAllElements()->GetExpressions();
  if(array->GetType() != frontend::CLASS_TYPE) {
    // write array dimensions
    for(int i = 0; i < array->GetDimension(); i++) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)array->GetSize(i)));
    }
    // write copy instructions
    switch(array->GetType()) {
    case frontend::INT_TYPE:    
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_INT_ARY, (INT_VALUE)array->GetDimension()));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)array->GetId()));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::CPY_INT_STR_ARY));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP_RTRN, 3));
      break;
    
    case frontend::FLOAT_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_FLOAT_ARY, (INT_VALUE)array->GetDimension()));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)array->GetId()));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::CPY_FLOAT_STR_ARY));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP_RTRN, 3));
      break;
    
    case frontend::CHAR_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_BYTE_ARY, (INT_VALUE)array->GetDimension()));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)array->GetId()));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::CPY_CHAR_STR_ARY));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP_RTRN, 3));
      break;
    }
  }
  else {
    // create string literals
    is_str_array = true;
    for(int i = all_elements.size() - 1; i > -1; i--) {
      EmitCharacterString(static_cast<CharacterString*>(all_elements[i]));
    }
    is_str_array = false;
    // write array dimensions
    for(int i = 0; i < array->GetDimension(); i++) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)array->GetSize(i)));
    }
    // create string array
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_INT_ARY, (INT_VALUE)array->GetDimension()));        
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::CPY_CHAR_STR_ARYS));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP_RTRN, (INT_VALUE)(all_elements.size() + 2)));
  }
}

/****************************
 * Translates character string.
 * This creates a new byte array
 * and copies content.
 ****************************/
void IntermediateEmitter::EmitCharacterString(CharacterString* char_str)
{
  cur_line_num = char_str->GetLineNumber();
  
  // copy and create Char[]
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)char_str->GetString().size() + 1));
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_BYTE_ARY, (INT_VALUE)1));
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)char_str->GetId()));
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, (INT_VALUE)instructions::CPY_CHAR_STR_ARY));
  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, TRAP_RTRN, 3));

#ifdef _SYSTEM
  Class* klass = SearchProgramClasses("System.String");
  assert(klass);
  int string_cls_id = klass->GetId();
#else
  LibraryClass* lib_klass = parsed_program->GetLinker()->SearchClassLibraries("System.String");
  assert(lib_klass);
  int string_cls_id = lib_klass->GetId();
#endif

  // create 'System.String' instance
  if(is_lib) {
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LIB_NEW_OBJ_INST, "System.String"));
  } 
  else {
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_OBJ_INST, (INT_VALUE)string_cls_id));
  }
  // note: method ID is position dependant
  if(is_lib) {
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LIB_MTHD_CALL, 0, 
									       "System.String", "System.String:New:c*,"));
  }
  else {
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, MTHD_CALL, (INT_VALUE)string_cls_id, 2L, 0L)); 
  }
  
  // check for stack swap
  new_char_str_count++;
  if(!is_str_array && new_char_str_count >= 2) {
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, SWAP_INT));
  }
}

/****************************
 * Translates a calculation
 ****************************/
void IntermediateEmitter::EmitAndOr(CalculatedExpression* expression)
{
  cur_line_num = expression->GetLineNumber();

  switch(expression->GetExpressionType()) {
  case AND_EXPR: {
    // emit right
    EmitExpression(expression->GetRight());
    int label = ++conditional_label;
    // AND jump
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, label, 1));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, 0));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_INT_VAR, 0, LOCL));
    int end = ++unconditional_label;
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, end, -1));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, label));
    // emit left
    EmitExpression(expression->GetLeft());
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, end));
  }
    break;

  case OR_EXPR: {
    // emit right
    EmitExpression(expression->GetRight());
    int label = ++conditional_label;
    // OR jump
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, label, 0));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, 1));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_INT_VAR, 0, LOCL));
    int end = ++unconditional_label;
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, JMP, end, -1));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, label));
    // emit left
    EmitExpression(expression->GetLeft());
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_INT_VAR, 0, LOCL));
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LBL, end));
  }
    break;
  }
}

/****************************
 * Translates a calculation
 ****************************/
void IntermediateEmitter::EmitCalculation(CalculatedExpression* expression)
{
  cur_line_num = expression->GetLineNumber();
  
  Expression* right = expression->GetRight();
  switch(right->GetExpressionType()) {
  case EQL_EXPR:
  case NEQL_EXPR:
  case LES_EXPR:
  case GTR_EXPR:
  case LES_EQL_EXPR:
  case GTR_EQL_EXPR:
  case ADD_EXPR:
  case SUB_EXPR:
  case MUL_EXPR:
  case DIV_EXPR:
  case MOD_EXPR:
  case SHL_EXPR:
  case SHR_EXPR:
  case BIT_AND_EXPR:
  case BIT_OR_EXPR:
  case BIT_XOR_EXPR:
    EmitCalculation(static_cast<CalculatedExpression*>(right));
    break;

  default:
    EmitExpression(right);
    break;
  }

  Expression* left = expression->GetLeft();
  switch(left->GetExpressionType()) {
  case EQL_EXPR:
  case NEQL_EXPR:
  case LES_EXPR:
  case GTR_EXPR:
  case LES_EQL_EXPR:
  case GTR_EQL_EXPR:
  case ADD_EXPR:
  case SUB_EXPR:
  case MUL_EXPR:
  case DIV_EXPR:
  case MOD_EXPR:
  case SHL_EXPR:
  case SHR_EXPR:
  case BIT_AND_EXPR:
  case BIT_OR_EXPR:
  case BIT_XOR_EXPR:
    EmitCalculation(static_cast<CalculatedExpression*>(left));
    break;

  default:
    EmitExpression(left);
    break;
  }

  EntryType eval_type = expression->GetEvalType()->GetType();
  switch(expression->GetExpressionType()) {
  case EQL_EXPR:
    if(left->GetEvalType()->GetType() == frontend::FLOAT_TYPE ||
       right->GetEvalType()->GetType() == frontend::FLOAT_TYPE) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, EQL_FLOAT));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, EQL_INT));
    }
    EmitCast(expression);
    break;

  case NEQL_EXPR:
    if(left->GetEvalType()->GetType() == frontend::FLOAT_TYPE ||
       right->GetEvalType()->GetType() == frontend::FLOAT_TYPE) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEQL_FLOAT));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEQL_INT));
    }
    EmitCast(expression);
    break;

  case LES_EXPR:
    if(left->GetEvalType()->GetType() == frontend::FLOAT_TYPE ||
       right->GetEvalType()->GetType() == frontend::FLOAT_TYPE) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LES_FLOAT));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LES_INT));
    }
    EmitCast(expression);
    break;

  case GTR_EXPR:
    if(left->GetEvalType()->GetType() == frontend::FLOAT_TYPE ||
       right->GetEvalType()->GetType() == frontend::FLOAT_TYPE) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, GTR_FLOAT));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, GTR_INT));
    }
    EmitCast(expression);
    break;

  case LES_EQL_EXPR:
    if(left->GetEvalType()->GetType() == frontend::FLOAT_TYPE ||
       right->GetEvalType()->GetType() == frontend::FLOAT_TYPE) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LES_EQL_FLOAT));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LES_EQL_INT));
    }
    EmitCast(expression);
    break;

  case GTR_EQL_EXPR:
    if(left->GetEvalType()->GetType() == frontend::FLOAT_TYPE ||
       right->GetEvalType()->GetType() == frontend::FLOAT_TYPE) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, GTR_EQL_FLOAT));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, GTR_EQL_INT));
    }
    EmitCast(expression);
    break;

  case ADD_EXPR:
    if(eval_type == frontend::FLOAT_TYPE) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, ADD_FLOAT));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, ADD_INT));
    }
    EmitCast(expression);
    break;

  case SUB_EXPR:
    if(eval_type == frontend::FLOAT_TYPE) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, SUB_FLOAT));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, SUB_INT));
    }
    EmitCast(expression);
    break;

  case MUL_EXPR:
    if(eval_type == frontend::FLOAT_TYPE) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, MUL_FLOAT));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, MUL_INT));
    }
    EmitCast(expression);
    break;

  case DIV_EXPR:
    if(eval_type == frontend::FLOAT_TYPE) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, DIV_FLOAT));
    } else {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, DIV_INT));
    }
    EmitCast(expression);
    break;

  case MOD_EXPR:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, MOD_INT));
    EmitCast(expression);
    break;

  case SHL_EXPR:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, SHL_INT));
    EmitCast(expression);
    break;
    
  case SHR_EXPR:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, SHR_INT));
    EmitCast(expression);
    break;
    
  case BIT_AND_EXPR:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, BIT_AND_INT));
    EmitCast(expression);
    break;
    
  case BIT_OR_EXPR:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, BIT_OR_INT));
    EmitCast(expression);
    break;
    
  case BIT_XOR_EXPR:
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, BIT_XOR_INT));
    EmitCast(expression);
    break;
  }
}

/****************************
 * Translates a type cast
 ****************************/
void IntermediateEmitter::EmitCast(Expression* expression)
{
  cur_line_num = expression->GetLineNumber();

  frontend::Type* cast_type = expression->GetCastType();
  if(cast_type) {
    Type* base_type;
    if(expression->GetExpressionType() == METHOD_CALL_EXPR) {
      MethodCall* call = static_cast<MethodCall*>(expression);
      if(call->GetMethod()) {
        base_type = call->GetMethod()->GetReturn();
      } else if(call->GetLibraryMethod()) {
        base_type = call->GetLibraryMethod()->GetReturn();
      } else {
        base_type = expression->GetBaseType();
      }
    } else {
      base_type = expression->GetBaseType();
    }

    switch(base_type->GetType()) {
    case frontend::BYTE_TYPE:
    case frontend::CHAR_TYPE:
    case frontend::INT_TYPE:
      if(cast_type->GetType() == frontend::FLOAT_TYPE) {
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, I2F));
      }
      break;

    case frontend::FLOAT_TYPE:
      if(cast_type->GetType() != frontend::FLOAT_TYPE) {
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, F2I));
      }
      break;
    }
  }
}

/****************************
 * Translates variable
 ****************************/
void IntermediateEmitter::EmitVariable(Variable* variable)
{
  cur_line_num = variable->GetLineNumber();
  
  // self
  if(variable->GetEntry()->IsSelf()) {
    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    return;
  }
  
  // indices
  ExpressionList* indices = variable->GetIndices();

  // memory context
  MemoryContext mem_context;
  if(variable->GetEntry()->IsLocal()) {
    mem_context = LOCL;
  } else if(variable->GetEntry()->IsStatic()) {
    mem_context = CLS;
  } else {
    mem_context = INST;
  }

  // array variable
  if(indices) {
    int dimension = (int)indices->GetExpressions().size();
    EmitIndices(indices);

    // load instance or class memory
    if(mem_context == INST) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    } else if(mem_context == CLS) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
    }

    switch(variable->GetBaseType()->GetType()) {
    case frontend::BYTE_TYPE:
    case frontend::CHAR_TYPE:
    case frontend::BOOLEAN_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, variable->GetId(), mem_context));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_BYTE_ARY_ELM, dimension, mem_context));
      break;

    case frontend::INT_TYPE:
    case frontend::CLASS_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, variable->GetId(), mem_context));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_ARY_ELM, dimension, mem_context));
      break;

    case frontend::FLOAT_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, variable->GetId(), mem_context));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_ARY_ELM, dimension, mem_context));
      break;
    }
  }
  // scalar variable
  else {
    // load instance or class memory
    if(mem_context == INST) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    } else if(mem_context == CLS) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
    }

    switch(variable->GetBaseType()->GetType()) {
    case frontend::BOOLEAN_TYPE:
    case frontend::BYTE_TYPE:
    case frontend::CHAR_TYPE:
    case frontend::INT_TYPE:
    case frontend::CLASS_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, variable->GetId(), mem_context));
      break;

    case frontend::FLOAT_TYPE:
      if(variable->GetEntry()->GetType()->GetDimension() > 0) {
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, variable->GetId(), mem_context));
      } else {
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_VAR, variable->GetId(), mem_context));
      }
      break;
      
    case frontend::FUNC_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FUNC_VAR, variable->GetId(), mem_context));
      break;
    }
  }
}

/****************************
 * Translates array indices
 ****************************/
void IntermediateEmitter::EmitIndices(ExpressionList* indices)
{
  EmitExpressions(indices);
}

/****************************
 * Translates an assignment
 * statement
 ****************************/
void IntermediateEmitter::EmitAssignment(Assignment* assignment)
{
  cur_line_num = assignment->GetLineNumber();

  // expression
  EmitExpression(assignment->GetExpression());

  // assignment
  Variable* variable = assignment->GetVariable();
  ExpressionList* indices = variable->GetIndices();
  MemoryContext mem_context;

  // memory context
  if(variable->GetEntry()->IsLocal()) {
    mem_context = LOCL;
  } 
  else if(variable->GetEntry()->IsStatic()) {
    mem_context = CLS;
  } 
  else {
    mem_context = INST;
  }

  // array variable
  if(indices) {
    int dimension = (int)indices->GetExpressions().size();
    EmitIndices(indices);

    // load instance or class memory
    if(mem_context == INST) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    } else if(mem_context == CLS) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
    }

    switch(variable->GetBaseType()->GetType()) {
    case frontend::BYTE_TYPE:
    case frontend::BOOLEAN_TYPE:
    case frontend::CHAR_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, variable->GetId(), mem_context));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_BYTE_ARY_ELM, dimension, mem_context));
      break;

    case frontend::INT_TYPE:
    case frontend::CLASS_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, variable->GetId(), mem_context));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_INT_ARY_ELM, dimension, mem_context));
      break;

    case frontend::FLOAT_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, variable->GetId(), mem_context));
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_FLOAT_ARY_ELM, dimension, mem_context));
      break;
    }
  }
  // scalar variable
  else {
    // load instance or class memory
    if(mem_context == INST) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
    } 
    else if(mem_context == CLS) {
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
    }
    
    // operation assignment
    EntryType eval_type = variable->GetEvalType()->GetType();      
    switch(assignment->GetStatementType()) {
    case ADD_ASSIGN_STMT:
      EmitOperatorVariable(variable, mem_context);
      if(eval_type == frontend::FLOAT_TYPE) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, ADD_FLOAT));
      } 
      else {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, ADD_INT));
      }
      // load instance or class memory
      if(mem_context == INST) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
      } 
      else if(mem_context == CLS) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
      }
      break;
      
    case SUB_ASSIGN_STMT:
      EmitOperatorVariable(variable, mem_context);
      if(eval_type == frontend::FLOAT_TYPE) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, SUB_FLOAT));
      } 
      else {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, SUB_INT));
      }
      // load instance or class memory
      if(mem_context == INST) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
      } 
      else if(mem_context == CLS) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
      }
      break;
      
    case MUL_ASSIGN_STMT:
      EmitOperatorVariable(variable, mem_context);
      if(eval_type == frontend::FLOAT_TYPE) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, MUL_FLOAT));
      } 
      else {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, MUL_INT));
      }      
      // load instance or class memory
      if(mem_context == INST) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
      } 
      else if(mem_context == CLS) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
      }
      break;
      
    case DIV_ASSIGN_STMT:
      EmitOperatorVariable(variable, mem_context);
      if(eval_type == frontend::FLOAT_TYPE) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, DIV_FLOAT));
      } 
      else {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, DIV_INT));
      }
      // load instance or class memory
      if(mem_context == INST) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
      } 
      else if(mem_context == CLS) {
	imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
      }
      break;
    }

    switch(variable->GetBaseType()->GetType()) {
    case frontend::BOOLEAN_TYPE:
    case frontend::BYTE_TYPE:
    case frontend::CHAR_TYPE:
    case frontend::INT_TYPE:
    case frontend::CLASS_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_INT_VAR, variable->GetId(), mem_context));
      break;

    case frontend::FLOAT_TYPE:
      if(variable->GetEntry()->GetType()->GetDimension() > 0) {
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_INT_VAR, variable->GetId(), mem_context));
      } 
      else {
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_FLOAT_VAR, variable->GetId(), mem_context));
      }
      break;

    case frontend::FUNC_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, STOR_FUNC_VAR, variable->GetId(), mem_context));
      break;
    }
  }
  new_char_str_count = 0;
}

/****************************
 * Translates a declaration
 ****************************/
void IntermediateEmitter::EmitDeclaration(Declaration* declaration)
{
  Statement* statement = declaration->GetAssignment();
  if(statement) {
    EmitStatement(statement);
  }
}

/****************************
 * Translates a method call
 * parameters
 ****************************/
void IntermediateEmitter::EmitMethodCallParameters(MethodCall* method_call)
{
  cur_line_num = static_cast<Statement*>(method_call)->GetLineNumber();

  // new array
  if(method_call->GetCallType() == NEW_ARRAY_CALL) {
    vector<Expression*> expressions = method_call->GetCallingParameters()->GetExpressions();
    for(unsigned int i = 0; i < expressions.size(); i++) {
      EmitExpression(expressions[i]);
    }
    is_new_inst = false;
  }
  // enum call
  else if(method_call->GetCallType() == ENUM_CALL) {
    if(method_call->GetEnumItem()) {
      INT_VALUE value = method_call->GetEnumItem()->GetId();
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, value));
    } else {
      INT_VALUE value = method_call->GetLibraryEnumItem()->GetId();
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_LIT, value));
    }
    is_new_inst = false;
  }
  // instance
  else if(method_call->GetCallType() == NEW_INST_CALL) {
    // declarations
    vector<Expression*> expressions = method_call->GetCallingParameters()->GetExpressions();
    for(unsigned int i = 0; i < expressions.size(); i++) {
      EmitExpression(expressions[i]);
      new_char_str_count = 0;
    }

    // new object instance
    Method* method = method_call->GetMethod();
    if(method) {
      if(is_lib) {
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LIB_NEW_OBJ_INST, method->GetClass()->GetName()));
      } else {
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_OBJ_INST, method->GetClass()->GetId()));
      }
    } else {
      LibraryMethod* lib_method = method_call->GetLibraryMethod();

      if(is_lib) {
        const string& klass_name = lib_method->GetLibraryClass()->GetName();
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LIB_NEW_OBJ_INST, klass_name));
      } else {
        int klass_id = lib_method->GetLibraryClass()->GetId();
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_OBJ_INST, klass_id));
      }
    }
    is_new_inst = true;
  }
  // method call
  else {
    // declarations
    if(method_call->GetCallingParameters()) {
      vector<Expression*> expressions = method_call->GetCallingParameters()->GetExpressions();
      for(unsigned int i = 0; i < expressions.size(); i++) {
        EmitExpression(expressions[i]);
	new_char_str_count = 0;
      }
    }
    is_new_inst = false;
  }
  // new_char_str_count = 0;
}

/****************************
 * Translates a method call
 ****************************/
void IntermediateEmitter::EmitMethodCall(MethodCall* method_call, bool is_nested, bool is_cast)
{
  cur_line_num = static_cast<Statement*>(method_call)->GetLineNumber();
  
  // new array
  if(method_call->GetCallType() == NEW_ARRAY_CALL) {
    vector<Expression*> expressions = method_call->GetCallingParameters()->GetExpressions();
    switch(method_call->GetArrayType()->GetType()) {
    case frontend::BYTE_TYPE:
    case frontend::BOOLEAN_TYPE:
    case frontend::CHAR_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_BYTE_ARY, (INT_VALUE)expressions.size()));
      break;

    case frontend::CLASS_TYPE:
    case frontend::INT_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_INT_ARY, (INT_VALUE)expressions.size()));
      break;

    case frontend::FLOAT_TYPE:
      imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, NEW_FLOAT_ARY,	(INT_VALUE)expressions.size()));
      break;
    }
  } 
  else {
    // literal and variable method calls
    Variable* variable = method_call->GetVariable();
    SymbolEntry* entry = method_call->GetEntry();
    
    if(variable && method_call->GetCallType() == METHOD_CALL) {
      EmitVariable(variable);
    }
    else if(entry) {
      // memory context
      MemoryContext mem_context;
      if(entry->IsLocal()) {
        mem_context = LOCL;
      } 
      else if(entry->IsStatic()) {
        mem_context = CLS;
      } 
      else {
        mem_context = INST;
      }
      
      if(entry->GetType()->GetDimension() > 0 && entry->GetType()->GetType() == CLASS_TYPE) {
        // load instance or class memory
        if(mem_context == INST) {
          imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
        } else if(mem_context == CLS) {
          imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
        }
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, entry->GetId(), mem_context));
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
      } 
      else if(!entry->IsSelf()) {
        switch(entry->GetType()->GetType()) {
        case frontend::BOOLEAN_TYPE:
        case frontend::BYTE_TYPE:
        case frontend::CHAR_TYPE:
        case frontend::INT_TYPE:
        case frontend::CLASS_TYPE:
          // load instance or class memory
          if(mem_context == INST) {
            imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
          } else if(mem_context == CLS) {
            imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
          }
          imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, entry->GetId(), mem_context));
          break;

        case frontend::FLOAT_TYPE:
          // load instance or class memory
          if(mem_context == INST) {
            imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
          } 
	  else if(mem_context == CLS) {
            imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
          }

          if(entry->GetType()->GetDimension() > 0) {
            imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INT_VAR, entry->GetId(), mem_context));
          } 
	  else {
            imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FLOAT_VAR, entry->GetId(), mem_context));
          }
          break;
	  
	case frontend::FUNC_TYPE:
          // load instance or class memory
          if(mem_context == INST) {
            imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
          } else if(mem_context == CLS) {
            imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_CLS_MEM));
          }
          imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_FUNC_VAR, entry->GetId(), mem_context));
          break;
        }
      } else {
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
      }
    }
    
    // program method call
    if(method_call->GetMethod()) {
      Method* method = method_call->GetMethod();
      if(method_call->GetCallType() == PARENT_CALL ||
	 (method->GetMethodType() != NEW_PUBLIC_METHOD &&
	  method->GetMethodType() != NEW_PRIVATE_METHOD) ||
	 current_method == method) {
        if(entry) {
          switch(entry->GetType()->GetType()) {
          case frontend::BOOLEAN_TYPE:
          case frontend::BYTE_TYPE:
          case frontend::CHAR_TYPE:
          case frontend::INT_TYPE:
          case frontend::FLOAT_TYPE:
	  case frontend::FUNC_TYPE:
            imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
            break;
          }
	  // enum check
	  if(entry->GetType()->GetType() == frontend::CLASS_TYPE && 
	     SearchProgramEnums(entry->GetType()->GetClassName())) {
	    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
	  }
        }
        // TODO: this needs to be looked at... simpiler?
        else if(!is_cast && !is_nested && (!variable || !variable->GetIndices() || variable->GetEntry()->GetType()->GetType() != CLASS_TYPE)) {
          imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
        }
	else if(method_call->IsEnumCall()) {
	  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
	}
      } 
      else if((current_method->GetMethodType() == NEW_PUBLIC_METHOD || current_method->GetMethodType() == NEW_PRIVATE_METHOD) && (method->GetMethodType() == NEW_PUBLIC_METHOD || method->GetMethodType() == NEW_PRIVATE_METHOD) && !is_new_inst) {       
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
      }
    }
    // library method call
    else if(method_call->GetLibraryMethod()) {
      LibraryMethod* lib_method = method_call->GetLibraryMethod();
      if(lib_method->GetMethodType() != NEW_PUBLIC_METHOD &&
	 lib_method->GetMethodType() != NEW_PRIVATE_METHOD) {
        if(entry) {
          switch(entry->GetType()->GetType()) {
          case frontend::BOOLEAN_TYPE:
          case frontend::BYTE_TYPE:
          case frontend::CHAR_TYPE:
          case frontend::INT_TYPE:
          case frontend::FLOAT_TYPE:
	  case frontend::FUNC_TYPE:
            imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
            break;
          }
	  // enum check
	  if(entry->GetType()->GetType() == frontend::CLASS_TYPE && 
	     parsed_program->GetLinker()->SearchEnumLibraries(entry->GetType()->GetClassName(), parsed_program->GetUses())) {
	    imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
	  }
        }
        // TODO: this needs to be looked at... simpiler?
        else if(!is_cast && !is_nested && (!variable || !variable->GetIndices() ||
                               variable->GetEntry()->GetType()->GetType() != CLASS_TYPE)) {
          imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
        }
	else if(method_call->IsEnumCall()) {
	  imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
	}
      } 
      else if((current_method->GetMethodType() == NEW_PUBLIC_METHOD || current_method->GetMethodType() == NEW_PRIVATE_METHOD) && (lib_method->GetMethodType() == NEW_PUBLIC_METHOD || lib_method->GetMethodType() == NEW_PRIVATE_METHOD) && !is_new_inst) {        
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LOAD_INST_MEM));
      }
    }    
    
    // program method call
    if(method_call->GetMethod()) {
      Method* method = method_call->GetMethod();
      if(is_lib) {
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LIB_MTHD_CALL, method->IsNative(),
										   method->GetClass()->GetName(),
										   method->GetEncodedName()));
      } else {
        int klass_id = method->GetClass()->GetId();
        int method_id = method->GetId();
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, MTHD_CALL, klass_id, method_id, method->IsNative()));
      }
    }
    // library method call
    else if(method_call->GetLibraryMethod()) {
      LibraryMethod* lib_method = method_call->GetLibraryMethod();
      if(is_lib) {
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, LIB_MTHD_CALL, lib_method->IsNative(),
										   lib_method->GetLibraryClass()->GetName(),
										   lib_method->GetName()));
      } else {
        int klass_id = lib_method->GetLibraryClass()->GetId();
        int method_id = lib_method->GetId();
        imm_block->AddInstruction(IntermediateFactory::Instance()->MakeInstruction(cur_line_num, MTHD_CALL, klass_id, method_id, lib_method->IsNative()));
      }
    }
  }  
            
  new_char_str_count = 0;
  is_new_inst = false;
}

/****************************
 * Emits expressions
 ****************************/
void IntermediateEmitter::EmitExpressions(ExpressionList* declarations)
{
  vector<Expression*> expressions = declarations->GetExpressions();
  for(unsigned int i = 0; i < expressions.size(); i++) {
    EmitExpression(expressions[i]);
  }
}

/****************************
 * Calculates the memory space
 * needed for a method or class
 ****************************/
int IntermediateEmitter::CalculateEntrySpace(SymbolTable* table, int &index,
					     IntermediateDeclarations* declarations,
					     bool is_static)
{
  if(table) {
    int var_space = 0;
    vector<SymbolEntry*> entries = table->GetEntries();
    for(unsigned int i = 0; i < entries.size(); i++) {
      SymbolEntry* entry = entries[i];
      if(!entry->IsSelf() && entry->IsStatic() == is_static) {
        switch(entry->GetType()->GetType()) {
        case frontend::BOOLEAN_TYPE:
          if(entry->GetType()->GetDimension() > 0) {
#ifdef _DEBUG
            cout << "\t" << index << ": INT_ARY_PARM: name=" << entry->GetName()
                 << ", dim=" << entry->GetType()->GetDimension() << endl;
#endif
            declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), INT_ARY_PARM));
          } 
	  else {
#ifdef _DEBUG
            cout << "\t" << index << ": INT_PARM: name=" << entry->GetName() << endl;
#endif
            declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), INT_PARM));
          }
          entry->SetId(index++);
          var_space++;
          break;

        case frontend::BYTE_TYPE:
          if(entry->GetType()->GetDimension() > 0) {
#ifdef _DEBUG
            cout << "\t" << index << ": BYTE_ARY_PARM: name=" << entry->GetName() << endl;
#endif
            declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), BYTE_ARY_PARM));
          } 
	  else {
#ifdef _DEBUG
            cout << "\t" << index << ": INT_PARM: name=" << entry->GetName() << endl;
#endif
            declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), INT_PARM));
          }
          entry->SetId(index++);
          var_space++;
          break;

        case frontend::INT_TYPE:
          if(entry->GetType()->GetDimension() > 0) {
#ifdef _DEBUG
            cout << "\t" << index << ": INT_ARY_PARM: name=" << entry->GetName()
                 << ", dim=" << entry->GetType()->GetDimension() << endl;
#endif
            declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), INT_ARY_PARM));
          } 
	  else {
#ifdef _DEBUG
            cout << "\t" << index << ": INT_PARM: name=" << entry->GetName() << endl;
#endif
            declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), INT_PARM));
          }
          entry->SetId(index++);
          var_space++;
          break;

        case frontend::CHAR_TYPE:
          if(entry->GetType()->GetDimension() > 0) {
#ifdef _DEBUG
            cout << "\t" << index << ": BYTE_ARY_PARM: name=" << entry->GetName() << endl;
#endif
            declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), BYTE_ARY_PARM));
          } else {
#ifdef _DEBUG
            cout << "\t" << index << ": INT_PARM: name=" << entry->GetName() << endl;
#endif
            declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), INT_PARM));
          }
          entry->SetId(index++);
          var_space++;
          break;

        case frontend::CLASS_TYPE:
          // object array
          if(entry->GetType()->GetDimension() > 0) {
            if(parsed_program->GetClass(entry->GetType()->GetClassName())) {
#ifdef _DEBUG
              cout << "\t" << index << ": OBJ_ARY_PARM: name=" << entry->GetName() << endl;
#endif
              declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), OBJ_ARY_PARM, parsed_program->GetClass(entry->GetType()->GetClassName())->GetId()));
            } 
	    else if(SearchProgramEnums(entry->GetType()->GetClassName())) {
#ifdef _DEBUG
              cout << "\t" << index << ": INT_ARY_PARM: name=" << entry->GetName() << endl;
#endif
              declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), INT_ARY_PARM));
            } 
	    else if(parsed_program->GetLinker()->SearchEnumLibraries(entry->GetType()->GetClassName(), parsed_program->GetUses())) {
#ifdef _DEBUG
              cout << "\t" << index << ": INT_ARY_PARM: name=" << entry->GetName() << endl;
#endif
              declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), INT_ARY_PARM));
            } 
	    else {
#ifdef _DEBUG
              cout << "\t" << index << ": OBJ_ARY_PARM: name=" << entry->GetName() << endl;
#endif
              declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), OBJ_ARY_PARM, parsed_program->GetLinker()->SearchClassLibraries(entry->GetType()->GetClassName(), parsed_program->GetUses())->GetId()));
            }
          }
          // object
          else {
            if(SearchProgramClasses(entry->GetType()->GetClassName())) {
#ifdef _DEBUG
              cout << "\t" << index << ": OBJ_PARM: name=" << entry->GetName() << endl;
#endif
              declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), OBJ_PARM, SearchProgramClasses(entry->GetType()->GetClassName())->GetId()));
            } 
	    else if(SearchProgramEnums(entry->GetType()->GetClassName())) {
#ifdef _DEBUG
              cout << "\t" << index << ": INT_PARM: name=" << entry->GetName() << endl;
#endif
              declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), INT_PARM));
            } 
	    else if(parsed_program->GetLinker()->SearchEnumLibraries(entry->GetType()->GetClassName(), parsed_program->GetUses())) {
#ifdef _DEBUG
              cout << "\t" << index << ": INT_PARM: name=" << entry->GetName() << endl;
#endif
              declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), INT_PARM));
            } 
	    else {
#ifdef _DEBUG
              cout << "\t" << index << ": OBJ_PARM: name=" << entry->GetName() << endl;
#endif
              declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), OBJ_PARM, parsed_program->GetLinker()->SearchClassLibraries(entry->GetType()->GetClassName(), parsed_program->GetUses())->GetId()));
            }
          }
          entry->SetId(index++);
          var_space++;
          break;

        case frontend::FLOAT_TYPE:
          if(entry->GetType()->GetDimension() > 0) {
#ifdef _DEBUG
            cout << "\t" << index << ": FLOAT_ARY_PARM: name=" << entry->GetName() << endl;
#endif
            declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), FLOAT_ARY_PARM));
            entry->SetId(index++);
            var_space++;
          } 
	  else {
#ifdef _DEBUG
            cout << "\t" << index << ": FLOAT_PARM: name=" << entry->GetName() << endl;
#endif
            declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), FLOAT_PARM));
            entry->SetId(index);
            index += 2;
            var_space += 2;
          }
          break;
	  
	case frontend::FUNC_TYPE:          
#ifdef _DEBUG
	  cout << "\t" << index << ": FUNC_PARM: name=" << entry->GetName() << endl;
#endif
	  declarations->AddParameter(new IntermediateDeclaration(entry->GetName(), FUNC_PARM));
          entry->SetId(index);
	  index += 2;
	  var_space += 2;
	  break;
	}
      }
    }
    
    return var_space * sizeof(INT_VALUE);
  }

  return 0;
}

/****************************
 * Calculates the memory space
 * needed for a method or class
 ****************************/
int IntermediateEmitter::CalculateEntrySpace(IntermediateDeclarations* declarations, bool is_static)
{
  int size = 0;
  int index = 0;

  // class
  if(!current_method) {
    SymbolTableManager* symbol_table = parsed_bundle->GetSymbolTableManager();

    // inspect parent classes
    Class* parent = current_class->GetParent();
    if(parent) {
      while(parent) {
        SymbolTable* table = symbol_table->GetSymbolTable(parent->GetName());
        size += CalculateEntrySpace(table, index, declarations, is_static);
        parent = SearchProgramClasses(parent->GetParentName());
      }
    } 
    else {
      // inspect parent library
      LibraryClass* lib_parent = current_class->GetLibraryParent();
      if(lib_parent) {
        if(is_static) {
          size += lib_parent->GetClassSpace();
        } else {
          size += lib_parent->GetInstanceSpace();
        }
      }
    }
    // calculate current space
    size += CalculateEntrySpace(current_table, index, declarations, is_static);
  }
  // method
  else {
    if(current_method->HasAndOr()) {
      size = index = 1;
    }

    size = CalculateEntrySpace(current_table, index, declarations, false);
  }

  return size;
}
