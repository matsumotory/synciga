/*
** synciga - File Syncer Tool for NAT using P2P
**
** See Copyright Notice in LICENSE and LICENSE_THIRD_PARTY
**
*/
#define _CRT_SECURE_NO_DEPRECATE 1

#include <time.h>

#if defined(POSIX)
#include <unistd.h>
#endif

#include <iomanip>
#include <iostream>
#include <string>
#include <exception>

#if HAVE_CONFIG_H
#include "config.h"
#endif  // HAVE_CONFIG_H

#include "talk/base/sslconfig.h"

//#include "talk/base/basicdefs.h"
//#include "talk/base/common.h"
//#include "talk/base/helpers.h"
//#include "talk/base/logging.h"
#include "talk/base/ssladapter.h"
//#include "talk/base/stringutils.h"
//#include "talk/base/thread.h"
//#include "talk/p2p/base/sessionmanager.h"
#include "talk/p2p/client/autoportallocator.h"
#include "talk/p2p/client/sessionmanagertask.h"
#include "talk/xmpp/xmppengine.h"
#include "talk/session/tunnel/tunnelsessionclient.h"
//#include "talk/xmpp/xmppclient.h"
//#include "talk/xmpp/xmppclientsettings.h"
#include "talk/xmpp/xmpppump.h"
#include "talk/xmpp/xmppsocket.h"

#include "inotify-cxx.h"
#include "logger.h"
#include "customxmpppump.h"
#include "error.h"

#include <mruby.h>
#include <mruby/compile.h>

using namespace std;

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1400)
// The following are necessary to properly link when compiling STL without
// /EHsc, otherwise known as C++ exceptions.
void __cdecl _Throw(const exception &) {}
_Prhand _Raise_handler = 0;
#endif

enum {
  MSG_LOGIN_COMPLETE = 1,
  MSG_LOGIN_FAILED,
  MSG_DONE,
};

buzz::Jid gUserJid;
talk_base::InsecureCryptStringImpl gUserPass;
string gXmppHost = "talk.google.com";
int gXmppPort = 5222;
bool syncer_enable = false;
buzz::TlsOptions gXmppUseTls = buzz::TLS_REQUIRED;
static DebugLog debug_log_;

// synciga_parserarg.cc
bool ParseArg(const char* arg, string* name, string* value);
int ParseIntArg(const string& name, const string& value);
bool ParseBoolArg(const string& name, const string& value);
void ParseFileArg(const char* arg, buzz::Jid* jid, string* file);
void SetConsoleEcho(bool on);

// synciga_thread.cc
uint32 Loop(const vector<uint32>& ids);

buzz::XmppClientSettings LoginSettings() {
  buzz::XmppClientSettings xcs;
  xcs.set_user(gUserJid.node());
  xcs.set_host(gUserJid.domain());
  xcs.set_resource("synciga");
  xcs.set_pass(talk_base::CryptString(gUserPass));
  talk_base::SocketAddress server(gXmppHost, gXmppPort);
  xcs.set_server(server);
  xcs.set_use_tls(gXmppUseTls);
  return xcs;
}

#ifdef WIN32
#pragma warning(disable:4355)
#endif

