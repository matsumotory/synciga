#include <iostream>

#include "talk/base/stringutils.h"
#include "talk/base/thread.h"

using namespace std;

// Runs the current thread until a message with the given ID is seen.
uint32 Loop(const vector<uint32>& ids) {
  talk_base::Message msg;
  while (talk_base::Thread::Current()->Get(&msg)) {
    if (msg.phandler == NULL) {
      if (find(ids.begin(), ids.end(), msg.message_id) != ids.end())
        return msg.message_id;
      cout << "orphaned message: " << msg.message_id;
      continue;
    }
    talk_base::Thread::Current()->Dispatch(&msg);
  }
  return 0;
}
