/***************************************************************************
 * REPL editor
 *
 * Copyright (c) 2023, Randy Hollines
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduceC the above copyright
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
 ***************************************************************************/

#ifndef __EDITOR_H__
#define __EDITOR_H__

#include "repl.h"
#include "../module/lang.h"

//
// Line
//
class Line {
public:
  enum Type {
    // read-only lines
    RO_LINE,
    RO_CLS_START_LINE,
    RO_CLS_END_LINE,
    RO_FUNC_START_LINE,
    RO_FUNC_END_LINE,
    // read/write line
    RW_LINE,
    RW_CLS_START_LINE,
    RW_CLS_END_LINE,
    RW_FUNC_START_LINE,
    RW_FUNC_END_LINE,
  };

private:
  std::wstring line;
  Line::Type type;

public:
  Line(const Line &l)
  {
    line = l.line;
    type = l.type;
  }

  Line(const std::wstring &l, Line::Type t)
  {
    line = l;
    type = t;
  }

  ~Line() {
  }

  const std::wstring ToString() {
    return line;
  }

  const Line::Type GetType() {
    return type;
  }
};

//
// Document
//
class Document {
  std::list<Line> lines;

 public:
   Document() {
   }

   ~Document() {
   }

   size_t Size() {
     return lines.size();
   }

   size_t Reset();
   std::wstring ToString();

   void List(size_t cur_pos, bool all);

   bool InsertLine(size_t line_num, const std::wstring line, int padding);
   bool DeleteLine(size_t line_num);

   size_t InsertFunction(const std::wstring line);
   size_t DeleteFunction(const std::wstring line);
};

//
// Editor
//
class Editor {
  Document doc;
  size_t cur_pos;

public:
  Editor();

  ~Editor() {
  }

  // start REPL loop
  void Edit();

  // commands
  void DoReset();
  void DoHelp();
  void DoExecute();
  void DoInsertLine(std::wstring in);
  void DoInsertMultiLine(std::wstring in);
  void DoDeleteLine(std::wstring& in);
  void DoInsertFunction(std::wstring in);
  void DoGotoLine(std::wstring& in);
  void DoReplaceLine(std::wstring& in);

  bool AppendFunction(std::wstring line);

  // utility functions
  bool AppendLine(std::wstring line, const int padding);
  
  static inline void LeftTrim(std::wstring& str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
      return !std::isspace(ch);     
    }));
  }

  static inline void RightTrim(std::wstring& str) {
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
      return !std::isspace(ch);
    }).base(), str.end());
  }

  static inline void Trim(std::wstring& str) {
    RightTrim(str);
    LeftTrim(str);
  }
};

#endif