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
  : Socket(port)
  , mDefaultBlocking(false)
  , mDefaultLinger(true) {
  mFD = ::socket(AF_INET, SOCK_STREAM, 0);
  mCurReadConnection = mReadConnections.end();
  mCurWriteConnection = mWriteConnections.end();
}

TCPSocket::TCPSocket(const Host &host) throw(Exception)
  : Socket(host)
  , mDefaultBlocking(false)
  , mDefaultLinger(true) {
  mFD = ::socket(AF_INET, SOCK_STREAM, 0);
  mCurReadConnection = mReadConnections.end();
  mCurWriteConnection = mWriteConnections.end();
}

TCPSocket::~TCPSocket() {
  closeAll();
  if (isValid()) {
    ::shutdown(mFD, SHUT_RDWR);
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
    if (conn->isValid()) {
      if (conn->fd() != mFD) {
        // mFD is a listening socket
        conn->shutdown();
#ifdef _WIN32
        closesocket(conn->fd());
#else
        ::close(conn->fd());
#endif
      } else {
        // mFD is a single connection socket
      }
    }
    delete conn;
  }
  
  mConnections.clear();
  mReadConnections.clear();
  mWriteConnections.clear();
  
#ifdef _WIN32
  clearEvents();
#endif
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
      if (conn->isValid()) {
        if (conn->fd() != fd()) {
          // mFD is a listening socket
          conn->shutdown();
#ifdef _WIN32
          closesocket(conn->fd());
#else
          ::close(conn->fd());
#endif
        } else {
          // mFD is a single connection socket
        }
      }
      
      // destroy it
      delete conn;
    }
  }
}

void TCPSocket::setDefaultBlocking(bool blocking) {
  mDefaultBlocking = blocking;
}

void TCPSocket::setDefaultLinger(bool linger) {
  mDefaultLinger = linger;
}

void TCPSocket::setup(TCPConnection *conn) {
  
  // may want to expose the setup values
  
  conn->setBlocking(mDefaultBlocking);
  
  conn->setLinger(mDefaultLinger);
  
#ifdef SO_NOSIGPIPE
  int nosigpipe = 1;
  setsockopt(mFD, SOL_SOCKET, SO_NOSIGPIPE, &nosigpipe, sizeof(int));
#endif
}

TCPConnection* TCPSocket::connect() throw(Exception) {
  
  socklen_t len = sizeof(struct sockaddr);
  
  if (::connect(mFD, mHost, len) < 0) {
    throw Exception("TCPSocket", "Could not connect.", true);
  }
  
  mConnections.push_back(new TCPConnection(this, mFD, mHost));
  
  TCPConnection *conn = mConnections.back();
  setup(conn);
  
  return conn;
}

TCPConnection* TCPSocket::accept() throw(Exception) {
  
  Host h;
  
  socklen_t len = sizeof(struct sockaddr_in);
  
  sock_t fd = ::accept(mFD, h, &len);
  
  if (fd == NULL_SOCKET) {
    throw Exception("TCPSocket", "Could not accept connection.", true);
  }
  
  mConnections.push_back(new TCPConnection(this, fd, h));
  
  TCPConnection *conn = mConnections.back();
  setup(conn);
  
  return conn;
}

bool TCPSocket::toTimeval(double ms, struct timeval &tv) const {
  if (ms < 0) {
    return false;
  }
  
  if (ms > 0) {
    double total = gcore::TimeCounter::ConvertUnits(ms, gcore::TimeCounter::MilliSeconds, gcore::TimeCounter::Seconds);
    double secs = floor(total);
    double remain = total - secs;
    double usecs = floor(0.5 + gcore::TimeCounter::ConvertUnits(remain, gcore::TimeCounter::Seconds, gcore::TimeCounter::MicroSeconds));
    
    tv.tv_sec = (long) secs;
    tv.tv_usec = (long) usecs;
    
  } else {
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
  }
  
  return true;
}

#ifdef _WIN32
void TCPSocket::clearEvents() {
  for (size_t i=0; i<mEvents.size(); ++i) {
    WSACloseEvent(mEvents[i]);
  }
  mEvents.clear();
  mEventConns.clear();
}
#endif

