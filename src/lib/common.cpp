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

Status::Status()
  : mSuccess(true)
  , mErrCode(-1)
  , mMsg("") {
}

Status::Status(bool success, const char *msg, bool useErrno) {
  set(success, msg, useErrno);
}

Status::~Status() {
}

Status& Status::operator=(const Status &rhs) {
  mSuccess = rhs.mSuccess;
  mErrCode = rhs.mErrCode;
  mMsg = rhs.mMsg;
  return *this;
}

void Status::clear() {
  mSuccess = true;
  mErrCode = -1;
  mMsg = "";
}

void Status::set(bool success, const char *msg, bool useErrno) {
  mSuccess = success;
  mErrCode = -1;
  mMsg = (msg != NULL ? msg : "");
  if (!mSuccess && useErrno) {
    mMsg += " (";
#ifdef _WIN32
    mErrCode = WSAGetLastError();
    LPTSTR buffer = NULL; 
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, WSAGetLastError(), MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT),
                  (LPTSTR)&buffer, 0, NULL);
    if (buffer) {
      mMsg += std::string(buffer);
      LocalFree(buffer);
    }
#else
    mErrCode = errno;
    mMsg += strerror(mErrCode);
#endif
    mMsg += ")";
  }
}

// ---

bool Initialize() {
#ifdef _WIN32
  WSADATA wsadata;
  if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
    return false;
  }
  // Confirm that the WinSock DLL supports 2.2
  if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2) {
    WSACleanup();
    return false;
  }
#endif
  return true;
}

void Uninitialize() {
#ifdef _WIN32
  WSACleanup();
#endif
}
  
}
