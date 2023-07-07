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

#define DEFAULT_FILE_NAME L"shell://repl.obs"

class CodeFormatter;

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
    RW_FUNC_END_LINE
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
  std::wstring name;
  std::list<Line> lines;
  size_t shell_count;

 public:
   Document(std::wstring n) {
     name = n;
     shell_count = 0;
   }

   ~Document() {
   }

   size_t Size() {
     return lines.size();
   }

   std::wstring GetName() {
     return name;
   }

   void SetName(const std::wstring &n) {
     name = n;
   }

   bool Save();

   size_t Reset();
   bool LoadFile(const std::wstring &file);
   std::wstring ToString();
   void List(size_t cur_pos, bool all);
   bool InsertLine(size_t line_num, const std::wstring line, Line::Type = Line::Type::RW_LINE);
   bool DeleteLine(size_t line_num);
#ifdef _DEBUG
   void Debug(size_t cur_pos);
#endif
};

//
// Editor
//
class Editor {
  Document doc;
  std::wstring lib_uses;
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
  void DoUseLibraries(std::wstring &in);
  void DoInsertLine(std::wstring &in);
  bool DoLoadFile(std::wstring &in);
  bool DoSaveFile(std::wstring& in);
  void DoInsertMultiLine(std::wstring &in);
  bool DoDeleteLine(std::wstring& in);
  bool DoReplaceLine(std::wstring& in);
  void DoGotoLine(std::wstring& in);

  // utility functions
  bool AppendLine(std::wstring line);
  
  static inline void LeftTrim(std::wstring& str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [] (wchar_t ch) {
      return !std::isspace(ch);     
    }));
  }

  static inline void RightTrim(std::wstring& str) {
    str.erase(std::find_if(str.rbegin(), str.rend(), [] (wchar_t ch) {
      return !std::isspace(ch);
    }).base(), str.end());
  }

  static inline std::wstring& Trim(std::wstring& str) {
    LeftTrim(str); 
    RightTrim(str);
    return str;
  }

  bool EndsWith(const std::wstring& str, std::wstring const& ending) {
    if(str.length() >= ending.length()) {
      return str.compare(str.length() - ending.length(), ending.length(), ending) == 0;
    }

    return false;
  }
};

/*
Code formatter
*/
class CodeFormatter {
  Token* token; Token* prev_token; Token* next_token;
  bool skip_space;

  bool stmt_space;
  long ident_space;

  std::wstring buffer;
  long start_range;
  long end_range;

  bool trim_trailing;

  std::wstring ASCII_RED;
  std::wstring ASCII_BLUE_HIGH;
  std::wstring ASCII_CYAN;
  std::wstring ASCII_GREEN;
  std::wstring ASCII_GREY;
  std::wstring ASCII_END;

  void AppendBuffer(std::wstring str) {
      if(!buffer.empty() && (buffer.back() == L' ' && (buffer.at(buffer.size() - 2) == L'}' || buffer.at(buffer.size() - 2) == L';'))) {
        buffer.pop_back();
      }

      buffer += str;
    
  } 

  void AppendBuffer(wchar_t str) {
    if(!buffer.empty() && (buffer.back() == L' ' && (buffer.at(buffer.size() - 2) == L'}' || buffer.at(buffer.size() - 2) == L';'))) {
      buffer.pop_back();
    }

    buffer += str;
  }

  void PopBuffer() {
    buffer.pop_back();
  }

  bool InRange()  {
    return (start_range < 0 && end_range < 0) || (token != nullptr && start_range <= token->GetLineNumber() && end_range >= token->GetLineNumber());
  }
  
  void VerticalSpace(Token* prev_token, long tab_space, long ident_space) {
    if(prev_token && prev_token->GetType() == TOKEN_OPEN_BRACE) {
      AppendBuffer(L"\n\n");
      TabSpace(tab_space, ident_space);
      skip_space = true;
    }
  }

  void TabSpace(long tab_space, long) {
    if(tab_space > 0) {
      for(long i = 0; i < tab_space; ++i) {
        for(long j = 0; j < ident_space; ++j) {
          AppendBuffer(' ');
        };
        AppendBuffer(' ');
      };
    }
  }

public:
  CodeFormatter(std::map<std::wstring, std::wstring>& options);
  ~CodeFormatter();

  std::wstring Format(std::wstring source, bool is_color);
};

#endif