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

#include <gnet/connection.h>
#include <gnet/socket.h>
#include <gcore/threads.h>
#include <gcore/time.h>

#ifndef _WIN32
# include <signal.h>
class BlockSignal {
public:
  BlockSignal(int sig, bool noop=false)
    : mRestoreMask(!noop) {
    if (!noop) {
      sigset_t mask;
      sigemptyset(&mask);
      sigaddset(&mask, sig);
      sigprocmask(SIG_BLOCK, &mask, &mOldMask);
    }
  }
  ~BlockSignal() {
    if (mRestoreMask) {
      sigprocmask(SIG_BLOCK, &mOldMask, NULL);
    }
  }
private:
  bool mRestoreMask;
  sigset_t mOldMask;
};
#else
class BlockSignal {
public:
  BlockSignal(int, bool=false) {}
  ~BlockSignal() {}
};
#endif

namespace gnet {
  

Connection::Connection()
  : mBufferSize(0), mBuffer(0), mBufferOffset(0) {
  setBufferSize(512);
}

Connection::Connection(sock_t fd)
  : mFD(fd), mBufferSize(0), mBuffer(0) {
  setBufferSize(512);
}

Connection::~Connection() {
  if (mBuffer) {
    if (mBufferOffset > 0) {
      std::cerr << "WARNING: Deleting Connection object that have pending data" << std::endl;
    }
    delete[] mBuffer;
    mBuffer = 0;
    mBufferSize = 0;
    mBufferOffset = 0;
  }
}

char* Connection::alloc(size_t sz, char *bytes) {
  if (bytes) {
    return (char*) realloc((void*)bytes, sz);
  } else {
    return (char*) malloc(sz);
  }
}

void Connection::free(char *&bytes) {
  if (bytes) {
    ::free(bytes);
    bytes = 0;
  }
}

bool Connection::isValid() const {
  return (mFD != NULL_SOCKET);
}

void Connection::invalidate() {
  mFD = NULL_SOCKET;
}

bool Connection::isAlive() const {
  return (mFD != NULL_SOCKET);
}

void Connection::setBufferSize(unsigned long n) {
  if (n > mBufferSize) {
    if (mBuffer) {
      delete[] mBuffer;
      mBuffer = 0;
    }
    mBuffer = new char[n];
    memset(mBuffer, 0, n*sizeof(char));
    mBufferOffset = 0;
  }
  mBufferSize = n;
}

bool Connection::setBlocking(bool blocking) {
  if (!isValid()) {
    return false;
  }
#ifdef _WIN32
  ULONG arg = (blocking ? 0 : 1);
  return (::ioctlsocket(mFD, FIONBIO, &arg) == 0);
#else
  int flags = ::fcntl(mFD, F_GETFL, NULL);
  if (blocking) {
    flags = flags & ~O_NONBLOCK;
  } else {
    flags = flags | O_NONBLOCK;
  }
  return (::fcntl(mFD, F_SETFL, flags) != -1);
#endif
}

bool Connection::setLinger(bool onoff) {
  if (!isValid()) {
    return false;
  }
#ifdef SO_LINGER
  struct linger l;
  socklen_t optlen = sizeof(struct linger);
  if (onoff) {
    // get current value to preserve timeout
    if (::getsockopt(mFD, SOL_SOCKET, SO_LINGER, (char*)&l, &optlen) != 0) {
      return false;
    }
  }
  l.l_onoff = (onoff ? 1 : 0);
  return (::setsockopt(mFD, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(struct linger)) == 0);
#else
  return false;
#endif
}

bool Connection::read(std::string &s, double timeout, Status *status) {
  char *bytes = 0;
  size_t len = 0;
  bool rv = this->read(bytes, len, timeout, status);
  if (bytes != 0) {
    s = bytes;
    this->free(bytes);
  } else {
    s = "";
  }
  return rv;
}

bool Connection::readUntil(const char *until, std::string &s, double timeout, Status *status) {
  char *bytes = 0;
  size_t len = 0;
  bool rv = this->readUntil(until, bytes, len, timeout, status);
  if (bytes != 0) {
    s = bytes;
    this->free(bytes);
  } else {
    s = "";
  }
  return rv;
}

size_t Connection::write(const std::string &s, double timeout, Status *status) {
   return this->write(s.c_str(), s.length(), timeout, status);
}

// ---

TCPConnection::TCPConnection()
  : mSocket(0) {
}

TCPConnection::TCPConnection(TCPSocket *socket, sock_t fd, const Host &host)
  : Connection(fd), mHost(host), mSocket(socket) {
}

TCPConnection::~TCPConnection() {
}

bool TCPConnection::isValid() const {
  return (Connection::isValid() && mSocket != NULL && mSocket->isValid());
}

void TCPConnection::invalidate() {
  Connection::invalidate();
  mSocket = NULL;
}

bool TCPConnection::isAlive() const {
  if (isValid()) {
    char c = '\0';
    // Try to read something from mFD, 0 is returned if connection is closed.
    // This assumes that the connection is not blocking.
    // Use MSG_PEEK to avoid pulling data we're not supposed to
    return (recv(mFD, &c, 1, MSG_PEEK) != 0);
  } else {
    return false;
  }
}

bool TCPConnection::readShutdown() {
  if (isValid()) {
    return (::shutdown(mFD, SHUT_RD) == 0);
  }
  return false;
}

bool TCPConnection::writeShutdown() {
  if (isValid()) {
    return (::shutdown(mFD, SHUT_WR) == 0);
  }
  return false;
}

bool TCPConnection::shutdown() {
  if (isValid()) {
    return (::shutdown(mFD, SHUT_RDWR) == 0);
  }
  return false;
}

bool TCPConnection::read(char *&bytes, size_t &len, double timeout, Status *status) {
  return readUntil(NULL, bytes, len, timeout, status);
}

bool TCPConnection::checkUntil(const char *until, char *in, size_t inlen, char *&out, size_t &outlen) {
  if (until != NULL && in && inlen > 0) {
    // if out is set, in must be a substring
    if (out && (in < out || in + inlen > out + outlen)) {
      return false;
    }
    
    size_t ulen = strlen(until);
    char *found = strstr(in, until);
    
    if (found != NULL) {
      size_t sublen = found + ulen - in;
      size_t rmnlen = inlen - sublen;
      
      if (!out) {
        out = this->alloc(sublen+1);
        outlen = sublen;
        memcpy(out, in, sublen);
        out[sublen] = '\0';
      } else {
        found[ulen] = '\0';
        outlen -= rmnlen;
      }
      
      // Keep any bytes after until in internal buffer
      mBufferOffset = (unsigned long)rmnlen;
      for (size_t i=0; i<rmnlen; ++i) {
        mBuffer[i] = mBuffer[sublen+i];
      }
      mBuffer[rmnlen] = '\0';
      
      return true;
    }
  }
  
  return false;
}

bool TCPConnection::readUntil(const char *until, char *&bytes, size_t &len, double timeout, Status *status) {
  bytes = 0;
  len = 0;
  
  size_t allocated = 0;
  size_t searchOffset = 0;
  
  if (mBufferSize == 0) {
    setBufferSize(512);
  
  } else if (mBufferOffset > 0) {
    len = mBufferOffset;
    bytes = this->alloc(len + 1);
    memcpy(bytes, mBuffer, len);
    bytes[len] = '\0';
    
    mBufferOffset = 0;
    
    allocated = len;
    searchOffset = 0;
  }
  
  if (checkUntil(until, bytes, len, bytes, len)) {
    if (status) {
      status->set(true, NULL);
    }
    return true;
  }
  
  if (!isValid()) {
    if (len > 0) {
      if (status) {
        status->set(true, NULL);
      }
      return (until ? false : true);
    
    } else {
      if (status) {
        status->set(false, "[gnet::TCPConnection::readUntil] Invalid connection.", true);
      }
      return false;
    }
  }
  
  int n = 0;
  bool cont = false;
  gcore::TimeCounter st(gcore::TimeCounter::MilliSeconds);
  
  do {
    
    if (timeout > 0) {
      if (st.elapsed().value() > timeout) {
        if (status) {
          status->set(true, NULL);
        }
        return (until ? false : (len > 0));
      }
    }
    
    // If socket is not set to be non-blocking, we'll be stuck here until
    // data comes in, defeating the purpose of 'timeout'
    // Be sure to call 'setBlocking(false)' before make use of the method with timeout
    // The TCPSocket class will do it on all the TCPConnection intances it creates
    n = recv(mFD, mBuffer+mBufferOffset, mBufferSize-mBufferOffset, 0);
    
    if (n == -1) {
      // There's no guaranty that EWOULDBLOCK == EAGAIN
      int err = sock_errno();
#ifdef _WIN32
      bool retry = (err == WSAEWOULDBLOCK);
#else
      bool retry = (err == EAGAIN || err == EWOULDBLOCK);
#endif
      if (retry) {
        if (timeout == 0) {
          if (status) {
            status->set(true, NULL);
          }
          return (until ? false : (len > 0));
        
        } else {
          if (timeout < 0) {
            // Blocking read
            if (len > 0) {
              // We already have data to return
              if (status) {
                status->set(true, NULL);
              }
              return (until ? false : true);
            } else {
              // Sleep 50ms before trying again
              gcore::Thread::SleepCurrent(50);
            }
          }
          cont = true;
          continue;
        }
      
      } else {
        if (status) {
          status->set(false, "[gnet::TCPConnection::readUntil] Failed to read from socket.", true);
        }
        return (until ? false : (len > 0));
      }
    }
    
    if (n == 0) {
      // Connection closed
      mSocket->close(this);
      if (status) {
        status->set(false, "[gnet::TCPConnection::readUntil] Connection remotely closed.");
      }
      return (until ? false : (len > 0));
    }
    
    cont = (n == int(mBufferSize - mBufferOffset));
    mBufferOffset = 0;
    
    if (bytes == 0) {
      len = n;
      allocated = n;
      bytes = this->alloc(allocated+1);
      memcpy(bytes, mBuffer, n);
      bytes[len] = '\0';
      searchOffset = 0;
      
    } else {
      if ((len + n) >= allocated) {
        while (allocated < (len + n)) {
          allocated <<= 1;
        }
        bytes = this->alloc(allocated+1, bytes);
      }
      memcpy(bytes+len, mBuffer, n);
      searchOffset = len;
      len += n;
      bytes[len] = '\0';
    }
    
    if (checkUntil(until, bytes + searchOffset, size_t(n), bytes, len)) {
      if (status) {
        status->set(true, NULL);
      }
      return true;
    } else {
      if (until) {
        cont = true;
      }
    }
    
  } while (cont);
  
  if (status) {
    status->set(true, NULL);
  }
  return true;
}

size_t TCPConnection::write(const char *bytes, size_t len, double timeout, Status *status) {
  if (!isValid()) {
    if (status) {
      status->set(false, "[gnet::TCPConnection::write] Invalid connection.");
    }
    return 0;
  }
  
  if (len == 0) {
    if (status) {
      status->set(true, NULL);
    }
    return 0;
  }
  
  int offset = 0;
  int remaining = int(len);
  
  gcore::TimeCounter st(gcore::TimeCounter::MilliSeconds);
  
  while (remaining > 0) {
    
    if (timeout > 0) {
      if (st.elapsed().value() > timeout) {
        if (status) {
          status->set(true, NULL);
        }
        return size_t(offset);
      }
    }
    
    // If connection is remotely closed, 'send' will result in a SIGPIPE signal on unix systems
    
    /*
    // Using isAlive seems to respond faster to remove connection close
    // But it can be blocking!
    if (!isAlive()) {
      mSocket->close(this);
      if (status) {
        status->set(false, "[gnet::TCPConnection::write] Connection remotely closed.");
      }
      return size_t(offset);
    }
    int n = send(mFD, bytes+offset, remaining, 0);
    */
    
    int n = -1;
    int flags = 0;
    {
#ifdef MSG_NOSIGNAL
      flags = MSG_NOSIGNAL;
#else // MSG_NOSIGNAL
# ifdef SIGPIPE
      bool noop = false;
#  ifdef SO_NOSIGPIPE
      int nosigpipe = 0;
      socklen_t optlen = sizeof(int);
      if (!getsockopt(mFD, SOL_SOCKET, SO_NOSIGPIPE, &nosigpipe, &optlen)) {
        noop = (nosigpipe != 0);
      }
#  endif // SO_SIGPIPE
      BlockSignal _sbs(SIGPIPE, noop);
# else // SIGPIPE
      // No workaround SIGPIPE signal
# endif // SIGPIPE
#endif // MSG_NOSIGNAL
      n = send(mFD, bytes+offset, remaining, flags);
    }

    if (n == -1) {
      int err = sock_errno();
#ifdef _WIN32
      if (err == WSAEWOULDBLOCK) {
#else
      if (err == EAGAIN || err == EWOULDBLOCK) {
#endif
        if (timeout == 0) {
          if (status) {
            status->set(true, NULL);
          }
          return size_t(offset);
        
        } else {
          if (timeout < 0) {
            // Blocking write -> Sleep 50ms before trying again
            gcore::Thread::SleepCurrent(50);
          }
          continue;
        }
      
      } else {
        // Should notify socket ?
#ifdef _WIN32
        if (err == WSAECONNRESET || err == WSAECONNABORTED) {
#else
        // why did I check for err == 0
        //if (err == 0 || err == EPIPE) {
        if (err == EPIPE) {
#endif
          mSocket->close(this);
          
          if (status) {
            status->set(false, "[gnet::TCPConnection::write] Connection remotely closed.");
          }
        
        } else {
          if (status) {
            status->set(false, "[gnet::TCPConnection::write] Failed to write to socket.", true);
          }
        }
        
        return size_t(offset);
      }
    
    } else {
      remaining -= n;
      offset += n;
    }
  }
  
  if (status) {
    status->set(true, NULL);
  }
  return size_t(offset);
}
  
}

