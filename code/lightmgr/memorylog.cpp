#include "memorylog.h"

#include <stdio.h>
#include <string.h>

namespace logging {

MemoryLog::MemoryLog(int size) {
  m_log = new char[size];
  m_size = size;
  m_i = 0;

  m_log[0] = '\0';
}

MemoryLog::~MemoryLog() {
  delete[] m_log;
}

void MemoryLog::error(const char* s, ...) {
  this->printf("error: ");
  this->printf(s);
  this->printf("\r\n");
}

void MemoryLog::info(const char* s, ...) {
  this->printf(s);
  this->printf("\r\n");
}

void MemoryLog::warn(const char* s, ...) {
  this->printf("warning: ");
  this->printf(s);
  this->printf("\r\n");
}

void MemoryLog::printf(const char* s, ...) {
  char buf[128];

  sprintf(buf, s);

  int n = strlen(buf)+1;

  if (n > m_size-m_i)
    rotate(n-m_size+m_i);

  for (int i = 0; i < n; i++)
    m_log[m_i++] = buf[i];

  m_i--;
}

const char* MemoryLog::getText() {
  return (m_log);
}

void MemoryLog::rotate(int n) {
  if (n > m_size)
    n = m_size;

  for (int i = 0; i < m_size-n; i++)
    m_log[i] = m_log[i+n];

  m_i -= n;
  if (m_i < 0)
    m_i = 0;
}

}

