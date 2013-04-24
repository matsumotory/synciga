#include <iomanip>
#include <iostream>
#include <string>

//#include "talk/base/basicdefs.h"
//#include "talk/base/common.h"
//#include "talk/base/helpers.h"
//#include "talk/base/logging.h"
//#include "talk/base/ssladapter.h"
//#include "talk/base/stringutils.h"
//#include "talk/base/thread.h"
//#include "talk/p2p/base/sessionmanager.h"
//#include "talk/p2p/client/autoportallocator.h"
#include "talk/p2p/client/sessionmanagertask.h"
#include "talk/xmpp/xmppengine.h"
#include "talk/session/tunnel/tunnelsessionclient.h"
#include "talk/xmpp/xmppclient.h"
#include "talk/xmpp/xmppclientsettings.h"
#include "talk/xmpp/xmpppump.h"
//#include "talk/xmpp/xmppsocket.h"

#include "customxmpppump.h"

using namespace std;

enum {
    MSG_LOGIN_COMPLETE = 1,
    MSG_LOGIN_FAILED,
    MSG_DONE,
};

CustomXmppPump::CustomXmppPump() : XmppPump(this), server_(false) { }

void CustomXmppPump::Serve(cricket::TunnelSessionClient* client) {
  client->SignalIncomingTunnel.connect(this,
    &CustomXmppPump::OnIncomingTunnel);
  server_ = true;
}

void CustomXmppPump::OnStateChange(buzz::XmppEngine::State state) {
  switch (state) {
  case buzz::XmppEngine::STATE_START:
    cout << "Connecting...";
    break;
  case buzz::XmppEngine::STATE_OPENING:
    cout << " OK" << endl;
    cout << "Logging in...";
    break;
  case buzz::XmppEngine::STATE_OPEN:
    cout << " OK" << endl;
    cout << "Logged in...";
    talk_base::Thread::Current()->Post(NULL, MSG_LOGIN_COMPLETE);
    break;
  case buzz::XmppEngine::STATE_CLOSED:
    cout << "Logged out..." << endl;
    talk_base::Thread::Current()->Post(NULL, MSG_LOGIN_FAILED);
    break;
  }
}

void CustomXmppPump::OnIncomingTunnel(cricket::TunnelSessionClient* client, buzz::Jid jid,
  string description, cricket::Session* session) {
  cout << "IncomingTunnel from " << jid.Str()
    << ": " << description << endl;
  //if (!server_ || file_) {
  if (!server_ || (file_.get() != NULL)) {
    client->DeclineTunnel(session);
    return;
  }
  string filename;
  bool send;
  if (strncmp(description.c_str(), "send:", 5) == 0) {
    send = true;
  } else if (strncmp(description.c_str(), "recv:", 5) == 0) {
    send = false;
  } else {
    client->DeclineTunnel(session);
    return;
  }
  filename = description.substr(5);
  talk_base::StreamInterface* stream = client->AcceptTunnel(session);
  if (!ProcessStream(stream, filename, send))
    talk_base::Thread::Current()->Post(NULL, MSG_DONE);

  // TODO: There is a potential memory leak, however, since the synciga
  // app doesn't work right now, I can't verify the fix actually works, so
  // comment out the following line until we fix the synciga app.

  // delete stream;
}

bool CustomXmppPump::ProcessStream(talk_base::StreamInterface* stream,
                   const string& filename, bool send) {
  //ASSERT(file_);
  ASSERT(file_.get() == NULL);
  sending_ = send;
  file_.reset(new talk_base::FileStream);
  buffer_len_ = 0;
  int err;
  if (!file_->Open(filename.c_str(), sending_ ? "rb" : "wb", &err)) {
    cerr << "Error opening <" << filename << ">: "
              << strerror(err) << endl;
    return false;
  }


  stream->SignalEvent.connect(this, &CustomXmppPump::OnStreamEvent);
  if (stream->GetState() == talk_base::SS_CLOSED) {
    cerr << "Failed to establish P2P tunnel" << endl;
    return false;
  }
  if (stream->GetState() == talk_base::SS_OPEN) {
    OnStreamEvent(stream,
      talk_base::SE_OPEN | talk_base::SE_READ | talk_base::SE_WRITE, 0);
  }
  return true;
}

