/***************************************************************************
 * Native executable launcher for 
 *
 * Copyright (c) 2022, Randy Hollines
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

#include "native_launcher.h"

int main(int argc, char* argv[])
{
  const string working_dir = GetWorkingDirectory();
  if(working_dir.empty()) {
    cerr << ">>> Unable to find the working directory <<<" << endl;
    return 1;
  }

cout << "--- |" << working_dir << "| ---" << endl;
  const string path_env = GetEnviromentPath(working_dir);
  const string lib_env = GetLibraryPath(working_dir);
  if(path_env.empty() || lib_env.empty()) {
    cout << ">>> Unable to determine the current working directory <<<" << endl;
    return 1;
  }

  const char* spawn_env[] = { path_env.c_str(), lib_env.c_str(), nullptr };

  // _spawnve(P_WAIT, spawn_path, spawn_args, spawn_env);


 // cout << "env_path=|" << env_path << "|, lib_path=|" << lib_path << '|' << endl;

  return 0;
}
