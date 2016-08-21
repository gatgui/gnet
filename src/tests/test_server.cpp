#include <gcore/all.h>
#include <gnet/all.h>


int main(int, char**) {
  
  gnet::Initialize();

  gnet::TCPSocket socket(8080);
  
  socket.bindAndListen(5);
  
  bool end = false;
  
  try {
    while (!end) {
      char *buffer = 0;
      size_t len = 0;
      
      std::cout << "Accept new connection...";
      gnet::TCPConnection *conn = socket.accept();
      if (!conn) {
        std::cout << "FAILED" << std::endl;
        continue;
      }
      std::cout << "DONE" << std::endl;
      
      std::cout << "Read data from connection...";
      bool rv = conn->read(buffer, len, -1);
      std::cout << "DONE: (" << rv << ")" << std::endl;
      if (buffer) {
        std::cout << "\"" << buffer << "\"" << std::endl;
        if (strstr(buffer, "QUIT") != NULL) {
          end = true;
        }
        free(buffer);
      } else {
        std::cout << "<null>" << std::endl;
      }
      
      std::cout << "Close connection...";
      socket.close(conn);
      std::cout << "DONE" << std::endl;
    }
    
  } catch (gnet::Exception &e) {
    
    std::cout << e.what() << std::endl;
  }
  
  gnet::Uninitialize();
  
  return 0;
}