int main(int argc, char **argv) {
  talk_base::LogMessage::LogThreads();
  talk_base::LogMessage::LogTimestamps();

  // TODO: Default the username to the current users's name.


  char path[MAX_PATH];
  string sync_dir;
  string remote_dir;
#if WIN32
  GetCurrentDirectoryA(MAX_PATH, path);
#else
  if (NULL == getcwd(path, MAX_PATH))
    Error("Unable to get current path");
#endif

  sync_dir = string(path);

  // Parse the arguments.
  int index = 1;
  while (index < argc) {
    string name, value;
    if (!ParseArg(argv[index], &name, &value))
      break;

    if (name == "help") {
      Usage();
    } else if (name == "sync") {
      syncer_enable = true;
    } else if (name == "verbose") {
      talk_base::LogMessage::LogToDebug(talk_base::LS_VERBOSE);
    } else if (name == "xmpp-host") {
      gXmppHost = value;
    } else if (name == "xmpp-port") {
      gXmppPort = ParseIntArg(name, value);
    } else if (name == "sync-dir") {
      sync_dir = value;
    } else if (name == "remote-dir") {
      remote_dir = value;
    } else if (name == "xmpp-use-tls") {
      gXmppUseTls = ParseBoolArg(name, value)?
          buzz::TLS_REQUIRED : buzz::TLS_DISABLED;
    } else {
      Error(string("unknown option: ") + name);
    }

    index += 1;
  }

  if (index >= argc)
    Error("bad arguments");
  gUserJid = buzz::Jid(argv[index++]);
  if (!gUserJid.IsValid())
    Error("bad arguments");

  cout << "Directory: " << sync_dir << endl;

  buzz::Jid gSrcJid;
  buzz::Jid gDstJid;
  string gSrcFile;
  string gDstFile;

  bool as_server = true;
  if (syncer_enable && index + 1 == argc) {
    //ParseFileArg(argv[index], &gDstJid, &gDstFile);
    buzz::Jid jid_arg(string(argv[index]));
    gDstJid = jid_arg;
    if (gDstJid.IsBare())
      Error("A full JID is required for the source or destination arguments.");
    if(gSrcJid.Str().empty() == gDstJid.Str().empty())
      Error("Exactly one of source JID or destination JID must be empty.");
    as_server = false;
  } else if (index + 2 == argc) {
    ParseFileArg(argv[index], &gSrcJid, &gSrcFile);
    ParseFileArg(argv[index+1], &gDstJid, &gDstFile);
    if(gSrcJid.Str().empty() == gDstJid.Str().empty())
      Error("Exactly one of source JID or destination JID must be empty.");
    as_server = false;
  } else if (index != argc) {
    Error("bad arguments");
  }

  cout << "Password: ";
  SetConsoleEcho(false);
  cin >> gUserPass.password();
  SetConsoleEcho(true);
  cout << endl;

  talk_base::InitializeSSL();
  // Log in.
  CustomXmppPump pump;
  pump.client()->SignalLogInput.connect(&debug_log_, &DebugLog::Input);
  pump.client()->SignalLogOutput.connect(&debug_log_, &DebugLog::Output);
  pump.DoLogin(LoginSettings(), new buzz::XmppSocket(gXmppUseTls), 0);
    //new XmppAuth());

  // Wait until login succeeds.
  vector<uint32> ids;
  ids.push_back(MSG_LOGIN_COMPLETE);
  ids.push_back(MSG_LOGIN_FAILED);
  if (MSG_LOGIN_FAILED == Loop(ids))
    FatalError("Failed to connect");

  {
    talk_base::scoped_ptr<buzz::XmlElement> presence(
      new buzz::XmlElement(buzz::QN_PRESENCE));
    presence->AddElement(new buzz::XmlElement(buzz::QN_PRIORITY));
    presence->AddText("-1", 1);
    pump.SendStanza(presence.get());
  }

  string user_jid_str = pump.client()->jid().Str();
  cout << " OK" << endl;
  cout << "Assigned FullJID " << user_jid_str << endl;
  if (as_server) {
    cout << "Input below command on client synciga" << endl << endl;
    cout << "./synciga --sync --remote-dir=" << sync_dir << " " << gUserJid.Str() << " " << user_jid_str << endl;
  }

  // Prepare the random number generator.
  talk_base::InitRandom(user_jid_str.c_str(), user_jid_str.size());

  // Create the P2P session manager.
  talk_base::BasicNetworkManager network_manager;
  AutoPortAllocator allocator(&network_manager, "synciga_agent");
  allocator.SetXmppClient(pump.client());
  cricket::SessionManager session_manager(&allocator);
  cricket::TunnelSessionClient session_client(pump.client()->jid(),
                                              &session_manager);
  cricket::SessionManagerTask *receiver =
      new cricket::SessionManagerTask(pump.client(), &session_manager);
  receiver->EnableOutgoingMessages();
  receiver->Start();

  bool success = true;
  string filename;
  bool sending;
  talk_base::StreamInterface* stream = NULL;

  // mruby test
  mrb_state* mrb = mrb_open();
  mrb_load_string(mrb, "puts 'mruby added test'");
  mrb_close(mrb);

  // Establish the appropriate connection.
  if (as_server) {
    pump.Serve(&session_client);
  } else if (syncer_enable) {
    string watch_dir = sync_dir;
    try {
      Inotify notify;
      InotifyWatch watch(watch_dir, IN_ALL_EVENTS);
      notify.Add(watch);
      cout << "Syncing directory " << watch_dir << endl;
      LOG(INFO) << "from " << user_jid_str << " to " << gDstJid.Str();
      for (;;) {
        notify.WaitForEvents();
        size_t count = notify.GetEventCount();
        string prev_mask_str;
        while (count > 0) {
          InotifyEvent event;
          bool got_event = notify.GetEvent(&event);
          if (got_event) {
            string mask_str;
            event.DumpTypes(mask_str);
            filename = event.GetName();
            LOG(INFO) << "[watch " << watch_dir << "] " << "event mask: \"" << mask_str << "\", " << "filename: \"" << filename << "\"";
            if (mask_str == "IN_CLOSE_WRITE" && prev_mask_str !=  "IN_CLOSE_NOWRITE") {
              cout << "Syncing file start: " << filename << endl;
              string message("recv:");
              if (remote_dir.empty())
                remote_dir = sync_dir;
              message.append(remote_dir + string("/") + filename);
              stream = session_client.CreateTunnel(gDstJid, message);
              sending = true;
              success = pump.ProcessStream(stream, sync_dir + string("/") + filename, sending);
              if (success) {
                ids.clear();
                ids.push_back(MSG_DONE);
                ids.push_back(MSG_LOGIN_FAILED);
                Loop(ids);
                cout << "Syncing file end: " << filename << endl;
              }
            }
            prev_mask_str = mask_str;
          }
          count--;
        }
      }
    } catch (InotifyException &e) {
      cerr << "Inotify exception occured: " << e.GetMessage() << endl;
    } catch (exception &e) {
      cerr << "STL exception occured: " << e.what() << endl;
    } catch (...) {
      cerr << "unknown exception occured" << endl;
    }
  } else {
    if (gSrcJid.Str().empty()) {
      string message("recv:");
      message.append(gDstFile);
      stream = session_client.CreateTunnel(gDstJid, message);
      filename = gSrcFile;
      sending = true;
    } else {
      string message("send:");
      message.append(gSrcFile);
      stream = session_client.CreateTunnel(gSrcJid, message);
      filename = gDstFile;
      sending = false;
    }
    success = pump.ProcessStream(stream, filename, sending);
  }

  if (success) {
    // Wait until the copy is done.
    ids.clear();
    ids.push_back(MSG_DONE);
    ids.push_back(MSG_LOGIN_FAILED);
    Loop(ids);
  }

  // Log out.
  pump.DoDisconnect();

  return 0;
}
