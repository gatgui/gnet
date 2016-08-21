#include <gcore/all.h>
#include <gnet/all.h>


int main(int, char**) {
  
  gnet::Initialize();

  gnet::TCPSocket socket(8080);
  
  socket.bindAndListen(5);
  
  bool end = false;
  
  try {
    bool waiting = false;
    while (!end) {
      // non-blocking select
      if (socket.selectReadable(0)) {
        gnet::TCPConnection *conn;
        std::string data;
        
        while ((conn = socket.nextReadable()) != NULL) {
          std::cout << "Read data from connection (blocking)...";
          
          try {
            if (conn->read(data)) {
              std::cout << "DONE: \"" << data << "\"" << std::endl;
              if (data.find("QUIT") != std::string::npos) {
                end = true;
                break;
              }
            }
          } catch (gnet::Exception &e) {
            std::cerr << "Connection error: " << e.what() << std::endl;
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
  
  gnet::Uninitialize();
  
  return 0;
}
