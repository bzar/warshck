#include "libwebsockets.h"
//#include "libsocketio.h"
#include "gamenode.h"
#include "jsonpp.h"
#include <iostream>
/*
const char* LWS_EXT_CALLBACK_STR[] = {
  "LWS_CALLBACK_ESTABLISHED",
  "LWS_CALLBACK_CLIENT_CONNECTION_ERROR",
  "LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH",
  "LWS_CALLBACK_CLIENT_ESTABLISHED",
  "LWS_CALLBACK_CLOSED",
  "LWS_CALLBACK_CLOSED_HTTP",
  "LWS_CALLBACK_RECEIVE",
  "LWS_CALLBACK_CLIENT_RECEIVE",
  "LWS_CALLBACK_CLIENT_RECEIVE_PONG",
  "LWS_CALLBACK_CLIENT_WRITEABLE",
  "LWS_CALLBACK_SERVER_WRITEABLE",
  "LWS_CALLBACK_HTTP",
  "LWS_CALLBACK_HTTP_BODY",
  "LWS_CALLBACK_HTTP_BODY_COMPLETION",
  "LWS_CALLBACK_HTTP_FILE_COMPLETION",
  "LWS_CALLBACK_HTTP_WRITEABLE",
  "LWS_CALLBACK_FILTER_NETWORK_CONNECTION",
  "LWS_CALLBACK_FILTER_HTTP_CONNECTION",
  "LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED",
  "LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION",
  "LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS",
  "LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS",
  "LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION",
  "LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER",
  "LWS_CALLBACK_CONFIRM_EXTENSION_OKAY",
  "LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED",
  "LWS_CALLBACK_PROTOCOL_INIT",
  "LWS_CALLBACK_PROTOCOL_DESTROY",
  "LWS_CALLBACK_WSI_CREATE",
  "LWS_CALLBACK_WSI_DESTROY",
  "LWS_CALLBACK_GET_THREAD_ID",
  "LWS_CALLBACK_ADD_POLL_FD",
  "LWS_CALLBACK_DEL_POLL_FD",
  "LWS_CALLBACK_CHANGE_MODE_POLL_FD",
  "LWS_CALLBACK_LOCK_POLL",
  "LWS_CALLBACK_UNLOCK_POLL",
  "LWS_CALLBACK_USER"
};
struct GamenodeContext
{

};

*/
bool running = true;
/*
int callback_gamenode(libwebsocket_context *context, libwebsocket *wsi, libwebsocket_callback_reasons reason,
                      void *user, void *in, size_t len)
{

  std::cout << LWS_EXT_CALLBACK_STR[reason] << std::endl;

  switch(reason)
  {
    case LWS_CALLBACK_ESTABLISHED: break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR: break;
    case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH: break;
    case LWS_CALLBACK_CLIENT_ESTABLISHED: break;
    case LWS_CALLBACK_CLOSED: break;
    case LWS_CALLBACK_CLOSED_HTTP: break;
    case LWS_CALLBACK_RECEIVE: break;
    case LWS_CALLBACK_CLIENT_RECEIVE:
    {
      std::cout << "Received: " << (char*) in << std:: endl;
      lsio_packet_t packet;
      lsio_packet_parse(&packet, (char*) in);
      switch(packet.type)
      {
        case LSIO_PACKET_TYPE_ACK: break;
        case LSIO_PACKET_TYPE_UNDEFINED: break;
        case LSIO_PACKET_TYPE_DISCONNECT: break;
        case LSIO_PACKET_TYPE_CONNECT: break;
        case LSIO_PACKET_TYPE_HEARTBEAT:break;
        case LSIO_PACKET_TYPE_MESSAGE: break;
        case LSIO_PACKET_TYPE_JSON_MESSAGE: break;
        case LSIO_PACKET_TYPE_EVENT: break;
        case LSIO_PACKET_TYPE_ACK: break;
        case LSIO_PACKET_TYPE_ERROR: break;
        case LSIO_PACKET_TYPE_NOOP: break;
      }

      break;
    }
    case LWS_CALLBACK_CLIENT_RECEIVE_PONG: break;
    case LWS_CALLBACK_CLIENT_WRITEABLE: break;
    case LWS_CALLBACK_SERVER_WRITEABLE: break;
    case LWS_CALLBACK_HTTP: break;
    case LWS_CALLBACK_HTTP_BODY: break;
    case LWS_CALLBACK_HTTP_BODY_COMPLETION: break;
    case LWS_CALLBACK_HTTP_FILE_COMPLETION: break;
    case LWS_CALLBACK_HTTP_WRITEABLE: break;
    case LWS_CALLBACK_FILTER_NETWORK_CONNECTION: break;
    case LWS_CALLBACK_FILTER_HTTP_CONNECTION: break;
    case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED: break;
    case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION: break;
    case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS: break;
    case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS: break;
    case LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION: break;
    case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER: break;
    case LWS_CALLBACK_CONFIRM_EXTENSION_OKAY: break;
    case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED: break;
    case LWS_CALLBACK_PROTOCOL_INIT: break;
    case LWS_CALLBACK_PROTOCOL_DESTROY: break;
    case LWS_CALLBACK_WSI_CREATE: break;
    case LWS_CALLBACK_WSI_DESTROY:
      running = false;
      break;
    case LWS_CALLBACK_GET_THREAD_ID: break;
    default: break;
  }

  return 0;
}
*/

