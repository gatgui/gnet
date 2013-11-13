/*

Copyright (C) 2013  Gaetan Guidet

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

#ifndef __gnet_base64_h_
#define __gnet_base64_h_

#include <gnet/config.h>
#include <string>

namespace gnet {
  
  class GNET_API Base64 {
    public:
    
      size_t encodeLength(size_t inlen) const;
      std::string encode(const void *data, size_t len) const;
      std::string encode(const std::string &in) const;
      
      size_t decodeLength(const char *in, size_t inlen) const;
      size_t decode(const std::string &in, void *data, size_t maxlen) const;
      std::string decode(const std::string &in) const;
  };
}

#endif
