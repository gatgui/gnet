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

#ifndef __gnet_host_h_
#define __gnet_host_h_

#include <gnet/config.h>

namespace gnet {
  
  class GNET_API Host {
    
    public:
      
      Host();
      Host(const std::string &addr, unsigned short port) throw(Exception);
      Host(const Host &rhs);
      ~Host();

      Host& operator=(const Host &rhs);

      unsigned short port() const;
      std::string address() const;

      operator struct sockaddr* ();
      operator const struct sockaddr* () const;

    protected:
      
      struct sockaddr_in mAddr;
    
  };
  
}

#endif
