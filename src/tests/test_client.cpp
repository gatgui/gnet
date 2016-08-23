#include <gcore/all.h>
#include <gnet/all.h>
#include <exception>

int main(int argc, char **argv) {
  
  gnet::GlobalInit gni;
  
  if (gni) {
    
    if (argc > 3) {
      std::cout << "test_client [server [port]]" << std::endl;
      return 1;
    }
  
    std::string server = "localhost";
    unsigned short port = 4001;

    if (argc >= 2) {
      server = argv[1];
    }

    if (argc >= 3) {
      sscanf(argv[2], "%hu", &port);
    }
    
    gnet::Status stat;

    std::cout << "Get Host...";
    gnet::Host host(server, port);
    std::cout << "DONE: " << host.address() << ":" << host.port() << std::endl;
    
    gnet::TCPSocket socket(host, &stat);
    if (!stat) {
      std::cerr << stat << std::endl;
      return 1;
    }
    
    std::cout << "Connect to server..." << std::endl;
    gnet::TCPConnection *conn = socket.connect(&stat);
    if (!conn) {
      std::cerr << stat << std::endl;
      return 1;
    }

    std::cout << "Type 'QUIT' to exit." << std::endl;
      
    bool end = false;
    
    while (!end) {
      
      char buffer[512];
      
      std::cout << "Input text: ";
      if (!fgets(buffer, 512, stdin)) {
        end = true;
        continue;
      }
      
      size_t len = strlen(buffer);
      if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
      }
      
      if (!strcmp(buffer, "QUIT")) {
        conn->shutdown();
        end = true;
      
      } else {
        std::cout << "Send data: \"" << buffer << "\"..." << std::endl;
        if (conn->write(buffer, len-1, -1, &stat) == 0 && !stat) {
          // nothing written and stat set to failed
          std::cerr << stat << std::endl;
          end = true;
        }
      }
    }
    
    char buffer[8];
    std::cout << "Press any Key to Terminate" << std::endl;
    fgets(buffer, 8, stdin);
  }
  
  return 0;
}
