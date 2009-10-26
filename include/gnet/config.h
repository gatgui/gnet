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
#include <iostream>
#include <exception>
#include <stdexcept>

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <winsock2.h>
# pragma warning(disable: 4290 4251 4702)
# pragma comment(lib, "wsock32.lib")
typedef SOCKET sock_t;
typedef int socklen_t;
#define NULL_SOCKET INVALID_SOCKET
//typedef int socklen_t;
//# define socket_close          closesocket
//# define socket_read(s, b, l)  recv(s, b, l, 0)
//# define socket_write(s, b, l) send(s, b, l, 0)
//# define SocketHandle          SOCKET
//# define NULL_SOCKET           INVALID_SOCKET
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <errno.h>
typedef int sock_t;
#define NULL_SOCKET -1
//# define socket_close          ::close
//# define socket_read(s, b, l)  recv(s, b, l, 0) //::read(s, b, l)
//# define socket_write(s, b, l) send(s, b, l, 0) //::write(s, b, l)
//# define SocketHandle          int
//# define NULL_SOCKET           -1
#endif

namespace gnet {
  
  GNET_API void Initialize();
  GNET_API void Uninitialize();
  
  class GNET_API Exception : public std::exception {
    public:
      explicit Exception(const std::string &klass, const std::string &msg, bool useErrno=false);
      virtual ~Exception() throw();
      virtual const char* what() const throw();
    private:
      std::string mMsg;
  };
}

#endif
