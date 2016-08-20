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
      std::cout << "DONE" << std::endl;
      
      std::cout << "Read data from connection...";
      conn->read(buffer, len);
      std::cout << "DONE: \"" << buffer << "\"" << std::endl;
      
      if (!strcmp(buffer, "QUIT")) {
        end = true;
      }
      
      free(buffer);
      
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
