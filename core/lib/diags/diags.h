/***************************************************************************
 * Diagnostics support for Objeck
 *
 * Copyright (c) 2021, Randy Hollines
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#ifndef __DIAG_H__
#define __DIAG_H__

#ifdef _WIN32
#include <windows.h>
#endif

#include "../../vm/lib_api.h"
#include "../../shared/sys.h"
#include "../../../compiler/tree.h"

// severity
#define DIAG_ERROR 101
#define DIAG_WARN 102
#define DIAG_INFO 103
// symbols
#define DIAG_FILE 201
#define DIAG_NAMESPACE 203
#define DIAG_CLASS 205
#define DIAG_METHOD 206
// other
#define DIAG_UNKN 0

extern "C" {
  size_t* FormatErrors(VMContext& context, vector<wstring> error_strings);

  frontend::Method* FindMethod(const int line_num, frontend::ParsedProgram* program, frontend::SymbolTable* &table);
  frontend::Expression* SearchMethod(const int line_num, const int line_pos, frontend::Method* method);
  frontend::Expression* SearchStatements(const int line_num, const int line_pos, vector<frontend::Statement*> statements);

  frontend::Expression* SearchWhile(const int line_num, const int line_pos, frontend::While* while_stmt);
  frontend::Expression* SearchAssignment(const int line_num, const int line_pos, frontend::Assignment* asgn_stmt);
}

#endif
