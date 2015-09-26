#ifndef memorylog_h_
#define memorylog_h_

namespace logging {

class MemoryLog {

public:

  MemoryLog(int size = 512);
  ~MemoryLog();

  void error(const char* s, ...);
  void info(const char* s, ...);
  void warn(const char* s, ...);

  void printf(const char* s, ...);

  const char* getText();

private:

  int m_size;
  int m_i;
  char* m_log;

  void rotate(int n);

};

}

#endif // memorylog_h_

