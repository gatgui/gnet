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

#include <gnet/host.h>
#include <gcore/rexp.h>
#include <sstream>

namespace gnet {

static gcore::Regexp IPAddressRE(IEC("\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}"));

Host::Host() {
  memset(&mAddr, 0, sizeof(struct sockaddr_in));
}

Host::Host(const std::string &addr, unsigned short port) throw(Exception) {

  memset(&mAddr, 0, sizeof(struct sockaddr_in));
  mAddr.sin_family = AF_INET;
  mAddr.sin_addr.s_addr = INADDR_ANY;
  mAddr.sin_port = htons((u_short)port);

  if (IPAddressRE.match(addr) == true) {

#ifdef _WIN32
    mAddr.sin_addr.s_addr = inet_addr(addr.c_str());
#else
    inet_aton(addr.c_str(), &(mAddr.sin_addr));
#endif

  } else {
    
    struct hostent *he = gethostbyname(addr.c_str());
    if (he == NULL) {
      std::ostringstream oss;
      oss << "Could not find host: \"" << addr << "\"";
      throw Exception("Host", oss.str());
    }
    memcpy(&(mAddr.sin_addr.s_addr), he->h_addr, he->h_length);
  }
  
}

Host::Host(const Host &rhs) {
  memcpy(&mAddr, &(rhs.mAddr), sizeof(struct sockaddr_in));
}

Host::~Host() {
}

Host& Host::operator=(const Host &rhs) {
  if (this != &rhs) {
    memcpy(&mAddr, &(rhs.mAddr), sizeof(struct sockaddr_in));
  }
  return *this;
}

unsigned short Host::port() const {
  return ntohs(mAddr.sin_port);
}

std::string Host::address() const {
  return inet_ntoa(mAddr.sin_addr);
}

Host::operator struct sockaddr* () {
  return (struct sockaddr*) &mAddr;
}

Host::operator const struct sockaddr* () const {
  return (const struct sockaddr*) &mAddr;
}
  
}


