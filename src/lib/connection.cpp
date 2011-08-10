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
#include <exception>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cerrno>

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
    delete[] mBuffer;
    mBuffer = 0;
    mBufferSize = 0;
    mBufferOffset = 0;
  }
}
  
bool Connection::isValid() const {
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

// ---

TCPConnection::TCPConnection()
  : mSocket(0) {
}

TCPConnection::TCPConnection(TCPSocket *socket, sock_t fd, const Host &host)
  : Connection(fd), mHost(host), mSocket(socket) {
}

TCPConnection::~TCPConnection() {
}

bool TCPConnection::read(char *&bytes, size_t &len, const char *until, int timeout) throw(Exception) {
  if (!isValid()) {
    throw Exception("TCPConnection", "Invalid connections.");
  }
  
  if (mBufferSize == 0) {
    setBufferSize(512);
  }
  
  int n = 0;
  size_t allocated = 0;
  bool full = false;
  size_t searchOffset = 0;
  
  bytes = 0;
  len = 0;
  
#ifdef _DEBUG
  std::cout << "gnet::TCPConnection::read: until \"" << (until ? until : "") << "\"" << std::endl;
#endif
  
  if (until != NULL && mBufferOffset > 0)
  {
    size_t ulen = strlen(until);
    char *found = strstr(mBuffer, until);
    if (found != NULL)
    {
#ifdef _DEBUG
      std::cout << "gnet::TCPConnection::read: until found in remaining buffer" << std::endl;
#endif
      size_t sublen = found + ulen - mBuffer;
      size_t rmnlen = mBufferOffset - sublen;
      bytes = (char*) malloc(sublen+1);
      len = sublen;
      memcpy(bytes, mBuffer, sublen);
      bytes[sublen] = '\0';
      mBufferOffset = (unsigned long)rmnlen;
      for (size_t i=0; i<rmnlen; ++i) {
        mBuffer[i] = mBuffer[sublen+i];
      }
      mBuffer[rmnlen] = '\0';
#ifdef _DEBUG
      std::cout << "gnet::TCPConnection::read: \"" << mBuffer << "\" remains in buffer" << std::endl;
#endif
      return true;
    }
  }
  
  time_t st, et;
  
  st = time(NULL);
  
  do {
    
    if (timeout > 0) {
      et = time(NULL);
      if ((et - st) >= time_t(timeout)) {
        // return current state
        return false;
      }
    }
    
    // flags? will this be blocking? then what about our timeout?
    // => might need to set options on the socket itself for non-blocking read
    n = recv(mFD, mBuffer+mBufferOffset, mBufferSize-mBufferOffset, 0);
    
    if (n == -1) {
      // Should notify socket ?
      if (timeout > 0 && errno == EAGAIN) {
        continue;
      }
      if (bytes) {
         free(bytes);
         bytes = 0;
         len = 0;
      }
      throw Exception("TCPConnection", "Could not read from socket.", true);
    }
    
    if (n == 0) {
      // Connection closed
      if (mSocket->fd() == mFD) {
        mSocket->invalidate();
      }
      mFD = NULL_SOCKET;
      if (bytes) {
         free(bytes);
         bytes = 0;
         len = 0;
      }
      throw Exception("TCPConnection", "Connection was remotely closed.");
    }
    
    full = (n == int(mBufferSize - mBufferOffset));
    mBufferOffset = 0;
    
#ifdef _DEBUG
    std::cout << "gnet::TCPConnection::read: received " << n << " characters" << std::endl;
#endif
    
    if (bytes == 0) {
      len = allocated = n;
      bytes = (char*) malloc(allocated+1);
      memcpy(bytes, mBuffer, n);
      bytes[len] = '\0';
      searchOffset = 0;
      
    } else {
      if ((len + n) >= allocated) {
        allocated <<= 1;
        bytes = (char*) realloc(bytes, allocated+1);
      }
      memcpy(bytes+len, mBuffer, n);
      searchOffset = len;
      len += n;
      bytes[len] = '\0';
    }
    
#ifdef _DEBUG
    std::cout << "gnet::TCPConnection::read: \"" << (bytes+len-n) << "\"" << std::endl;
#endif
        
    if (until != NULL) {
      size_t ulen = strlen(until);
      char *found = strstr(bytes+searchOffset, until);
      if (found != NULL) {
        size_t sublen = found + ulen - (bytes + searchOffset);
        size_t rmnlen = n - sublen;
        found[ulen] = '\0';
        len -= rmnlen;
        // keep remaining bits in buffer
        mBufferOffset = (unsigned long)rmnlen;
        for (size_t i=0; i<rmnlen; ++i) {
          mBuffer[i] = mBuffer[sublen+i];
        }
        mBuffer[rmnlen] = '\0';
#ifdef _DEBUG
        std::cout << "gnet::TCPConnection::read: \"" << mBuffer << "\" remains in buffer" << std::endl;
#endif
        return true;
      } else {
#ifdef _DEBUG
        std::cout << "gnet::TCPConnection::read: until not found, continue reading" << std::endl;
#endif
        full = true;
      }
    }
    
  } while (full);
  
  return true;
}

void TCPConnection::write(const char *bytes, size_t len) throw(Exception) {
  if (!isValid()) {
    throw Exception("TCPConnection", "Invalid connection.");
  }
  
  // flags ?
  int n = send(mFD, bytes, int(len), 0);
  
  if (n == -1) {
    if (errno != 0) {
      // Should notify socket ?
      throw Exception("TCPConnection", "Could not write to socket.", true);
      
    } else {
      if (mSocket->fd() == mFD) {
        mSocket->invalidate();
      }
      mFD = NULL_SOCKET;
      throw Exception("TCPConnection", "Connection was remotely closed.");
    }
  }
  
}
  
}

