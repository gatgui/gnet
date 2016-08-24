/*

Copyright (C) 2009  Gaetan Guidet

This file is part of gnet.

gnet is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at
your option) any later version.

gnet is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
USA.

*/

#ifndef __gnet_common_h_
#define __gnet_common_h_

#ifndef GNET_STATIC
# ifdef GNET_EXPORTS
#   ifdef _WIN32
#     define GNET_API __declspec(dllexport)
#   else
#     define GNET_API
#   endif
# else
#   ifdef _WIN32
#     define GNET_API __declspec(dllimport)
#   else
#     define GNET_API
#   endif
# endif
#else
# define GNET_API
#endif

#include <string>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <ctime>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <fcntl.h>

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <winsock2.h>
# pragma warning(disable: 4290 4251 4702)
# pragma comment(lib, "wsock32.lib")
typedef SOCKET sock_t;
typedef int socklen_t;
# define NULL_SOCKET INVALID_SOCKET
# define sock_close closesocket
# define sock_errno WSAGetLastError
# ifndef SHUT_RD
#  define SHUT_RD SD_RECEIVE
# endif
# ifndef SHUT_WR
#  define SHUT_WR SD_SEND
# endif
# ifndef SHUT_RDWR
#  define SHUT_RDWR SD_BOTH
# endif
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/select.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <errno.h>
# include <unistd.h>
typedef int sock_t;
# define NULL_SOCKET -1
# define sock_close   ::close
# define sock_errno() errno
#endif

namespace gnet {
  
  GNET_API bool Initialize();
  GNET_API void Uninitialize();
  
  class GNET_API GlobalInit {
  public:
    inline GlobalInit() {
      mInitialized = Initialize();
    }
    inline ~GlobalInit() {
      if (mInitialized) {
        Uninitialize();
      }
    }
    inline operator bool() const {
      return mInitialized;
    }
  private:
    bool mInitialized;
  };
  
  class GNET_API Status {
  public:
    Status();
    Status(bool success, const char *msg=0, bool useErrno=false);
    ~Status();
    
    Status& operator=(const Status &rhs);
    inline operator bool () const { return mSuccess; }
    inline bool operator ! () const { return !mSuccess; }
    
    void clear();
    void set(bool success, const char *msg=0, bool useErrno=false);
    
    inline bool succeeded() const { return mSuccess; }
    inline bool failed() const { return !mSuccess; }
    inline int errcode() const { return mErrCode; }
    inline const char* message() const { return mMsg.c_str(); }
    
  private:
    bool mSuccess;
    int mErrCode;
    std::string mMsg;
  };
}

inline std::ostream& operator<<(std::ostream &os, const gnet::Status &st) {
  os << st.message();
  return os;
}

#endif
