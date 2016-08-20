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

  try {
    /*
    std::cout << "Create socket" << std::endl;
    Nett::Socket socket(server, 8080);
    
    std::cout << "Connect to socket" << std::endl;
    gnet::Connection conn = socket.connect();
    
    std::cout << "Send data to socket" << std::endl;
    conn.write(content.c_str(), content.length());
    */
    bool end = false;
    
    std::cout << "Get Host...";
    gnet::Host host(server, port);
    std::cout << "DONE: " << host.address() << ":" << host.port() << std::endl;
    
    gnet::TCPSocket socket(host);
    
    std::cout << "Connect to server...";
    gnet::TCPConnection *conn = socket.connect();
    std::cout << "DONE" << std::endl;
    
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
        continue;
      }
      
      std::cout << "Send data: \"" << buffer << "\"...";
      conn->write(buffer, len-1);
      std::cout << "DONE" << std::endl;
      
      // to check if connection is alive
      // do a non-blocking read, and check for 0 result
      // read(fd, buffer, len, MSG_NONBLOCK) == 0
  
    }
      
    std::cout << "Close connection...";
    socket.close(conn);
    std::cout << "DONE" << std::endl;
    
  } catch (std::exception &e) {
    
    std::cout << e.what() << std::endl;
  }
  
  gnet::Uninitialize();
  
  return 0;
}
