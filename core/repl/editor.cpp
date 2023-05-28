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
 * - Redistributions in binary form must reproduce the above copyright
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

#include "editor.h"

//
// Line
//
const std::wstring Line::ToString() {
  return line;
}

const Line::Type Line::GetType() {
  return type;
}

//
// Document
//
Document::Document()
{
}

size_t Document::Lines()
{
  return lines.size();
}

size_t Document::Reset()
{
  lines.clear();

  lines.push_back(Line(L"class Shell {", Line::Type::READ_ONLY));
  lines.push_back(Line(L"  function : Main(args : String[]) ~ Nil {", Line::Type::READ_ONLY));
  lines.push_back(Line(L"  }", Line::Type::READ_ONLY));
  lines.push_back(Line(L"}", Line::Type::READ_ONLY));

  return 3;
}

std::wstring Document::ToString()
{
  std::wstring buffer;

  for(auto &line : lines) {
    buffer += line.ToString();
    buffer += L'\n';
  }

  return buffer;
}

void Document::List(size_t cur_pos)
{
  std::wcout << L"---" << std::endl;

  auto index = 0;
  for(auto& line : lines) {
    if(++index == cur_pos) {
      std::wcout << "=> ";
    }
    else {
      std::wcout << "   ";
    }

    std::wcout << index;
    std::wcout << L": ";
    std::wcout << line.ToString() << std::endl;
  }
}

bool Document::Insert(size_t line_num, const std::wstring line)
{
  if(line_num < lines.size()) {
    size_t cur_num = 1;

    std::list<Line>::iterator iter = lines.begin();
    while(cur_num++ < line_num) {
      ++iter;
    }

    lines.insert(iter, Line(line, Line::Type::READ_WRITE));
    return true;
  }

  return false;
}

bool Document::Delete(size_t line_num)
{
  if(line_num < lines.size()) {
    size_t cur_num = 1;

    std::list<Line>::iterator iter = lines.begin();
    while(cur_num++ < line_num) {
      ++iter;
    }

    if(iter->GetType() == Line::Type::READ_WRITE) {
      lines.erase(iter);
      return true;
    }
  }

  return false;
}

//
// Editor
//
Editor::Editor()
{
  cur_pos = doc.Reset();
}

void Editor::Edit()
{
  bool done = false;
  std::wstring in;
  do {
    std::wcout << L"> ";
    std::getline(std::wcin, in);

    // quit
    if(in == L"/q") {
      done = true;
    }
    // list
    else if(in == L"/l") {
      doc.List(cur_pos);
    }
    // reset
    else if(in == L"/x") {
      doc.Reset();
      std::wcout << SYNTAX_SUCCESS << std::endl;
    }
    // delete line
    else if(StartsWith(in, L"/d ")) {
      const size_t offset = in.find_last_of(L' ');
      if(offset != std::wstring::npos) {
        const std::wstring line_pos_str = in.substr(offset);
        if(doc.Delete(std::stoi(line_pos_str))) {
          cur_pos--;
          std::wcout << SYNTAX_SUCCESS << std::endl;
        }
        else {
          std::wcout << "Read-only." << std::endl;
        }
      }
      else {
        std::wcout << SYNTAX_ERROR << std::endl;
      }
    }
    // goto line
    else if(StartsWith(in, L"/g ")) {
      const size_t offset = in.find_last_of(L' ');
      if(offset != std::wstring::npos) {
        const std::wstring line_pos_str = in.substr(offset);
        const size_t line_pos = std::stoi(line_pos_str);
        if(line_pos < doc.Lines()) {
          cur_pos = line_pos;
          std::wcout << SYNTAX_SUCCESS << std::endl;
        }
        else {
          std::wcout << SYNTAX_ERROR << std::endl;
        }
      }
      else {
        std::wcout << SYNTAX_ERROR << std::endl;
      }
    }
    // insert line
    else if(StartsWith(in, L"/i ")) {
      Append(in.substr(3));
    }
    else {
      std::wcout << SYNTAX_ERROR << std::endl;
    }
  }
  while(!done);

  std::wcout << "Goodbye." << std::endl;
}

void Editor::Append(std::wstring line)
{
  if(doc.Insert(cur_pos, line)) {
    cur_pos++;
    std::wcout << SYNTAX_SUCCESS << std::endl;
  }
  else {
    std::wcout << SYNTAX_ERROR << std::endl;
  }
}