/***************************************************************************
 * Language scanner.
 *
 * Copyright (c) 2008-2012, Randy Hollines
 * All rights reserved.
 *
 * Reistribution and use in source and binary forms, with or without
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

#ifndef __SCANNER_H__
#define __SCANNER_H__

#include "tree.h"

using namespace std;

// comment
#define COMMENT '#'
#define EXTENDED_COMMENT '~'

// look ahead value
#define LOOK_AHEAD 3
// white space
#define WHITE_SPACE (cur_char == ' ' || cur_char == '\t' || cur_char == '\r' || cur_char == '\n')

/****************************
 * Token types
 ****************************/
enum TokenType {
  // misc
  TOKEN_END_OF_STREAM = -1000,
  TOKEN_NO_INPUT,
  TOKEN_UNKNOWN,
  // symbols
  TOKEN_TILDE,
  TOKEN_PERIOD,
  TOKEN_COLON,
  TOKEN_SEMI_COLON,
  TOKEN_COMMA,
  TOKEN_ASSIGN,
  TOKEN_ADD_ASSIGN,
  TOKEN_SUB_ASSIGN,
  TOKEN_MUL_ASSIGN,
  TOKEN_DIV_ASSIGN,
  TOKEN_OPEN_BRACE,
  TOKEN_CLOSED_BRACE,
  TOKEN_OPEN_PAREN,
  TOKEN_CLOSED_PAREN,
  TOKEN_OPEN_BRACKET,
  TOKEN_CLOSED_BRACKET,
  TOKEN_ASSESSOR,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_QUESTION,
  TOKEN_EQL,
  TOKEN_NEQL,
  TOKEN_LES,
  TOKEN_GTR,
  TOKEN_GEQL,
  TOKEN_LEQL,
  TOKEN_ADD,
  TOKEN_SUB,
  TOKEN_MUL,
  TOKEN_DIV,
  TOKEN_MOD,
  TOKEN_SHL,
  TOKEN_SHR,
  // literals
  TOKEN_IDENT,
  TOKEN_INT_LIT,
  TOKEN_FLOAT_LIT,
  TOKEN_CHAR_LIT,
  TOKEN_CHAR_STRING_LIT,
  // reserved words
  TOKEN_AND_ID,
  TOKEN_OR_ID,
  TOKEN_XOR_ID,
  TOKEN_NATIVE_ID,
  TOKEN_IF_ID,
  TOKEN_ELSE_ID,
  TOKEN_DO_ID,
  TOKEN_WHILE_ID,
  TOKEN_BREAK_ID,
  TOKEN_BOOLEAN_ID,
  TOKEN_TRUE_ID,
  TOKEN_FALSE_ID,
  TOKEN_USE_ID,
  TOKEN_BUNDLE_ID,
  TOKEN_VIRTUAL_ID,
  TOKEN_STATIC_ID,
  TOKEN_PUBLIC_ID,
  TOKEN_PRIVATE_ID,
  TOKEN_RETURN_ID,
  TOKEN_AS_ID,
  TOKEN_TYPE_OF_ID,
  TOKEN_PARENT_ID,
  TOKEN_FROM_ID,
  TOKEN_FOR_ID,
  TOKEN_EACH_ID,
  TOKEN_ENUM_ID,
  TOKEN_SELECT_ID,
  TOKEN_OTHER_ID,
  TOKEN_LABEL_ID,
  TOKEN_NEW_ID,
  TOKEN_CLASS_ID,
  TOKEN_INTERFACE_ID,
  TOKEN_IMPLEMENTS_ID,
  TOKEN_FUNCTION_ID,
  TOKEN_METHOD_ID,
  TOKEN_BYTE_ID,
  TOKEN_INT_ID,
  TOKEN_FLOAT_ID,
  TOKEN_CHAR_ID,
  TOKEN_NIL_ID,
  TOKEN_CRITICAL_ID,
#ifdef _SYSTEM
  CPY_BYTE_ARY,
  CPY_INT_ARY,
  CPY_FLOAT_ARY,
  FLOR_FLOAT,
  CEIL_FLOAT,
  SIN_FLOAT,
  COS_FLOAT,
  TAN_FLOAT,
  LOG_FLOAT,
  POW_FLOAT,
  SQRT_FLOAT,
  RAND_FLOAT,
  LOAD_CLS_INST_ID,
  LOAD_CLS_BY_INST,
  LOAD_CLS_BY_NAME,
  INVOKE_MTHD,
  LOAD_INST_UID,
  // standard i/o
  STD_OUT_BOOL,
  STD_OUT_BYTE,
  STD_OUT_CHAR,
  STD_OUT_INT,
  STD_OUT_FLOAT,
  LOAD_ARY_SIZE,
  STD_OUT_CHAR_ARY,
  STD_IN_STRING,
  // file open/close
  FILE_OPEN_READ,
  FILE_OPEN_WRITE,
  FILE_OPEN_READ_WRITE,
  FILE_CLOSE,
  // file-in
  FILE_IN_BYTE,
  FILE_IN_BYTE_ARY,
  FILE_IN_STRING,
  // file-out
  FILE_OUT_BYTE,
  FILE_OUT_BYTE_ARY,
  FILE_OUT_STRING,
  // file-operations
  FILE_EXISTS,
  FILE_IS_OPEN,
  FILE_SIZE,
  FILE_SEEK,
  FILE_REWIND,
  FILE_EOF,
  FILE_DELETE,
  FILE_RENAME,
  // directory-operations
  DIR_CREATE,
  DIR_EXISTS,
  DIR_LIST,
  // socket operations
  SOCK_TCP_CONNECT,
  SOCK_TCP_IS_CONNECTED,
  SOCK_TCP_CLOSE,
  // socket-in
  SOCK_TCP_IN_BYTE,
  SOCK_TCP_IN_BYTE_ARY,
  SOCK_TCP_IN_STRING,
  // socket-out
  SOCK_TCP_OUT_BYTE,
  SOCK_TCP_OUT_BYTE_ARY,
  SOCK_TCP_OUT_STRING,
  SOCK_TCP_HOST_NAME,
  // serialization
  SERL_INT,
  SERL_FLOAT,
  SERL_OBJ_INST,
  SERL_BYTE_ARY,
  SERL_INT_ARY,
  SERL_FLOAT_ARY,
  DESERL_INT,
  DESERL_FLOAT,
  DESERL_OBJ_INST,
  DESERL_BYTE_ARY,
  DESERL_INT_ARY,
  DESERL_FLOAT_ARY,
  // shared library support
  DLL_LOAD,
  DLL_UNLOAD,
  DLL_FUNC_CALL,
  // thread management
  ASYNC_MTHD_CALL,
  THREAD_MUTEX,
  THREAD_SLEEP,
  THREAD_JOIN,
  // time
  SYS_TIME,
  TIMER_START,
  TIMER_END,
  // platform
  PLTFRM,
  EXIT,
#endif
};

