#ifndef telnetserver_h_
#define telnetserver_h_

#include <Ethernet.h>

namespace telnet {

typedef void (*TelnetCmdFn)(class TelnetServer& server, char* args[],
                            int num_args);

class TelnetServer {

public:

  TelnetServer(int port);
  ~TelnetServer();

  void init();

  void onCommand(const char* cmd, TelnetCmdFn cmd_fn);

  void respond(const char* s);

  void update();

private:
  typedef struct commandT {
    const char* str;
    TelnetCmdFn fn;
    struct commandT* next;
  } commandT;

  EthernetClient m_client;
  EthernetServer m_server;

  int m_i;
  char m_command[64];

  commandT* m_commands;

  int parseArgs(char* str, char* args[], int num_max_args);
};

}

#endif // telnetserver_h_

