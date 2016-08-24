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
#include <list>

namespace gnet {
  
  class GNET_API Socket {
    public:
      
      friend class Connection;
      
      Socket(unsigned short port, Status *status=0);
      Socket(const Host &host, Status *status=0);
      virtual ~Socket();
      
      bool isValid() const;
      
      inline const Host& host() const { return mHost; }
      inline sock_t fd() const { return mFD; }
  
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
      
      TCPSocket(unsigned short port, Status *status=0);
      TCPSocket(const Host &host, Status *status=0);
      virtual ~TCPSocket();
      
      
      void setDefaultBlocking(bool blocking);
      void setDefaultLinger(bool linger);
      
      
      Status bind();
      Status listen(int maxConnections);
      Status bindAndListen(int maxConnections);
      
      // Arguments
      //   [in] timeout : Select timeout.
      //                  A value < 0 will make the call blocking.
      //                  A value if 0 will make the select call returns immediately
      //                  A value > 0 will make the call monitor the connections for the specified amount of time in milliseconds
      //   [out] status : Error status
      // 
      // Return value
      //   Number of connections that can be processed
      inline size_t select(double timeout=-1, Status *status=0) { return this->select(true, true, timeout, status); }
      inline size_t selectReadable(double timeout=-1, Status *status=0) { return this->select(true, false, timeout, status); }
      inline size_t selectWritable(double timeout=-1, Status *status=0) { return this->select(false, true, timeout, status); }
      // Use the following to iterate over selected connections
      TCPConnection* nextReadable();
      TCPConnection* nextWritable();
      
      // Arguments
      //   As select
      // 
      // Return value
      //   -1 on error
      //   number of sockets ready to operate otherwise
      // 
      // Note
      //   The difference with select is that peek doen't populate the read/write connection sets
      inline int peek(double timeout=-1) { return this->peek(true, true, timeout); }
      inline int peekReadable(double timeout=-1) { return this->peek(true, false, timeout); }
      inline int peekWritable(double timeout=-1) { return this->peek(false, true, timeout); }
      
      TCPConnection* accept(Status *status=0);
      TCPConnection* connect(Status *status=0);
      
      // Note: The close methods won't free the Connection objects
      //       Use cleanup method to remove
      // close all connections and the socket itself
      void close();
      // close all connections only
      void closeConnections();
      // close a single connection
      void close(TCPConnection*);
      
      // Cleanup closed connections objects
      // 
      // Arguments
      //   [in] flushPending : Closed connection may still have remaining data in their internal read buffer.
      //                       Set to false, cleanup will keep suchs so that data can still be queried.
      void cleanup(bool flushPending=false);
      
      
    protected:
      
      TCPSocket();
      TCPSocket(const TCPSocket&);
      TCPSocket& operator=(const TCPSocket&);
      
      void setup(TCPConnection *conn);
      bool toTimeval(double ms, struct timeval &tv) const;
      size_t select(bool readable, bool writable, double timeout, Status *status=0);
      int peek(bool readable, bool writable, double timeout, fd_set *readfds=0, fd_set *writefds=0);
      void clearEvents();
    
    protected:
      
      int mMaxConnections;
      
      typedef std::list<TCPConnection*> ConnectionList;
      
      ConnectionList mConnections;
      ConnectionList mReadConnections;
      ConnectionList mWriteConnections;
      ConnectionList::iterator mCurReadConnection;
      ConnectionList::iterator mCurWriteConnection;
      bool mDefaultBlocking;
      bool mDefaultLinger;
      
#ifdef _WIN32
      std::vector<WSAEVENT> mEvents;
      std::vector<ConnectionList::const_iterator> mEventConns;
#endif
  };
  
}

#endif
