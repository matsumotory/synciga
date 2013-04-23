#include <iomanip>
#include <iostream>
#include <string>

#include "talk/base/logging.h"
#include "talk/base/thread.h"

using namespace std;

class DebugLog : public sigslot::has_slots<> {
public:
  char * debug_input_buf_;
  int debug_input_len_;
  int debug_input_alloc_;
  char * debug_output_buf_;
  int debug_output_len_;
  int debug_output_alloc_;
  bool censor_password_;

  DebugLog();

  void Input(const char * data, int len);
  void Output(const char * data, int len);
  bool IsAuthTag(const char * str, size_t len);
  void DebugPrint(char * buf, int * plen, bool output);
};
