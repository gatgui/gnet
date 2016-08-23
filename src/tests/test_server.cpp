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
  
  gnet::GlobalInit gni;
  
  if (gni)
  {
    gnet::Status stat;
    gnet::TCPSocket socket(4001, &stat);
    
    if (!stat) {
      std::cerr << stat << std::endl;
      return 1;
    }
    
    stat = socket.bindAndListen(5);
    if (!stat) {
      std::cerr << stat << std::endl;
      return 1;
    }
    
    ReadStdin reader(&socket);
    gcore::Thread thr(&reader, METHOD(ReadStdin, run), true);
    
    std::cout << "Type 'QUIT' to exit." << std::endl;
    
    while (thr.running()) {
      char *buffer = 0;
      size_t len = 0;
      
      gnet::TCPConnection *conn = socket.accept(&stat);
      if (!conn) {
        std::cerr << "Server error: " << stat << std::endl;
        continue;
      }
      
      if (conn->read(buffer, len, -1, &stat)) {
        if (buffer) {
          std::cout << "\"" << buffer << "\"" << std::endl;
          free(buffer);
        } else {
          std::cout << "<null>" << std::endl;
        }
      } else if (!stat) {
        std::cout << "Client error: " << stat << std::endl;
      }
      
      socket.close(conn);
    }
    
    thr.join();
  }
  
  return 0;
}
