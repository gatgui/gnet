#include <gcore/all.h>
#include <gnet/all.h>

class ReadStdin {
public:
  ReadStdin(gnet::TCPSocket *socket)
    : mSocket(socket) {
  }
  ~ReadStdin() {
  }
  int run() {
    bool end = false;
    char rdbuf[512];
    while (!end) {
      if (fgets(rdbuf, 512, stdin)) {
        if (!strncmp(rdbuf, "QUIT", 4)) {
          end = true;
        }
      }
    }
    if (mSocket) {
      mSocket->disconnect();
    }
    return 0;
  }
private:
  gnet::TCPSocket *mSocket;
};

int main(int, char**) {
  
  if (!gnet::Initialize()) {
    return 1;
  }
  
  // Create a new scope so that socket destructor is called before gnet::Uninitialize
  {
    gnet::TCPSocket socket(4001);
    
    try {
      socket.bindAndListen(5);
    } catch (std::exception &e) {
      std::cout << e.what() << std::endl;
      gnet::Uninitialize();
      return 1;
    }
    
    ReadStdin reader(&socket);
    gcore::Thread thr(&reader, METHOD(ReadStdin, run), true);
    
    std::cout << "Type 'QUIT' to exit." << std::endl;
    
    try {
      while (thr.running()) {
        char *buffer = 0;
        size_t len = 0;
        
        gnet::TCPConnection *conn = socket.accept();
        if (!conn) {
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
        } catch (std::exception &e) {
          std::cout << "Client error: " << e.what() << std::endl;
        }
        
        socket.close(conn);
      }
      
    } catch (gnet::Exception &e) {
      std::cout << "Server error: " << e.what() << std::endl;
    }
    
    thr.join();
  }
  
  gnet::Uninitialize();
  
  return 0;
}
