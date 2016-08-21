#include <gcore/all.h>
#include <gnet/all.h>
#include <exception>

int main(int argc, char **argv) {

  if (argc > 3) {
    std::cout << "test_client [server [port]]" << std::endl;
    return -1;
  }
  
  std::string server = "localhost";
  unsigned short port = 8080;

  if (argc >= 2) {
    server = argv[1];
  }

  if (argc >= 3) {
    sscanf(argv[2], "%hu", &port);
  }

  gnet::Initialize();

  std::cout << "Get Host...";
  gnet::Host host(server, port);
  std::cout << "DONE: " << host.address() << ":" << host.port() << std::endl;
  
  gnet::TCPSocket socket(host);
  
  std::cout << "Connect to server...";
  gnet::TCPConnection *conn = socket.connect();
  std::cout << "DONE" << std::endl;

  try {
    bool end = false;
    
    while (!end) {
      
      char buffer[512];
      
      if (!fgets(buffer, 512, stdin)) {
        end = true;
        continue;
      }
      
      size_t len = strlen(buffer);
      if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
      }
      
      if (!strcmp(buffer, "QUIT")) {
        end = true;
      }
      
      std::cout << "Send data: \"" << buffer << "\"...";
      bool rv = conn->write(buffer, len-1);
      std::cout << "DONE (" << rv << ")" << std::endl;
      
      // to check if connection is alive
      // do a non-blocking read, and check for 0 result
      // read(fd, buffer, len, MSG_NONBLOCK) == 0
    }
    
  } catch (std::exception &e) {
    
    std::cout << e.what() << std::endl;
  }
  
  char buffer[8];
  std::cout << "Press any Key to Terminate" << std::endl;
  fgets(buffer, 8, stdin);
  
  std::cout << "Close connection...";
  socket.close(conn);
  std::cout << "DONE" << std::endl;
  
  gnet::Uninitialize();
  
  return 0;
}