/****************************
 * Token class
 ****************************/
class Token {
private:
  TokenType token_type;
  int line_nbr;
  string filename;
  string ident;

  INT_VALUE int_lit;
  FLOAT_VALUE double_lit;
  CHAR_VALUE char_lit;
  BYTE_VALUE byte_lit;

public:

  inline void Copy(Token* token) {
    line_nbr = token->GetLineNumber();
    char_lit = token->GetCharLit();
    int_lit = token->GetIntLit();
    double_lit = token->GetFloatLit();
    ident = token->GetIdentifier();
    token_type = token->GetType();
    filename = token->GetFileName();
  }

  inline const string GetFileName() {
    return filename;
  }

  inline void SetFileName(string f) {
    filename = f;
  }

  inline const int GetLineNumber() {
    return line_nbr;
  }

  inline void SetLineNbr(int l) {
    line_nbr = l;
  }

  inline void  SetIntLit(INT_VALUE i) {
    int_lit = i;
  }

  inline void SetFloatLit(FLOAT_VALUE d) {
    double_lit = d;
  }

  inline void SetByteLit(BYTE_VALUE b) {
    byte_lit = b;
  }

  inline void SetCharLit(CHAR_VALUE c) {
    char_lit = c;
  }

  inline void SetIdentifier(string i) {
    ident = i;
  }

  inline const INT_VALUE GetIntLit() {
    return int_lit;
  }

