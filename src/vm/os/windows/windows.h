/***************************************************************************
 * Provides runtime support for Windows (Win32) based systems.
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
 *%
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

#ifndef __WINDOWS_H__
#define __WINDOWS_H__

#define BUFSIZE 256

#include "../../common.h"
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "User32.lib")

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

class File {
 public:
  static long FileSize(const char* name) {
    HANDLE file = CreateFile(name, GENERIC_READ, 
			     FILE_SHARE_READ, NULL, OPEN_EXISTING, 
			     FILE_ATTRIBUTE_NORMAL, NULL);
    
    if(file == INVALID_HANDLE_VALUE) {
      return -1;
    }
    
    long size = GetFileSize(file, NULL);
    CloseHandle(file);
    if(size < 0) {
      return -1;
    }
    
    return size;  
  }

  static bool FileExists(const char* name) {
    HANDLE file = CreateFile(name, GENERIC_READ, 
			     FILE_SHARE_READ, NULL, OPEN_EXISTING, 
			     FILE_ATTRIBUTE_NORMAL, NULL);    
    
    if(file == INVALID_HANDLE_VALUE) {
      return false;
    }
    CloseHandle(file);

    return true;
  }

  static FILE* FileOpen(const char* name, const char* mode) {
    FILE* file;
    if(fopen_s(&file, name, mode) != 0) {
      return NULL;
    }

    return file;
  }

  static bool MakeDir(const char* name) {
    if(CreateDirectory(name, NULL) == 0) {
      return false;
    }

    return true;
  }

  static bool IsDir(const char* name) {
    WIN32_FIND_DATA data;
    HANDLE find = FindFirstFile(name, &data);
    if(find == INVALID_HANDLE_VALUE) {
      return false;
    }
    FindClose(find);
    
    return true;
  }

  static vector<string> ListDir(char* p) {
    vector<string> files;
    
    string path = p;
    if(path.size() > 0 && path[path.size() - 1] == '\\') {
      path += "*";
    }
    else {
      path += "\\*";
    }

    WIN32_FIND_DATA file_data;
    HANDLE find = FindFirstFile(path.c_str(), &file_data);
    if(find == INVALID_HANDLE_VALUE) {
      return files;
    } 
    else {
      files.push_back(file_data.cFileName);

      BOOL b = FindNextFile(find, &file_data);
      while(b) {
        files.push_back(file_data.cFileName);
        b = FindNextFile(find, &file_data);
      }
      FindClose(find);
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
    if(sock == INVALID_SOCKET) {
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

  static char ReadByte(SOCKET sock) {
    char value;
    recv(sock, &value, 1, 0);

    return value;
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
    closesocket(sock);
  }
};

/****************************
 * System operations
 ****************************/
class System {
 public:
  static string GetPlatform() {
    string platform;

    TCHAR szOS[BUFSIZE];
    if(GetOSDisplayString(szOS)) {
      platform += szOS;
    }
    else {
      platform = "Unknown";
    }
    
    return platform;
  }

