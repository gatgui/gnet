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

#include <gnet/config.h>

namespace gnet {

Exception::Exception(const std::string &klass, const std::string &msg, bool useErrno) {
  mMsg = "Net::";
  mMsg += klass;
  mMsg += " Error: ";
  mMsg += msg;
  if (useErrno == true) {
    mMsg += " [";
    mMsg += strerror(errno);
    mMsg += "]";
    errno = 0;
  }
}

Exception::~Exception() throw() {
}

const char* Exception::what() const throw() {
  return mMsg.c_str();
}

// ---

void Initialize() {
#ifdef _WIN32
  WSADATA wsadata;
  WSAStartup(MAKEWORD(1, 0), &wsadata);
#endif
}

void Uninitialize() {
#ifdef _WIN32
  WSACleanup();
#endif
}
  
}
