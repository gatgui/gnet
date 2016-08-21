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

#include <gnet/socket.h>
#include <gcore/time.h>
#include <exception>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace gnet {
  
Socket::Socket(unsigned short port) throw(Exception)
  : mFD(NULL_SOCKET), mHost("localhost", port) {
}

Socket::Socket(const Host &host) throw(Exception)
  : mFD(NULL_SOCKET), mHost(host) {
}

Socket::Socket(sock_t fd, const Host &host)
  : mFD(fd), mHost(host) {
}

Socket::~Socket() {
}

bool Socket::isValid() const {
  return (mFD != NULL_SOCKET);
}

void Socket::invalidate() {
  mFD = NULL_SOCKET;
}

Socket::Socket() {
}

Socket::Socket(const Socket&) {
}

Socket& Socket::operator=(const Socket&) {
  return *this;
}

// ---

TCPSocket::TCPSocket(unsigned short port) throw(Exception)
  : Socket(port) {
  mFD = ::socket(AF_INET, SOCK_STREAM, 0);
  mCurReadConnection = mReadConnections.end();
  mCurWriteConnection = mWriteConnections.end();
}

TCPSocket::TCPSocket(const Host &host) throw(Exception)
  : Socket(host) {
  mFD = ::socket(AF_INET, SOCK_STREAM, 0);
  mCurReadConnection = mReadConnections.end();
  mCurWriteConnection = mWriteConnections.end();
}

TCPSocket::~TCPSocket() {
  closeAll();
  
  if (isValid()) {
#ifdef _WIN32
    closesocket(mFD);
#else
    ::close(mFD);
#endif
  }
}

void TCPSocket::bind() throw(Exception) {
  if (::bind(mFD, mHost, sizeof(struct sockaddr)) < 0) {
    throw Exception("TCPSocket", "Could not bind socket.", true);
  }
}

void TCPSocket::listen(int maxConnections) throw(Exception) {
  if (::listen(mFD, maxConnections) == -1) {
    throw Exception("TCPSocket", "Cannot listen on socket.", true);
  }
  mMaxConnections = maxConnections;
}

void TCPSocket::bindAndListen(int maxConnections) throw(Exception) {
  this->bind();
  this->listen(maxConnections);
}

void TCPSocket::closeAll() {
  for (ConnectionList::iterator it=mConnections.begin(); it!=mConnections.end(); ++it) {
    TCPConnection *conn = *it;
    if (conn->isValid() && conn->fd() != mFD) {
#ifdef _WIN32
      closesocket(mFD);
#else
      ::close(mFD);
#endif
    }
    delete conn;
  }
  
  mConnections.clear();
  mReadConnections.clear();
  mWriteConnections.clear();
}

void TCPSocket::close(TCPConnection *conn) {
  if (conn) {
    
    ConnectionList::iterator it = std::find(mConnections.begin(), mConnections.end(), conn);
    
    if (it != mConnections.end()) {
      
      mConnections.erase(it);
      
      it = std::find(mReadConnections.begin(), mReadConnections.end(), conn);
      if (it != mReadConnections.end()) {
        bool upd = (mCurReadConnection == it);
        it = mReadConnections.erase(it);
        if (upd) {
          mCurReadConnection = it;
        }
      }
      
      it = std::find(mWriteConnections.begin(), mWriteConnections.end(), conn);
      if (it != mWriteConnections.end()) {
        bool upd = (mCurWriteConnection == it);
        it = mWriteConnections.erase(it);
        if (upd) {
          mCurWriteConnection = it;
        }
      }
      
      // do not close connection that have same id
      if (conn->isValid() && conn->fd() != fd()) {
#ifdef _WIN32
        closesocket(conn->fd());
#else
        ::close(conn->fd());
#endif
      }
      
      // destroy it
      delete conn;
    }
  }
}

TCPConnection* TCPSocket::connect() throw(Exception) {
  
  socklen_t len = sizeof(struct sockaddr);
  
  if (::connect(mFD, mHost, len) < 0) {
    throw Exception("TCPSocket", "Could not connect.", true);
  }
  
  mConnections.push_back(new TCPConnection(this, mFD, mHost));
  
  TCPConnection *conn = mConnections.back();
  conn->setBlocking(false);
  
  return conn;
}

