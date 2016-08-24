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
      
      
      inline sock_t fd() const { return mFD; }
      
      bool setBlocking(bool blocking);
      bool setLinger(bool linger);
      
      
      virtual bool isValid() const;
      virtual void invalidate();
      // isAlive default behaviour is identical to isValid
      virtual bool isAlive() const;
      
      
      // Arguments
      //   [out] bytes    : Pointer to read bytes
      //   [out] len      : Length of read bytes buffer
      //   [in] timeout   : Read timeout
      //                    A value < 0 will result in a 'blocking' read.
      //                    A value of 0 will make the function return immediately after the read, no matter the result.
      //                    A value > 0 will make the function try reading for at least the specified amount of time in milliseconds.
      //   [out] status   : Error status
      //
      // Return value
      //   true when anything read, even when call ends up with an error
      //
      // Note
      //   'bytes', if allocated, MUST be freed by the caller no matter the return value or error status
      virtual bool read(char *&bytes, size_t &len, double timeout=-1, Status *status=0) = 0;
      // Arguments
      //   As for 'read'
      //
      // Return value
      //   true if the 'until' string was from in read bytes, false otherwise
      //
      // Note
      //   when false is returned, it doesn't mean nothing was read, be sure to check the 'bytes' buffer
      virtual bool readUntil(const char *until, char *&bytes, size_t &len, double timeout=-1, Status *status=0) = 0;
      virtual size_t write(const char* bytes, size_t len, double timeout=-1, Status *status=0) = 0;
      
      bool read(std::string &s, double timeout=-1, Status *status=0);
      bool readUntil(const char *until, std::string &s, double timeout=-1, Status *status=0);
      size_t write(const std::string &s, double timeout=-1, Status *status=0);
      
      
      void setBufferSize(unsigned long n);
      inline unsigned long getBufferSize() const { return mBufferSize; }
      inline bool hasPendingData() const { return (mBufferOffset > 0); }
      
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
      
      
      inline const Host& host() const { return mHost; }
      bool readShutdown();
      bool writeShutdown();
      bool shutdown();
      
      
      virtual bool isValid() const;
      virtual void invalidate();
      // isAlive will try to peek read on connection socket
      // If connection is blocking, so will isAlive be
      virtual bool isAlive() const;
      
      // Need to add those the std::string overrides of read, readUntil and write are
      //   available to TCPConnection class instances
      // (overrides only work in one scope at a time)
      using Connection::read;
      using Connection::readUntil;
      using Connection::write;
      
      virtual bool read(char *&bytes, size_t &len, double timeout=-1, Status *status=0);
      virtual bool readUntil(const char *until, char *&bytes, size_t &len, double timeout=-1, Status *status=0);
      virtual size_t write(const char* bytes, size_t len, double timeout=-1, Status *status=0);
      
      
    private:
      
      TCPConnection();
      TCPConnection(const TCPConnection&);
      TCPConnection& operator=(const TCPConnection&);
      
      bool checkUntil(const char *until, char *in, size_t inlen, char *&out, size_t &outlen);
      
    protected:
      
      TCPConnection(TCPSocket *socket, sock_t fd, const Host &host);
      
      Host mHost;
      TCPSocket *mSocket;
  };
  
  // class GNET_API UDPConnection : public Connection { ...
  
}

#endif
