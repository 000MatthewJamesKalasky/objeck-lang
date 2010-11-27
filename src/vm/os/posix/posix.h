/***************************************************************************
 * Provides runtime support for POSIX compliant systems i.e 
 * Linux, OS X, etc.
 *
 * Copyright (c) 2008-2009, Randy Hollines
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
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#ifndef __POSIX_H__
#define __POSIX_H__

#include "../../common.h"
#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SOCKET int

/****************************
 * File support class
 ****************************/
class File {
 public:
  static long FileSize(const char* name) {
    int fd = open(name, O_RDONLY);
    if(fd < 0) {
      return -1;
    }
	  
    struct stat info;
    if(fstat(fd, &info) < 0) {
      return -1;
    }
	  
    close(fd);
    return info.st_size;
  }
  
  static bool FileExists(const char* name) {
    int fd = open(name, O_RDONLY);
    if(fd < 0) {
      return false;
    }
    
    close(fd);
    return true;
  }

  static FILE* FileOpen(const char* name, const char* mode) {
    FILE* file = fopen(name, mode);
    if(file < 0) {
      return NULL;
    }
    
    return file;
  }

  static bool MakeDir(const char* name) {
    if(mkdir(name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
      return false;
    }

    return true;
  }

  static bool IsDir(const char* name) {
    DIR* dir = opendir(name);
    if(!dir) {
      return false;
    }
    closedir(dir);

    return true;
  }

  static vector<string> ListDir(const char* path) {
    vector<string> files;
    
    struct dirent **names;
    int n = scandir(path, &names, 0, alphasort);
    if(n > 0) {
      while(n--) {
	if((strcmp(names[n]->d_name, "..") != 0) && (strcmp(names[n]->d_name, ".") != 0)) {
	  files.push_back(names[n]->d_name);
	}
	free(names[n]);
      }
      free(names);
    }

    return files;
  }
};

/****************************
 * IP socket support class
 ****************************/
class IPSocket {
 public:
  static SOCKET Open(const char* address, int port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(socket < 0) {
      return -1;
    }

    struct hostent* host_info = gethostbyname(address);
    if(!host_info) {
      return -1;
    }
    
    long host_addr;
    memcpy(&host_addr, host_info->h_addr, host_info->h_length);
    
    struct sockaddr_in ip_addr;
    ip_addr.sin_addr.s_addr = host_addr;;
    ip_addr.sin_port=htons(port);
    ip_addr.sin_family = AF_INET;
    
    if(!connect(sock, (struct sockaddr*)&ip_addr,sizeof(ip_addr))) {
      return sock;
    }
    
    return -1;
  }

  static void WriteByte(char value, SOCKET sock) {
    send(sock, &value, 1, 0);
  }

  static int WriteBytes(char* values, int len, SOCKET sock) {
    return send(sock, values, len, 0);
  }

  static char ReadByte(SOCKET sock, int &status) {
    char value;
    status = recv(sock, &value, 1, 0);

    return value;
  }

  static int ReadBytes(char* values, int len, SOCKET sock) {
    return recv(sock, values, len, 0);
  }
  
  static void Close(SOCKET sock) {
    close(sock);
  }
};

#endif
