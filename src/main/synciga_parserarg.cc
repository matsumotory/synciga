#include <iostream>
#include <string>

#include "talk/base/stringutils.h"
#include "talk/xmpp/xmppengine.h"

#include "error.h"

using namespace std;
// Determines whether the given string is an option.  If so, the name and
// value are appended to the given strings.
bool ParseArg(const char* arg, string* name, string* value) {
  if (strncmp(arg, "--", 2) != 0)
    return false;

  const char* eq = strchr(arg + 2, '=');
  if (eq) {
    if (name)
      name->append(arg + 2, eq);
    if (value)
      value->append(eq + 1, arg + strlen(arg));
  } else {
    if (name)
      name->append(arg + 2, arg + strlen(arg));
    if (value)
      value->clear();
  }

  return true;
}

int ParseIntArg(const string& name, const string& value) {
  char* end;
  long val = strtol(value.c_str(), &end, 10);
  if (*end != '\0')
    Error(string("value of option ") + name + " must be an integer");
  return static_cast<int>(val);
}

#ifdef WIN32
#pragma warning(push)
// disable "unreachable code" warning b/c it varies between dbg and opt
#pragma warning(disable: 4702)
#endif
bool ParseBoolArg(const string& name, const string& value) {
  if (value == "true")
    return true;
  else if (value == "false")
    return false;
  else {
    Error(string("value of option ") + name + " must be true or false");
    return false;
  }
}
#ifdef WIN32
#pragma warning(pop)
#endif

void ParseFileArg(const char* arg, buzz::Jid* jid, string* file) {
  const char* sep = strchr(arg, ':');
  if (!sep) {
    *file = arg;
  } else {
    buzz::Jid jid_arg(string(arg, sep-arg));
    if (jid_arg.IsBare())
      Error("A full JID is required for the source or destination arguments.");
    *jid = jid_arg;
    *file = string(sep+1);
  }
}

void SetConsoleEcho(bool on) {
#ifdef WIN32
  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
  if ((hIn == INVALID_HANDLE_VALUE) || (hIn == NULL))
    return;

  DWORD mode;
  if (!GetConsoleMode(hIn, &mode))
    return;

  if (on) {
    mode = mode | ENABLE_ECHO_INPUT;
  } else {
    mode = mode & ~ENABLE_ECHO_INPUT;
  }

  SetConsoleMode(hIn, mode);
#else
  int re;
  if (on)
    re = system("stty echo");
  else
    re = system("stty -echo");
  if (-1 == re)
    return;
#endif
}
