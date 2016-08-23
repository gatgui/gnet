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
    
    gcore::Thread thr(&ReadStdin, NULL, true);
    
    std::cout << "Type 'QUIT' to exit." << std::endl;
      
    while (thr.running()) {
      if (socket.selectReadable(0, &stat)) {
        gnet::TCPConnection *conn;
        std::string data;
        
        while ((conn = socket.nextReadable()) != NULL) {
          if (conn->read(data, -1, &stat)) {
            std::cout << "\"" << data << "\"" << std::endl;
          
          } else if (!stat) {
            std::cerr << stat << std::endl;
            // should I wait here?
            socket.close(conn);
          }
        }
      
      } else {
        if (!stat) {
          std::cerr << stat << std::endl;
          break;
          
        } else {
          // Passively wait 50ms
          // CPU consumption wise, tt is seems better to do select return immediately and follow with a sleep
          // than to do a timed out select
          gcore::Thread::SleepCurrent(50);
        }
      }
    }
    
    thr.join();
  }
  
  return 0;
}
