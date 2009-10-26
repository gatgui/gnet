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

#ifdef _WIN32
# pragma warning(disable: 4702)
#endif

#include <string>
#include <iostream>
#include <exception>

#define MASK0  0x00FC0000
#define MASK1  0x0003F000
#define MASK2  0x00000FC0
#define MASK3  0x0000003F

#define SHIFT0 18
#define SHIFT1 12
#define SHIFT2 6
#define SHIFT3 0

class Base64 {

	public:
		
		
		std::string encode(const std::string &in) {
      std::string out = "";
      unsigned long tmp;
      size_t p = 0;
      size_t len = in.length();
      while ((len - p) >= 3) {
        tmp = static_cast<unsigned long>(in[p]) << 16;
        tmp = tmp | static_cast<unsigned long>(in[p+1]) << 8;
        tmp = tmp | static_cast<unsigned long>(in[p+2]);
        out.push_back(msEncTable[static_cast<int>((tmp & MASK0) >> SHIFT0)]);
        out.push_back(msEncTable[static_cast<int>((tmp & MASK1) >> SHIFT1)]);
        out.push_back(msEncTable[static_cast<int>((tmp & MASK2) >> SHIFT2)]);
        out.push_back(msEncTable[static_cast<int>((tmp & MASK3) >> SHIFT3)]);
        p += 3;
      }
      if ((len - p) == 2) {
        tmp = static_cast<unsigned long>(in[p]) << 16;
        tmp = tmp | static_cast<unsigned long>(in[p+1]) << 8;
        out.push_back(msEncTable[static_cast<int>((tmp & MASK0) >> SHIFT0)]);
        out.push_back(msEncTable[static_cast<int>((tmp & MASK1) >> SHIFT1)]);
        out.push_back(msEncTable[static_cast<int>((tmp & MASK2) >> SHIFT2)]);
        out.push_back('=');
      } else if ((len - p) == 1) {
        tmp = static_cast<unsigned long>(in[p]) << 16;
        out.push_back(msEncTable[static_cast<int>((tmp & MASK0) >> SHIFT0)]);
        out.push_back(msEncTable[static_cast<int>((tmp & MASK1) >> SHIFT1)]);
        out.push_back('=');
        out.push_back('=');
      }
      return out;
		}
		
		std::string decode(const std::string &in) {
      std::string out = "";
      unsigned long tmp;
      int npad;
      size_t p = 0;
      size_t len = in.length();
      while ((len - p) > 0) {
        tmp = 0;
        npad = 0;
        for (int i=0; i<4; ++i) {
          char c = in[p+i];
          if (c != '=') {
            const char *f = strchr(msEncTable, static_cast<int>(c));
            if (f == NULL) {
              throw "Invalid base64 encoded string";
            }
            unsigned long index = (unsigned long)(f - msEncTable);
            tmp = tmp | ((index & 0x0000003F) << (6 * (3 - i)));
          } else {
            ++npad;
          }
        }
        if (npad == 0) {
          out.push_back(static_cast<char>((tmp & 0x00FF0000) >> 16));
          out.push_back(static_cast<char>((tmp & 0x0000FF00) >> 8));
          out.push_back(static_cast<char>((tmp & 0x000000FF)));
        } else if (npad == 1) {
          out.push_back(static_cast<char>((tmp & 0x00FF0000) >> 16));
          out.push_back(static_cast<char>((tmp & 0x0000FF00) >> 8));
        } else if (npad == 2) {
          out.push_back(static_cast<char>((tmp & 0x00FF0000) >> 16));
        } else {
          throw "Invalid base64 encoded string";
        }
        p += 4;
      }
      return out;
		}
		
  protected:
    
    static const char* msEncTable;
};

const char * Base64::msEncTable =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << "Usage: base64 <string>" << std::endl;
    return -1;
  }
  Base64 encdec;
  std::string str = argv[2];
  if (strcmp(argv[1], "--decode") == 0) {
    std::string dec = encdec.decode(str);
    std::cout << "\"" << str << "\" -> \"" << dec << "\"" << std::endl;
  } else if (strcmp(argv[1], "--encode") == 0) {
    std::string enc = encdec.encode(str);
    std::cout << "\"" << str << "\" -> \"" << enc << "\"" << std::endl;
  } else {
    std::cout << "Invalid options: " << argv[1] << std::endl;
  }
  return 0;
}
*/
