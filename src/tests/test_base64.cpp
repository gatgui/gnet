#include <gnet/gnet>

int main(int argc, char **argv) {
  
  gnet::Base64 encdec;
  
  if (argc == 1) {
    float v0[4] = {-100.2f, 25.2349f, -0.291625f, 0.0001f};
    float v1[4] = {0, 0, 0, 0};
    
    std::cout << v0[0] << ", " << v0[1] << ", " << v0[2] << ", " << v0[3] << std::endl;
    
    std::string es = encdec.encode(v0, 4*sizeof(float));
    std::cout << es << std::endl;
    
    encdec.decode(es, v1, 4*sizeof(float));
    std::cout << v1[0] << ", " << v1[1] << ", " << v1[2] << ", " << v1[3] << std::endl;
    
    return 0;
  }
  
  if (argc != 3) {
    std::cout << "Usage: base64 <string>" << std::endl;
    return -1;
  }
  
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
