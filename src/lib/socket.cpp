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
}

TCPSocket::TCPSocket(const Host &host) throw(Exception)
  : Socket(host) {
  mFD = ::socket(AF_INET, SOCK_STREAM, 0);
}

TCPSocket::~TCPSocket() {
  
  for (size_t i=0; i<mConnections.size(); ++i) {
    delete mConnections[i];
  }
  mConnections.clear();
  
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

void TCPSocket::close(TCPConnection *conn) {
  if (conn) {
    
    std::vector<TCPConnection*>::iterator it =
      std::find(mConnections.begin(), mConnections.end(), conn);
    
    if (it != mConnections.end()) {
      
      mConnections.erase(it);
      
      it = std::find(mReadyConnections.begin(), mReadyConnections.end(), conn);
      if (it != mReadyConnections.end()) {
        mReadyConnections.erase(it);
      }
      
      // do not close connection that have same id
      if (conn->fd() != NULL_SOCKET && conn->fd() != fd()) {
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

size_t TCPSocket::select(double timeout) throw(Exception) {
  struct timeval _tv;
  struct timeval *tv = 0;
  
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
  
  int curfd, maxfd = (int) mFD;
  
  FD_ZERO(&mRead);
  FD_SET(mFD, &mRead);
  
  for (size_t i=0; i<mConnections.size(); ++i) {
    TCPConnection *conn = mConnections[i];
    if (conn->isValid()) {
      FD_SET(conn->fd(), &mRead);
      curfd = (int) conn->fd();
      if (curfd > maxfd) {
        maxfd = curfd;
      }
    }
  }
  
  mReadyConnections.clear();
  
  int rv = ::select(maxfd+1, &mRead, NULL, NULL, tv);
  
  if (rv == -1) {
    throw Exception("TCPSocket", "", true);
  
  } else if (rv > 0) {
    if (FD_ISSET(mFD, &mRead)) {
      // new connection, accept won't block
      TCPConnection *conn = this->accept();
      mReadyConnections.push_back(conn);
    }
    
    for (size_t i=0; i<mConnections.size(); ++i) {
      TCPConnection *conn = mConnections[i];
      if (conn->isValid() && FD_ISSET(conn->fd(), &mRead)) {
        mReadyConnections.push_back(conn);
      }
    }
  }
  
  return mReadyConnections.size();
}

TCPConnection* TCPSocket::next(TCPConnection *prevConn) {
  TCPConnection *nextConn = NULL;
  std::vector<TCPConnection*>::iterator it;
  
  if (!prevConn) {
    if (mReadyConnections.size() == 0) {
      return NULL;
    
    } else {
      it = mReadyConnections.begin();
    }
  
  } else {
    it = std::find(mReadyConnections.begin(), mReadyConnections.end(), prevConn);
    if (it == mReadyConnections.end()) {
      return NULL;
    } else {
      ++it;
    }
  }
  
  while (it != mReadyConnections.end()) {
    nextConn = *it;
    if (nextConn->isValid()) {
      return nextConn;
    }
    ++it;
  }
  
  return NULL;
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

