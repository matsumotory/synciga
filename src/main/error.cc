#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;

// Prints out a usage message then exits.
void Usage() {
  cerr << "Usage:" << endl;
  cerr << "  synciga --sync    <my_jid> <dst_full_jid>              (syncer client mode)" << endl;
  cerr << "  synciga [options] <my_jid>                             (server mode)" << endl;
  cerr << "  synciga [options] <my_jid> <src_file> <dst_full_jid>:<dst_file> (client sending)" << endl;
  cerr << "  synciga [options] <my_jid> <src_full_jid>:<src_file> <dst_file> (client rcv'ing)" << endl;
  cerr << "           --verbose" << endl;
  cerr << "           --sync-dir=<sync-dir>" << endl;
  cerr << "           --remote-dir=<remotec-dir>" << endl;
  cerr << "           --verbose" << endl;
  cerr << "           --xmpp-host=<host>" << endl;
  cerr << "           --xmpp-port=<port>" << endl;
  cerr << "           --xmpp-use-tls=(true|false)" << endl;
  exit(1);
}

// Prints out an error message, a usage message, then exits.
void Error(const string& msg) {
  cerr << "error: " << msg << endl;
  cerr << endl;
  Usage();
}

void FatalError(const string& msg) {
  cerr << "error: " << msg << endl;
  cerr << endl;
  exit(1);
}
