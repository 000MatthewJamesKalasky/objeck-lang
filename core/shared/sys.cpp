/***************************************************************************
 * Static logger
 *
 * Copyright (c) 2018-2020, Randy Hollines
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
 * - Neither the name of the VM Team nor the names of its
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

#include "sys.h"

void SetEnv() {
#ifdef _WIN32
#ifdef _MSYS2_CLANG
  std::ios_base::sync_with_stdio(false);
  std::locale utf8(std::locale(), new std::codecvt_utf8_utf16<wchar_t>);
  std::wcout.imbue(utf8);
  std::wcin.imbue(utf8);
#else
  if(_setmode(_fileno(stdin), _O_U8TEXT) < 0) {
    exit(1);
  }

  if(_setmode(_fileno(stdout), _O_U8TEXT) < 0) {
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