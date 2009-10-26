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
#include <exception>
#include <sstream>
#include <algorithm>

namespace gnet {
  
Socket::Socket(unsigned short port) throw(Exception)
  : mHost("localhost", port) {
#ifdef _WIN32
  mFD = INVALID_SOCKET;
#else
  mFD = -1;
#endif
}

Socket::Socket(const Host &host) throw(Exception)
  : mHost(host) {
#ifdef _WIN32
  mFD = INVALID_SOCKET;
#else
  mFD = -1;
#endif
}

Socket::Socket(sock_t fd, const Host &host)
  : mFD(fd), mHost(host) {
}

Socket::~Socket() {
}

bool Socket::isValid() const {
  return (mFD != NULL_SOCKET);
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
  
  while (mConnections.size() > 0) {
    delete mConnections.back();
    mConnections.pop_back();
  }
  
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

void TCPSocket::closeConnection(TCPConnection *conn) {
  if (conn) {
    
    std::vector<TCPConnection*>::iterator it =
      std::find(mConnections.begin(), mConnections.end(), conn);
    
    if (it != mConnections.end()) {
      
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
      
      mConnections.erase(it);
    }
  }
}

TCPConnection* TCPSocket::connect() throw(Exception) {
  
  socklen_t len = sizeof(struct sockaddr);
  
  if (::connect(mFD, mHost, len) < 0) {
    throw Exception("TCPSocket", "Could not connect.", true);
  }
  
  mConnections.push_back(new TCPConnection(this, mFD, mHost));
  return mConnections.back();
}

TCPConnection* TCPSocket::acceptConnection() throw(Exception) {
  
  Host h;
  
  socklen_t len = sizeof(struct sockaddr_in);
  
  sock_t fd = ::accept(mFD, h, &len);
  
  if (fd == NULL_SOCKET) {
    throw Exception("TCPSocket", "Could not accept connetion.", true);
  }
  
  mConnections.push_back(new TCPConnection(this, fd, h));
  return mConnections.back();
}

//void TCPSocket::select() throw(Exception) {
//}

TCPSocket::TCPSocket() {
}

TCPSocket::TCPSocket(const TCPSocket &rhs)
  : Socket(rhs) {
}

TCPSocket& TCPSocket::operator=(const TCPSocket&) {
  return *this;
}

}