TCPConnection* TCPSocket::accept() throw(Exception) {
  
  Host h;
  
  socklen_t len = sizeof(struct sockaddr_in);
  
  sock_t fd = ::accept(mFD, h, &len);
  
  if (fd == NULL_SOCKET) {
    throw Exception("TCPSocket", "Could not accept connetion.", true);
  }
  
  mConnections.push_back(new TCPConnection(this, fd, h));
  
  TCPConnection *conn = mConnections.back();
  conn->setBlocking(false);
  
  return conn;
}

size_t TCPSocket::select(bool readable, bool writable, double timeout) throw(Exception) {
  struct timeval _tv;
  struct timeval *tv = 0;
  int curfd, maxfd = -1;
  fd_set _readfds;
  fd_set _writefds;
  fd_set *readfds = (readable ? &_readfds : NULL);
  fd_set *writefds = (writable ? &_writefds : NULL);
  
  if (!readable && !writable) {
    return 0;
  }
  
  if (timeout >= 0) {
    if (timeout > 0) {
      double total = gcore::TimeCounter::ConvertUnits(timeout, gcore::TimeCounter::MilliSeconds, gcore::TimeCounter::Seconds);
      double secs = floor(total);
      double remain = total - secs;
      double usecs = floor(0.5 + gcore::TimeCounter::ConvertUnits(remain, gcore::TimeCounter::Seconds, gcore::TimeCounter::MicroSeconds));
      
      _tv.tv_sec = (long) secs;
      _tv.tv_usec = (long) usecs;
      
    } else {
      _tv.tv_sec = 0;
      _tv.tv_usec = 0;
    }
    
    tv = &_tv;
  }
  
  if (readable) {
    FD_ZERO(readfds);
    FD_SET(mFD, readfds);
    maxfd = mFD;
  }
  
  if (writable) {
    FD_ZERO(writefds);
    FD_SET(mFD, writefds);
    maxfd = mFD;
  }
  
  for (ConnectionList::iterator it=mConnections.begin(); it!=mConnections.end(); ++it) {
    TCPConnection *conn = *it;
    
    if (conn->isValid()) {
      if (readable) {
        FD_SET(conn->fd(), readfds);
      }
      
      if (writable) {
        FD_SET(conn->fd(), writefds);
      }
      
      curfd = (int) conn->fd();
      
      if (curfd > maxfd) {
        maxfd = curfd;
      }
    }
  }
  
  mReadConnections.clear();
  mWriteConnections.clear();
  mCurReadConnection = mReadConnections.begin();
  mCurWriteConnection = mWriteConnections.begin();
    
  int rv = ::select(maxfd+1, readfds, writefds, NULL, tv);
  
  if (rv == -1) {
    throw Exception("TCPSocket", "", true);
  }
  
  if (readable) {
    if (FD_ISSET(mFD, readfds)) {
      this->accept();
    }
  }
  
  if (writable) {
    if (FD_ISSET(mFD, writefds)) {
      // Nothing special to do here?
    }
  }
  
  for (ConnectionList::iterator it=mConnections.begin(); it!=mConnections.end(); ++it) {
    TCPConnection *conn = *it;
    if (!conn->isValid()) {
      continue;
    }
    if (readable) {
      if (FD_ISSET(conn->fd(), readfds)) {
        mReadConnections.push_back(conn);
      }
    }
    if (writable) {
      if (FD_ISSET(conn->fd(), writefds)) {
        mWriteConnections.push_back(conn);
      }
    }
  }
  
  mCurReadConnection = mReadConnections.begin();
  mCurWriteConnection = mWriteConnections.begin();
  
  return (mReadConnections.size() + mWriteConnections.size());
}

TCPConnection* TCPSocket::nextReadable() {
  TCPConnection *rv = NULL;
  while (mCurReadConnection != mReadConnections.end()) {
    TCPConnection *conn = *mCurReadConnection;
    if (conn->isValid()) {
      rv = conn;
    }
    ++mCurReadConnection;
    if (rv) {
      break;
    }
  }
  return rv;
}

TCPConnection* TCPSocket::nextWritable() {
  TCPConnection *rv = NULL;
  while (mCurWriteConnection != mWriteConnections.end()) {
    TCPConnection *conn = *mCurWriteConnection;
    if (conn->isValid()) {
      rv = conn;
    }
    ++mCurWriteConnection;
    if (rv) {
      break;
    }
  }
  return rv;
}

TCPSocket::TCPSocket() {
}

TCPSocket::TCPSocket(const TCPSocket &rhs)
  : Socket(rhs) {
}

TCPSocket& TCPSocket::operator=(const TCPSocket&) {
  return *this;
}

}