void gamenodeCallback(gamenode* gn, gamenodeEvent const* event)
{
  switch(event->type)
  {
    case GAMENODE_CONNECTED:
    {
      std::cout << "GAMENODE_CONNECTED" << std::endl;
      JSONValue credentials = JSONValue::object();
      credentials.set("username", JSONValue::string("bzar"));
      credentials.set("password", JSONValue::string("bzar"));
      long msgId = gamenodeMethodCall(gn, "newSession", credentials.extract());
      std::cout << "Tried to log in, msgId=" << msgId << std::endl;
      break;
    }
    case GAMENODE_DISCONNECTED:
    {
      running = false;
      break;
    }
    case GAMENODE_RESPONSE:
    {
      JSONValue value(event->response.value);
      std::cout << "Got response to message id " << event->response.id << ": " << value.toString() << std::endl;
      break;
    }
    default: break;
  }
}

int main(int argc, char** argv)
{
  lws_set_log_level(LLL_DEBUG, nullptr);
  gamenode* gn = gamenodeNew(gamenodeCallback);
  if(gamenodeConnect(gn, "localhost", 8888, "/socket.io/1/websocket", "localhost", "localhost"))
  {
    std::cerr << "Error creating gamenode connection" << std::endl;
    return -1;
  }

  while(running)
  {
    if(gamenodeHandle(gn))
    {
      break;
    }
  }

/*  lws_context_creation_info info = { 0 };
  libwebsocket_protocols protocols[] = {
    {"gamenode", callback_gamenode, sizeof(GamenodeContext), 1024, 1},
    { 0 }
  };

  info.port = CONTEXT_PORT_NO_LISTEN;
  info.gid = -1;
  info.uid = -1;
  info.protocols = protocols;
  info.user = new GamenodeContext;

  libwebsocket_context* context = libwebsocket_create_context(&info);

  if (!context)
  {
    std::cerr << "ERROR: Could not create websocket context" << std::endl;
    return 1;
  }


  libwebsocket* ws = libwebsocket_client_connect(context, "localhost", 8888, 0, "/socket.io/1/websocket", "localhost", "localhost", "gamenode", -1);
  if(!ws)
  {
    std::cerr << "ERROR: Could not create websocket" << std::endl;
    return 1;
  }

  int n = 0;
  while(n >= 0 && running)
  {
    libwebsocket_service(context, 10);
  }

  libwebsocket_context_destroy(context);
*/


  return 0;
}