int TCPSocket::peek(bool readable, bool writable, double timeout, fd_set *readfds, fd_set *writefds) {
  fd_set _readfds;
  fd_set _writefds;
  
  if (!readable && !writable) {
    return 0;
  }
  
  if (readable && !readfds) {
    readfds = &_readfds;
  }
  
  if (writable && !writefds) {
    writefds = &_writefds;
  }
  
#ifdef _WIN32
  
  // select() on windows would just not work
  // => WSAENOBUFS error right away
  
  if (readable) {
    FD_ZERO(readfds);
  }
  
  if (writable) {
    FD_ZERO(writefds);
  }
  
  ConnectionList::const_iterator eit = mConnections.end();
  
  clearEvents();
  
  mEvents.reserve(1 + mConnections.size());
  mEventConns.reserve(1 + mConnections.size());
  
  if (readable) {
    mEventConns.push_back(eit);
    mEvents.push_back(WSACreateEvent());
    WSAEventSelect(mFD, mEvents.back(), FD_ACCEPT|FD_CLOSE); // FD_CONNECT
  }
  
  for (ConnectionList::const_iterator it=mConnections.begin(); it!=eit; ++it) {
    TCPConnection *conn = *it;
    if (conn->isValid()) {
      long flags = FD_CLOSE;
      if (readable) {
        flags = flags | FD_READ;
      }
      if (writable) {
        flags = flags | FD_WRITE;
      }
      mEventConns.push_back(it);
      mEvents.push_back(WSACreateEvent());
      WSAEventSelect(conn->fd(), mEvents.back(), flags);
    }
  }
  
  DWORD sz = (DWORD) mEvents.size();
  if (sz == 0) {
    return 0;
  }
  
  DWORD to = (timeout < 0 ? WSA_INFINITE : (DWORD) floor(timeout + 0.5));
  DWORD rv = WSAWaitForMultipleEvents(sz, &mEvents[0], FALSE, to, FALSE);
  int ret = 0;
  
  if (rv == WSA_WAIT_FAILED) {
    ret = -1;
  
  } else if (timeout >= 0 && rv == WSA_WAIT_TIMEOUT) {
    ret = 0;
  
  } else if (rv >= WSA_WAIT_EVENT_0 && rv < (WSA_WAIT_EVENT_0 + sz)) {
    size_t fidx = rv - WSA_WAIT_EVENT_0;
    ConnectionList::const_iterator cit;
    WSANETWORKEVENTS nevts;
    
    for (size_t eidx=fidx; eidx<mEvents.size(); ++eidx) {
      rv = WSAWaitForMultipleEvents(1, &mEvents[eidx], TRUE, 0, FALSE);
      if (rv != WSA_WAIT_FAILED) {
        cit = mEventConns[eidx];
        sock_t fd = (cit == eit ? mFD : (*cit)->fd());
        WSAEnumNetworkEvents(fd, mEvents[eidx], &nevts);
        if (readable && (nevts.lNetworkEvents & (FD_READ|FD_ACCEPT|FD_CLOSE)) != 0) {
          FD_SET(fd, readfds);
          ++ret;
        }
        if (writable && (nevts.lNetworkEvents & FD_WRITE) != 0) {
          FD_SET(fd, writefds);
          ++ret;
        }
      }
    }
  
  } else {
    // unhandled error
    ret = -1;
  }
  
  // for (size_t i=0; i<mEvents.size(); ++i) {
  //   WSACloseEvent(mEvents[i]);
  // }
  
  return ret;
  
#else
  
  struct timeval _tv;
  struct timeval *tv = 0;
  if (toTimeval(timeout, _tv)) {
    tv = &_tv;
  }
  
  int curfd, maxfd = -1;
  if (readable) {
    FD_ZERO(readfds);
    FD_SET(mFD, readfds);
    maxfd = int(mFD);
  }
  
  if (writable) {
    FD_ZERO(writefds);
    FD_SET(mFD, writefds);
    maxfd = int(mFD);
  }
  
  for (ConnectionList::const_iterator it=mConnections.begin(); it!=mConnections.end(); ++it) {
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
  
  return ::select(maxfd+1, readfds, writefds, NULL, tv);
  
#endif
}

size_t TCPSocket::select(bool readable, bool writable, double timeout) throw(Exception) {
  mReadConnections.clear();
  mWriteConnections.clear();
  mCurReadConnection = mReadConnections.begin();
  mCurWriteConnection = mWriteConnections.begin();
  
  if (!readable && !writable) {
    return 0;
  }
  
  fd_set readfds, writefds;
    
  int rv = peek(readable, writable, timeout, &readfds, &writefds);
  
  if (rv == -1) {
    throw Exception("TCPSocket", "Select failed", true);
  }
  
  if (readable) {
    if (FD_ISSET(mFD, &readfds)) {
      this->accept();
    }
  }
  
  if (writable) {
    if (FD_ISSET(mFD, &writefds)) {
      // Nothing special to do here?
    }
  }
  
  for (ConnectionList::iterator it=mConnections.begin(); it!=mConnections.end(); ++it) {
    TCPConnection *conn = *it;
    if (!conn->isValid()) {
      continue;
    }
    if (readable) {
      if (FD_ISSET(conn->fd(), &readfds)) {
        mReadConnections.push_back(conn);
      }
    }
    if (writable) {
      if (FD_ISSET(conn->fd(), &writefds)) {
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

