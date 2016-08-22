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
      bool waiting = false;
      while (thr.running()) {
        // non-blocking select
        if (socket.selectReadable(0)) {
          gnet::TCPConnection *conn;
          std::string data;
          
          while ((conn = socket.nextReadable()) != NULL) {
            try {
              if (conn->read(data)) {
                std::cout << "\"" << data << "\"" << std::endl;
              }
            } catch (gnet::Exception &e) {
              std::cerr << "ERROR " << e.what() << std::endl;
              // should I wait here?
              socket.close(conn);
            }
          }
          
          waiting = false;
        
        } else {
          if (!waiting) {
            std::cout << "Nothing to read yet" << std::endl;
          }
          
          gcore::Thread::SleepCurrent(1000); // 1s
          
          waiting = true;
        }
      }
      
    } catch (gnet::Exception &e) {
      
      std::cout << e.what() << std::endl;
    }
  }
  
  gnet::Uninitialize();
  
  return 0;
}