  inline const FLOAT_VALUE GetFloatLit() {
    return double_lit;
  }

  inline const BYTE_VALUE GetByteLit() {
    return byte_lit;
  }

  inline const CHAR_VALUE GetCharLit() {
    return char_lit;
  }

  inline const string GetIdentifier() {
    return ident;
  }

  inline const TokenType GetType() {
    return token_type;
  }

  inline void SetType(TokenType t) {
    token_type = t;
  }
};

/**********************************
 * Token scanner with k lookahead
 * tokens
 **********************************/
class Scanner {
private:
  // input file name
  string filename;
  // input buffer
  char* buffer;
  // buffer size
  std::streamoff buffer_size;
  // input buffer position
  int buffer_pos;
  // start marker position
  int start_pos;
  // end marker position
  int end_pos;
  // input characters
  char cur_char, nxt_char, nxt_nxt_char;
  // map of reserved identifiers
  map<const string, TokenType> ident_map;
  // array of tokens for lookahead
  Token* tokens[LOOK_AHEAD];
  // line number
  int line_nbr;

  // warning message
  void ProcessWarning() {
    cout << GetToken()->GetFileName() << ":" << GetToken()->GetLineNumber() + 1
         << ": Parse warning: Unknown token: '" << cur_char << "'" << endl;
  }

  // loads a file into memory
  char* LoadFileBuffer(string filename, std::streamoff& buffer_size) {
    char* buffer = NULL;
    // open file
    ifstream in(filename.c_str(), ifstream::binary);
    if(in.good()) {
      // get file size
      in.seekg(0, ios::end);
      buffer_size = in.tellg();
      in.seekg(0, ios::beg);
      buffer = (char*)calloc(buffer_size + 1, sizeof(char));
      in.read(buffer, buffer_size);
      // close file
      in.close();
    } else {
      cout << "Unable to open source file: " << filename << endl;
      exit(1);
    }

    return buffer;
  }

  // parsers a character string
  inline void CheckString(int index) {
    // copy string
    const int length = end_pos - start_pos;
    string char_string(buffer, start_pos, length);
    // set string
    tokens[index]->SetType(TOKEN_CHAR_STRING_LIT);
    tokens[index]->SetIdentifier(char_string);
  }

  // parse an integer
  inline void ParseInteger(int index, int base = 0) {
    // copy string
    int length = end_pos - start_pos;
    string ident(buffer, start_pos, length);

    // set token
    char* end;
    tokens[index]->SetType(TOKEN_INT_LIT);
    tokens[index]->SetIntLit(strtol(ident.c_str(), &end, base));
    tokens[index]->SetLineNbr(line_nbr);
    tokens[index]->SetFileName(filename);
  }

  // parse a double
  inline void ParseDouble(int index) {
    // copy string
    const int length = end_pos - start_pos;
    string ident(buffer, start_pos, length);
    // set token
    tokens[index]->SetType(TOKEN_FLOAT_LIT);
    tokens[index]->SetFloatLit(atof(ident.c_str()));
    tokens[index]->SetLineNbr(line_nbr);
    tokens[index]->SetFileName(filename);
  }

  // parsers an unicode character
  inline void ParseUnicodeChar(int index) {
    // copy string
    const int length = end_pos - start_pos;
    string ident(buffer, start_pos, length);
    // set token
    char* end;
    tokens[index]->SetType(TOKEN_CHAR_LIT);
    tokens[index]->SetCharLit((CHAR_VALUE)strtol(ident.c_str(), &end, 16));
    tokens[index]->SetLineNbr(line_nbr);
    tokens[index]->SetFileName(filename);
  }

  // read input file into memory
  void ReadFile();
  // ignore white space
  void Whitespace();
  // next character
  void NextChar();
  // load reserved keywords
  void LoadKeywords();
  // parses a new token
  void ParseToken(int index);
  // check identifier
  void CheckIdentifier(int index);

public:
  // default constructor
  Scanner(string f, bool p = false);
  // default destructor
  ~Scanner();

  // next token
  void NextToken();

  // token accessor
  Token* GetToken(int index = 0);

  // gets the file name
  string GetFileName() {
    return filename;
  }
};

#endif
