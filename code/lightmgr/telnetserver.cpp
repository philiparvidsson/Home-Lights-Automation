#include "telnetserver.h"

namespace telnet {

TelnetServer::TelnetServer(int port)
    : m_commands(NULL),
      m_server(EthernetServer(23))
{
    m_commands = NULL;
}

TelnetServer::~TelnetServer() {
    while (m_commands) {
        commandT* cmd = m_commands;

        m_commands = m_commands->next;

        delete cmd;
    }
}

void TelnetServer::init() {
    m_server.begin();
}

void TelnetServer::onCommand(const char* cmd, TelnetCmdFn cmd_fn) {
    commandT* c = new commandT;

    c->str  = cmd;
    c->fn   = cmd_fn;
    c->next = m_commands;

    m_commands = c;
}

void TelnetServer::respond(const char* s) {
    if (!m_client || !m_client.connected())
        return;

    m_client.print(s);
    m_client.flush();
}

void TelnetServer::update() {
    if (!m_client) {
        m_client = m_server.available();

        for (int i = 0; i < sizeof(m_command); i++)
            m_command[i] = '\0';

        m_i = 0;
    }

    if (!m_client)
        return;

    if (!m_client.connected())
        m_client.stop();

    while (m_client.available()) {
        char c = (char)m_client.read();

        if (c == '\r')
            continue;

        m_command[m_i++] = c;

        if (m_command[m_i-1] == '\n') {
            m_command[m_i-1] = '\0';

            char* args[10] = { 0 };
            int num_args = parseArgs(m_command, args, 10);

            commandT* cmd = m_commands;
            while (cmd) {
                if (strcmp(m_command, cmd->str) == 0) {
                    cmd->fn(*this, args, num_args);
                    break;
                }

                cmd = cmd->next;
            }

            if (!cmd)
                respond("what?\r\n");

            for (int i = 0; i < sizeof(m_command); i++)
                m_command[i] = '\0';

            m_i = 0;
        }
        else if (m_i == sizeof(m_command)) {
            // Client sent a command that is too long.
            m_i = 0;
            m_client.stop();
            break;
        }
    }
}

int TelnetServer::parseArgs(char* str, char* args[], int num_max_args) {
    int n = strlen(str);

    int j = 0;
    for (int i = 0; i < n; i++) {
        if (str[i] != ' ')
            continue;

        str[i] = '\0';
        args[j++] = &str[i+1];
        if (j >= num_max_args)
            break;
    }

    return (j);
}

}