  static BOOL GetOSDisplayString( LPTSTR pszOS)
  {
     OSVERSIONINFOEX osvi;
     SYSTEM_INFO si;
     PGNSI pGNSI;
     PGPI pGPI;
     BOOL bOsVersionInfoEx;
     DWORD dwType;

     ZeroMemory(&si, sizeof(SYSTEM_INFO));
     ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

     osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

     if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
        return 1;

     // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.

     pGNSI = (PGNSI) GetProcAddress(
        GetModuleHandle(TEXT("kernel32.dll")), 
        "GetNativeSystemInfo");
     if(NULL != pGNSI)
        pGNSI(&si);
     else GetSystemInfo(&si);

     if ( VER_PLATFORM_WIN32_NT==osvi.dwPlatformId && 
          osvi.dwMajorVersion > 4 )
     {
        StringCchCopy(pszOS, BUFSIZE, TEXT("Microsoft "));

        // Test for the specific product.

        if ( osvi.dwMajorVersion == 6 )
        {
           if( osvi.dwMinorVersion == 0 )
           {
              if( osvi.wProductType == VER_NT_WORKSTATION )
                  StringCchCat(pszOS, BUFSIZE, TEXT("Windows Vista "));
              else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2008 " ));
           }

           if ( osvi.dwMinorVersion == 1 )
           {
              if( osvi.wProductType == VER_NT_WORKSTATION )
                  StringCchCat(pszOS, BUFSIZE, TEXT("Windows 7 "));
              else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2008 R2 " ));
           }
         
           pGPI = (PGPI) GetProcAddress(
              GetModuleHandle(TEXT("kernel32.dll")), 
              "GetProductInfo");

           pGPI( osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);

           switch( dwType )
           {
              case PRODUCT_ULTIMATE:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Ultimate Edition" ));
                 break;
              case PRODUCT_PROFESSIONAL:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Professional" ));
                 break;
              case PRODUCT_HOME_PREMIUM:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Home Premium Edition" ));
                 break;
              case PRODUCT_HOME_BASIC:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Home Basic Edition" ));
                 break;
              case PRODUCT_ENTERPRISE:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition" ));
                 break;
              case PRODUCT_BUSINESS:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Business Edition" ));
                 break;
              case PRODUCT_STARTER:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Starter Edition" ));
                 break;
              case PRODUCT_CLUSTER_SERVER:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Cluster Server Edition" ));
                 break;
              case PRODUCT_DATACENTER_SERVER:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Datacenter Edition" ));
                 break;
              case PRODUCT_DATACENTER_SERVER_CORE:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Datacenter Edition (core installation)" ));
                 break;
              case PRODUCT_ENTERPRISE_SERVER:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition" ));
                 break;
              case PRODUCT_ENTERPRISE_SERVER_CORE:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition (core installation)" ));
                 break;
              case PRODUCT_ENTERPRISE_SERVER_IA64:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition for Itanium-based Systems" ));
                 break;
              case PRODUCT_SMALLBUSINESS_SERVER:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Small Business Server" ));
                 break;
              case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Small Business Server Premium Edition" ));
                 break;
              case PRODUCT_STANDARD_SERVER:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Standard Edition" ));
                 break;
              case PRODUCT_STANDARD_SERVER_CORE:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Standard Edition (core installation)" ));
                 break;
              case PRODUCT_WEB_SERVER:
                 StringCchCat(pszOS, BUFSIZE, TEXT("Web Server Edition" ));
                 break;
           }
        }

        if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
        {
           if( GetSystemMetrics(SM_SERVERR2) )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Windows Server 2003 R2, "));
           else if ( osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Windows Storage Server 2003"));
           else if ( osvi.wSuiteMask & VER_SUITE_WH_SERVER )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Windows Home Server"));
           else if( osvi.wProductType == VER_NT_WORKSTATION &&
                    si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
           {
              StringCchCat(pszOS, BUFSIZE, TEXT( "Windows XP Professional x64 Edition"));
           }
           else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2003, "));

           // Test for the server type.
           if ( osvi.wProductType != VER_NT_WORKSTATION )
           {
              if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
              {
                  if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                     StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter Edition for Itanium-based Systems" ));
                  else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                     StringCchCat(pszOS, BUFSIZE, TEXT( "Enterprise Edition for Itanium-based Systems" ));
              }

              else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
              {
                  if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                     StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter x64 Edition" ));
                  else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                     StringCchCat(pszOS, BUFSIZE, TEXT( "Enterprise x64 Edition" ));
                  else StringCchCat(pszOS, BUFSIZE, TEXT( "Standard x64 Edition" ));
              }

              else
              {
                  if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
                     StringCchCat(pszOS, BUFSIZE, TEXT( "Compute Cluster Edition" ));
                  else if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                     StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter Edition" ));
                  else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                     StringCchCat(pszOS, BUFSIZE, TEXT( "Enterprise Edition" ));
                  else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
                     StringCchCat(pszOS, BUFSIZE, TEXT( "Web Edition" ));
                  else StringCchCat(pszOS, BUFSIZE, TEXT( "Standard Edition" ));
              }
           }
        }

        if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
        {
           StringCchCat(pszOS, BUFSIZE, TEXT("Windows XP "));
           if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Home Edition" ));
           else StringCchCat(pszOS, BUFSIZE, TEXT( "Professional" ));
        }

        if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
        {
           StringCchCat(pszOS, BUFSIZE, TEXT("Windows 2000 "));

           if ( osvi.wProductType == VER_NT_WORKSTATION )
           {
              StringCchCat(pszOS, BUFSIZE, TEXT( "Professional" ));
           }
           else 
           {
              if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                 StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter Server" ));
              else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                 StringCchCat(pszOS, BUFSIZE, TEXT( "Advanced Server" ));
              else StringCchCat(pszOS, BUFSIZE, TEXT( "Server" ));
           }
        }

         // Include service pack (if any) and build number.

        if( _tcslen(osvi.szCSDVersion) > 0 )
        {
            StringCchCat(pszOS, BUFSIZE, TEXT(" ") );
            StringCchCat(pszOS, BUFSIZE, osvi.szCSDVersion);
        }

        TCHAR buf[80];

        StringCchPrintf( buf, 80, TEXT(" (build %d)"), osvi.dwBuildNumber);
        StringCchCat(pszOS, BUFSIZE, buf);

        if ( osvi.dwMajorVersion >= 6 )
        {
           if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
              StringCchCat(pszOS, BUFSIZE, TEXT( ", 64-bit" ));
           else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
              StringCchCat(pszOS, BUFSIZE, TEXT(", 32-bit"));
        }
      
        return TRUE; 
     }

     else
     {  
        printf( "This sample does not support this version of Windows.\n");
        return FALSE;
     }
  }
};

#endif
