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

#ifndef __gnet_connection_h_
#define __gnet_connection_h_

#include <gnet/config.h>
#include <gnet/host.h>

namespace gnet {

  class TCPSocket;
  
  class GNET_API Connection {
    
    public:
      
      Connection();
      virtual ~Connection();
      
      // timeout < 0 -> blocking
      // timeout = 0 -> non-blocking
      // timeout > 0 -> non-blocking + time limit
      // timeout is in milliseconds
      // return true when something is read
      virtual bool read(char *&bytes, size_t &len, double timeout=-1) throw(Exception) = 0;
      virtual bool readUntil(const char *until, char *&bytes, size_t &len, double timeout=-1) throw(Exception) = 0;
      virtual bool write(const char* bytes, size_t len, double timeout=-1) throw(Exception) = 0;
      
      virtual bool isValid() const;
      virtual void invalidate();
      // isAlive default behaviour is identical to isValid
      virtual bool isAlive() const;
      
      // for some reasons, if those 2 following functions are named 'read' and 'write'
      // calling them from TCPConnection instance resulted in compilation error on linux...
      bool read(std::string &s, double timeout=-1) throw(Exception);
      bool readUntil(const char *until, std::string &s, double timeout=-1) throw(Exception);
      bool write(const std::string &s, double timeout=-1) throw(Exception);
      
      inline unsigned long getBufferSize() const {
        return mBufferSize;
      }
      
      inline sock_t fd() const {
        return mFD;
      }
      
      void setBufferSize(unsigned long n);
      
      bool setBlocking(bool blocking);
    
    private:
      
      Connection(const Connection&);
      Connection& operator=(const Connection &);
      
    protected:
      
      Connection(sock_t fd);
      
      sock_t mFD;
      unsigned long mBufferSize;
      char *mBuffer;
      unsigned long mBufferOffset;
  };
  
  class GNET_API TCPConnection : public Connection {
    
    public:
      
      friend class TCPSocket;
      
      virtual ~TCPConnection();
      
      virtual bool isValid() const;
      virtual void invalidate();
      // isAlive will try to peek read on connection socket
      // If connection is blocking, so will isAlive
      virtual bool isAlive() const;
      
      // Need to add those the std::string overrides of read, readUntil and write are 
      // available to TCPConnection class
      // (overrides only work in one scope at a time)
      using Connection::read;
      using Connection::readUntil;
      using Connection::write;
      
      virtual bool read(char *&bytes, size_t &len, double timeout=-1) throw(Exception);
      virtual bool readUntil(const char *until, char *&bytes, size_t &len, double timeout=-1) throw(Exception);
      virtual bool write(const char* bytes, size_t len, double timeout=-1) throw(Exception);
      
      inline const Host& host() const {
        return mHost;
      }
      
    private:
      
      TCPConnection();
      TCPConnection(const TCPConnection&);
      TCPConnection& operator=(const TCPConnection&);
      
    protected:
      
      TCPConnection(TCPSocket *socket, sock_t fd, const Host &host);
      
      Host mHost;
      TCPSocket *mSocket;
  };
  
  // class NET_API UDPConnection : public Connection
  
}

#endif
