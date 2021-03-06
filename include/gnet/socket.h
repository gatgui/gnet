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

#ifndef __gnet_socket_h_
#define __gnet_socket_h_

#include <gnet/config.h>
#include <gnet/connection.h>
#include <vector>

namespace gnet {
  
  class GNET_API Socket {
    public:
      
      friend class Connection;
      
      Socket(unsigned short port) throw(Exception);
      Socket(const Host &host) throw(Exception);
      virtual ~Socket();
      
      bool isValid() const;
      
      inline sock_t fd() const {
        return mFD;
      }
      
      inline const Host& host() const {
        return mHost;
      }
  
    protected:
      
      Socket();
      Socket(const Socket&);
      Socket& operator=(const Socket&);
      
      void invalidate();
      
    protected:
      
      Socket(sock_t fd, const Host &host);
      
      sock_t mFD;
      Host mHost;
      
  };
  
  class GNET_API TCPSocket : public Socket {
    public:
      
      friend class TCPConnection;
      
      TCPSocket(unsigned short port) throw(Exception);
      TCPSocket(const Host &host) throw(Exception);
      virtual ~TCPSocket();
      
      void bind() throw(Exception);
      void listen(int maxConnections) throw(Exception);
      void bindAndListen(int maxConnections) throw(Exception);
      
      TCPConnection* acceptConnection() throw(Exception);
      TCPConnection* connect() throw(Exception);
      void closeConnection(TCPConnection*);
    
    protected:
      
      TCPSocket();
      TCPSocket(const TCPSocket&);
      TCPSocket& operator=(const TCPSocket&);
      
    protected:
      
      int mMaxConnections;
      std::vector<TCPConnection*> mConnections;
      fd_set mRead;
      fd_set mWrite;
  };
  
}

#endif
