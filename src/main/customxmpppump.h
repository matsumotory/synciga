#include <iomanip>
#include <iostream>
#include <string>

using namespace std;

//#include "talk/base/basicdefs.h"
//#include "talk/base/common.h"
//#include "talk/base/helpers.h"
//#include "talk/base/logging.h"
//#include "talk/base/ssladapter.h"
//#include "talk/base/stringutils.h"
//#include "talk/base/thread.h"
//#include "talk/p2p/base/sessionmanager.h"
//#include "talk/p2p/client/autoportallocator.h"
//#include "talk/p2p/client/sessionmanagertask.h"
//#include "talk/xmpp/xmppengine.h"
//#include "talk/session/tunnel/tunnelsessionclient.h"
//#include "talk/xmpp/xmppclient.h"
//#include "talk/xmpp/xmppclientsettings.h"
//#include "talk/xmpp/xmpppump.h"
//#include "talk/xmpp/xmppsocket.h"

class CustomXmppPump : public buzz::XmppPumpNotify, public buzz::XmppPump {
public:
  CustomXmppPump();

  void Serve(cricket::TunnelSessionClient* client);
  void OnStateChange(buzz::XmppEngine::State state);
  void OnIncomingTunnel(cricket::TunnelSessionClient* client, buzz::Jid jid, string description, cricket::Session* session);
  bool ProcessStream(talk_base::StreamInterface* stream, const string& filename, bool send);
  void OnStreamEvent(talk_base::StreamInterface* stream, int events, int error);
  void Cleanup(talk_base::StreamInterface* stream, bool delay = false);

private:
  bool server_, sending_;
  talk_base::scoped_ptr<talk_base::FileStream> file_;
  char buffer_[1024 * 64];
  size_t buffer_len_;
};