void CustomXmppPump::OnStreamEvent(talk_base::StreamInterface* stream, int events,
                   int error) {
  if (events & talk_base::SE_CLOSE) {
    if (error == 0) {
      cout << "Tunnel closed normally" << endl;
    } else {
      cout << "Tunnel closed with error: " << error << endl;
    }
    CustomXmppPump::Cleanup(stream);
    return;
  }
  if (events & talk_base::SE_OPEN) {
    cout << "Tunnel connected" << endl;
  }
  talk_base::StreamResult result;
  size_t count;
  if (sending_ && (events & talk_base::SE_WRITE)) {
    LOG(LS_VERBOSE) << "Tunnel SE_WRITE";
    while (true) {
      size_t write_pos = 0;
      while (write_pos < buffer_len_) {
        result = stream->Write(buffer_ + write_pos, buffer_len_ - write_pos,
                              &count, &error);
        if (result == talk_base::SR_SUCCESS) {
          write_pos += count;
          continue;
        }
        if (result == talk_base::SR_BLOCK) {
          buffer_len_ -= write_pos;
          memmove(buffer_, buffer_ + write_pos, buffer_len_);
          LOG(LS_VERBOSE) << "Tunnel write block";
          return;
        }
        if (result == talk_base::SR_EOS) {
          cout << "Tunnel closed unexpectedly on write" << endl;
        } else {
          cout << "Tunnel write error: " << error << endl;
        }
        CustomXmppPump::Cleanup(stream);
        return;
      }
      buffer_len_ = 0;
      while (buffer_len_ < sizeof(buffer_)) {
        result = file_->Read(buffer_ + buffer_len_,
                            sizeof(buffer_) - buffer_len_,
                            &count, &error);
        if (result == talk_base::SR_SUCCESS) {
          buffer_len_ += count;
          continue;
        }
        if (result == talk_base::SR_EOS) {
          if (buffer_len_ > 0)
            break;
          cout << "End of file" << endl;
          // A hack until we have friendly shutdown
          CustomXmppPump::Cleanup(stream, true);
          return;
        } else if (result == talk_base::SR_BLOCK) {
          cout << "File blocked unexpectedly on read" << endl;
        } else {
          cout << "File read error: " << error << endl;
        }
        CustomXmppPump::Cleanup(stream);
        return;
      }
    }
  }
  if (!sending_ && (events & talk_base::SE_READ)) {
    LOG(LS_VERBOSE) << "Tunnel SE_READ";
    while (true) {
      buffer_len_ = 0;
      while (buffer_len_ < sizeof(buffer_)) {
        result = stream->Read(buffer_ + buffer_len_,
                              sizeof(buffer_) - buffer_len_,
                              &count, &error);
        if (result == talk_base::SR_SUCCESS) {
          buffer_len_ += count;
          continue;
        }
        if (result == talk_base::SR_BLOCK) {
          if (buffer_len_ > 0)
            break;
          LOG(LS_VERBOSE) << "Tunnel read block";
          return;
        }
        if (result == talk_base::SR_EOS) {
          cout << "Tunnel closed unexpectedly on read" << endl;
        } else {
          cout << "Tunnel read error: " << error << endl;
        }
        CustomXmppPump::Cleanup(stream);
        return;
      }
      size_t write_pos = 0;
      while (write_pos < buffer_len_) {
        result = file_->Write(buffer_ + write_pos, buffer_len_ - write_pos,
                              &count, &error);
        if (result == talk_base::SR_SUCCESS) {
          write_pos += count;
          continue;
        }
        if (result == talk_base::SR_EOS) {
          cout << "File closed unexpectedly on write" << endl;
        } else if (result == talk_base::SR_BLOCK) {
          cout << "File blocked unexpectedly on write" << endl;
        } else {
          cout << "File write error: " << error << endl;
        }
        CustomXmppPump::Cleanup(stream);
        return;
      }
    }
  }
}

void CustomXmppPump::Cleanup(talk_base::StreamInterface* stream, bool delay) {
  LOG(LS_VERBOSE) << "Closing";
  stream->Close();
  file_.reset();
  if (!server_) {
    if (delay)
      talk_base::Thread::Current()->PostDelayed(2000, NULL, MSG_DONE);
    else
      talk_base::Thread::Current()->Post(NULL, MSG_DONE);
  }
}

