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
  
Socket::Socket(unsigned short port, Status *status)
  : mFD(NULL_SOCKET), mHost("localhost", port, status) {
}

Socket::Socket(const Host &host, Status *status)
  : mFD(NULL_SOCKET), mHost(host) {
  if (status) {
    status->set(true, NULL);
  }
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

TCPSocket::TCPSocket(unsigned short port, Status *status)
  : Socket(port, status)
  , mDefaultBlocking(false)
  , mDefaultLinger(true) {
  mFD = ::socket(AF_INET, SOCK_STREAM, 0);
  if (status) {
    if (mFD == NULL_SOCKET) {
      status->set(false, "[gnet::TCPSocket]", true);
    }
  }
  mCurReadConnection = mReadConnections.end();
  mCurWriteConnection = mWriteConnections.end();
}

TCPSocket::TCPSocket(const Host &host, Status *status)
  : Socket(host, status)
  , mDefaultBlocking(false)
  , mDefaultLinger(true) {
  mFD = ::socket(AF_INET, SOCK_STREAM, 0);
  if (status) {
    if (mFD == NULL_SOCKET) {
      status->set(false, "[gnet::TCPSocket]", true);
    }
  }
  mCurReadConnection = mReadConnections.end();
  mCurWriteConnection = mWriteConnections.end();
}

TCPSocket::~TCPSocket() {
  close();
  // 'close' doesn't destroy Connection objects (on purpose)
  for (ConnectionList::iterator it=mConnections.begin(); it!=mConnections.end(); ++it) {
    delete *it;
  }
  mConnections.clear();
}

Status TCPSocket::bind() {
  return Status(::bind(mFD, mHost, sizeof(struct sockaddr)) == 0, "[gnet::TCPSocket::bind]", true);
}

Status TCPSocket::listen(int maxConnections) {
  return Status(::listen(mFD, maxConnections) == 0, "[gnet::TCPSocket::listen]", true);
}

Status TCPSocket::bindAndListen(int maxConnections) {
  Status stat = this->bind();
  if (stat) {
    stat = this->listen(maxConnections);
  }
  return stat;
}

void TCPSocket::clearEvents() {
#ifdef _WIN32
  for (size_t i=0; i<mEvents.size(); ++i) {
    WSACloseEvent(mEvents[i]);
  }
  mEvents.clear();
  mEventConns.clear();
#endif
}

void TCPSocket::close() {
  closeConnections();
  
  if (isValid()) {
    ::shutdown(mFD, SHUT_RDWR);
    sock_close(mFD);
    invalidate();
  }
}

void TCPSocket::closeConnections() {
  for (ConnectionList::iterator it=mConnections.begin(); it!=mConnections.end(); ++it) {
    close(*it);
  }
  mReadConnections.clear();
  mWriteConnections.clear();
  mCurReadConnection = mReadConnections.end();
  mCurWriteConnection = mWriteConnections.end();
  clearEvents();
}

void TCPSocket::close(TCPConnection *conn) {
  if (conn && conn->isValid()) {
    bool self = (conn->fd() == mFD);
    
    conn->shutdown();
    sock_close(conn->fd());
    
    conn->invalidate();
    if (self) {
      invalidate();
    }
  }
}

void TCPSocket::cleanup() {
  ConnectionList::iterator it1 = mConnections.begin();
  ConnectionList::iterator it2;
  
  while (it1 != mConnections.end()) {
    TCPConnection *conn = *it1;
    
    if (!conn->isValid() && !conn->hasPendingData()) {
      // Remove from readable connections
      it2 = std::find(mReadConnections.begin(), mReadConnections.end(), conn);
      if (it2 != mReadConnections.end()) {
        bool upd = (mCurReadConnection == it2);
        it2 = mReadConnections.erase(it2);
        if (upd) {
          mCurReadConnection = it2;
        }
      }
      
      // Remove from writable connections
      it2 = std::find(mWriteConnections.begin(), mWriteConnections.end(), conn);
      if (it2 != mWriteConnections.end()) {
        bool upd = (mCurWriteConnection == it2);
        it2 = mWriteConnections.erase(it2);
        if (upd) {
          mCurWriteConnection = it2;
        }
      }
      
      // Remove from connections
      it1 = mConnections.erase(it1);
      
      // Delete object
      delete conn;
    
    } else {
      ++it1;
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
  conn->setBlocking(mDefaultBlocking);
  conn->setLinger(mDefaultLinger);
#ifdef SO_NOSIGPIPE
  int nosigpipe = 1;
  setsockopt(mFD, SOL_SOCKET, SO_NOSIGPIPE, &nosigpipe, sizeof(int));
#endif
}

TCPConnection* TCPSocket::connect(Status *status) {
  
  cleanup();
  
  socklen_t len = sizeof(struct sockaddr);
  
  if (::connect(mFD, mHost, len) != 0) {
    if (status) {
      status->set(false, "[gnet::TCPSocket::connect] Failed to establish connection.", true);
    }
    return 0;
  }
  
  mConnections.push_back(new TCPConnection(this, mFD, mHost));
  
  TCPConnection *conn = mConnections.back();
  setup(conn);
  
  if (status) {
    status->set(true, NULL);
  }
  
  return conn;
}

TCPConnection* TCPSocket::accept(Status *status) {
  
  cleanup();
  
  Host h;
  
  socklen_t len = sizeof(struct sockaddr_in);
  
  sock_t fd = ::accept(mFD, h, &len);
  
  if (fd == NULL_SOCKET) {
    if (status) {
      status->set(false, "[gnet::TCPSocket::accept] Failed to accept connection.", true);
    }
    return 0;
  }
  
  mConnections.push_back(new TCPConnection(this, fd, h));
  
  TCPConnection *conn = mConnections.back();
  setup(conn);
  
  if (status) {
    status->set(true, NULL);
  }
  
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
  
  int ret = 0;
  
  //cleanup();
  
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
  
  ret = ::select(maxfd+1, (readable ? readfds : NULL), (writable ? writefds : NULL), NULL, tv);
  
#endif
  
  // should add connections when pending data in their internal buffer
  // even if invalid or not ready to read
  if (ret != -1 && readable) {
    for (ConnectionList::const_iterator it=mConnections.begin(); it!=mConnections.end(); ++it) {
      TCPConnection *conn = *it;
      if (conn->hasPendingData()) {
        if (conn->isValid()) {
          if (FD_ISSET(conn->fd(), readfds)) {
            // already selected
          } else {
            FD_SET(conn->fd(), readfds);
            ++ret;
          }
        } else {
          ++ret;
        }
      }
    }
  }
  
  return ret;
}

size_t TCPSocket::select(bool readable, bool writable, double timeout, Status *status) {
  mReadConnections.clear();
  mWriteConnections.clear();
  mCurReadConnection = mReadConnections.end();
  mCurWriteConnection = mWriteConnections.end();
  
  if (!readable && !writable) {
    if (status) {
      status->set(true, NULL);
    }
    return 0;
  }
  
  fd_set readfds, writefds;
    
  int rv = peek(readable, writable, timeout, &readfds, &writefds);
  
  if (rv == -1) {
    if (status) {
      status->set(false, "[gnet::TCPSocket::select]", true);
    }
    return 0;
  }
  
  if (readable) {
    if (FD_ISSET(mFD, &readfds)) {
      // Note: this could fail!
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
      if (readable && conn->hasPendingData()) {
        mReadConnections.push_back(conn);
      }
      continue;
    }
    if (readable) {
      if (FD_ISSET(conn->fd(), &readfds) || conn->hasPendingData()) {
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
  
  if (status) {
    status->set(true, NULL);
  }
  
  return (mReadConnections.size() + mWriteConnections.size());
}

TCPConnection* TCPSocket::nextReadable() {
  TCPConnection *rv = NULL;
  while (mCurReadConnection != mReadConnections.end()) {
    TCPConnection *conn = *mCurReadConnection;
    if (conn->isValid() || conn->hasPendingData()) {
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

