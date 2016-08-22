#include <gcore/all.h>
#include <gnet/all.h>

int ReadStdin() {
  bool end = false;
  char rdbuf[512];
  while (!end) {
    if (fgets(rdbuf, 512, stdin)) {
      std::cout << "STDIN: \"" << rdbuf << "\"" << std::endl;
      if (!strncmp(rdbuf, "QUIT", 4)) {
        std::cout << "Read 'QUIT'" << std::endl;
        end = true;
      }
    }
  }
  return 0;
}

int main(int, char**) {
  
  gnet::Initialize();
  
  // Create a new scope so that socket destructor is called before gnet::Uninitialize
  {
    gnet::TCPSocket socket(8080);
    
    try {
      socket.bindAndListen(5);
    } catch (std::exception &e) {
      std::cout << e.what() << std::endl;
      gnet::Uninitialize();
      return 1;
    }
    
    gcore::Thread thr(&ReadStdin, NULL, true);
    
    try {
      while (thr.running()) {
        char *buffer = 0;
        size_t len = 0;
        
        std::cout << "Accept new connection..." << std::endl;
        
        gnet::TCPConnection *conn = 0;
        try {
          conn = socket.accept();
        } catch (std::exception &e) {
          std::cout << "ERROR " << e.what() << std::endl;
          continue;
        }
        
        if (!conn) {
          std::cout << "ERROR (unknown)" << std::endl;
          continue;
        }
        
        std::cout << "Read data from connection: ";
        try {
          if (conn->read(buffer, len, -1)) {
            if (buffer) {
              std::cout << "\"" << buffer << "\"" << std::endl;
              free(buffer);
            } else {
              std::cout << "<null>" << std::endl;
            }
          }
          
          std::cout << "Close connection" << std::endl;
          socket.close(conn);
          
        } catch (std::exception &e) {
          std::cout << "ERROR " << e.what() << std::endl;
        }
      }
      
    } catch (gnet::Exception &e) {
      std::cout << e.what() << std::endl;
    }
  }
  
  gnet::Uninitialize();
  
  return 0;
}
