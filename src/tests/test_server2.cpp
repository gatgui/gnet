#include <gcore/all.h>
#include <gnet/all.h>

int ReadStdin() {
  bool end = false;
  char rdbuf[512];
  while (!end) {
    if (fgets(rdbuf, 512, stdin)) {
      if (!strncmp(rdbuf, "QUIT", 4)) {
        end = true;
      }
    }
  }
  return 0;
}

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
    
    gcore::Thread thr(&ReadStdin, NULL, true);
    
    try {
      std::cout << "Type 'QUIT' to exit." << std::endl;
      
      while (thr.running()) {
        if (socket.selectReadable(0)) {
          gnet::TCPConnection *conn;
          std::string data;
          
          while ((conn = socket.nextReadable()) != NULL) {
            try {
              if (conn->read(data)) {
                std::cout << "\"" << data << "\"" << std::endl;
              }
            } catch (gnet::Exception &e) {
              std::cerr << e.what() << std::endl;
              // should I wait here?
              socket.close(conn);
            }
          }
        } else {
          // Passively wait 50ms
          // CPU consumption wise, tt is seems better to do select return immediately and follow with a sleep
          // than to do a timed out select
          gcore::Thread::SleepCurrent(50);
        }
      }
      
    } catch (gnet::Exception &e) {
      std::cout << e.what() << std::endl;
    }
  }
  
  gnet::Uninitialize();
  
  return 0;
}
