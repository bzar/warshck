#include "gamenode.h"
#include "libwebsockets.h"
#include "libsocketio.h"

#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>
#include <time.h>

typedef struct queueData {
  char* data;
  int size;
  struct queueData* next;
} queueData;

typedef struct _gamenode {
  struct libwebsocket_context* wsCtx;
  struct libwebsocket* ws;
  struct lws_context_creation_info wsInfo;
  struct queueData* writeQueue;

  char* readBuffer;
  size_t readBufferSize;

  char* sioSessionId;
  int sioHeartbeatInterval;
  int sioConnectionTimeout;
  time_t sioPreviousHeartbeat;

  long gamenodeMessageId;
  gamenodeCallbackFunc callback;

  char** methodNames;
  unsigned int numMethodNames;

  void* userData;
} _gamenode;

static int callback_gamenode(struct libwebsocket_context *context, struct libwebsocket *wsi,
                             enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len);

gamenode* gamenodeNew(gamenodeCallbackFunc callback)
{
  gamenode* gn = calloc(1, sizeof(gamenode));
  gn->callback = callback;
  gn->writeQueue = NULL;
  return gn;
}

static struct libwebsocket_protocols wsProtocols[] = {
{"gamenode", callback_gamenode, 0, 1024, 1},
{ 0 }
};

void gamenodeFree(gamenode* gn)
{
  if(gn->wsCtx)
  {
    gamenodeDisconnect(gn);
  }

  int i;
  if (gn->methodNames)
  {
    for (i = 0; i < gn->numMethodNames; ++i)
    {
      free(gn->methodNames[i]);
    }
    free (gn->methodNames);
  }

  if(gn->sioSessionId)
  {
    free(gn->sioSessionId);
  }

  free(gn);
}

typedef struct MemoryStruct {
  char *memory;
  size_t size;
} MemoryStruct;

static size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static char* httpPost(char const* address, int port)
{
  struct MemoryStruct chunk;
  chunk.memory = malloc(1);
  chunk.size = 0;

  CURL* curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, address);
  curl_easy_setopt(curl, CURLOPT_PORT, port);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeMemoryCallback);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if(res != CURLE_OK)
  {
    free(chunk.memory);
    return 0;
  }
  return chunk.memory;
}

char gamenodeConnect(gamenode* gn,
                     const char *address,
                     int port,
                     const char *path,
                     const char *host,
                     const char *origin)
{
  char handshakeUrl[256] = { 0 };
  sprintf(handshakeUrl, "http://%s/socket.io/1/websocket", address);
  char* handshakeContent = httpPost(handshakeUrl, port);
  if(!handshakeContent)
  {
    printf("Error getting handshake information\n");
    return -1;
  }

  char* sessionId = strtok(handshakeContent, ":");
  gn->sioSessionId = strdup(sessionId);
  char* heartbeatInterval = strtok(NULL, ":");
  gn->sioHeartbeatInterval = atoi(heartbeatInterval);
  char* connectionTimeout = strtok(NULL, ":");
  gn->sioConnectionTimeout = atoi(connectionTimeout);
  char* transports = strtok(NULL, ":");

  if(!strstr(transports, "websocket"))
  {
    printf("Websocket transport not accepted by server\n");
    return -1;
  }

  free(handshakeContent);

  gn->wsInfo.port = CONTEXT_PORT_NO_LISTEN;
  gn->wsInfo.gid = -1;
  gn->wsInfo.uid = -1;
  gn->wsInfo.protocols = wsProtocols;
  gn->wsInfo.user = gn;
  gn->wsInfo.ka_time = 1;
  gn->wsInfo.ka_interval = 5;
  gn->wsInfo.ka_probes = 5;

  gn->wsCtx = libwebsocket_create_context(&gn->wsInfo);
  if(!gn->wsCtx)
  {
    return -1;
  }

  char transportPath[256] = {0};
  sprintf(transportPath, "/socket.io/1/websocket/%s", gn->sioSessionId);
  gn->ws = libwebsocket_client_connect(gn->wsCtx, address, port, 0, transportPath, host, origin, "gamenode", -1);
  if(!gn->ws)
  {
    libwebsocket_context_destroy(gn->wsCtx);
    gn->wsCtx = NULL;
    return -1;
  }

  return 0;
}

void gamenodeDisconnect(gamenode* gn)
{
  if (gn->wsCtx)
  {
    libwebsocket_context_destroy(gn->wsCtx);
    gn->wsCtx = NULL;
    gn->ws = NULL;
    gn->gamenodeMessageId = 0;
  }

  while(gn->writeQueue)
  {
    queueData* qd = gn->writeQueue;
    gn->writeQueue = gn->writeQueue->next;
    free(qd->data);
    free(qd);
  }
}

static void queueDataForSending(gamenode* gn, char const* data)
{
  //printf("Queuing data: %s\n", data);
  size_t qdSize = sizeof(queueData);
  struct queueData* qData = calloc(2, qdSize);
  qData->size = strlen(data);
  qData->data = calloc(1, LWS_SEND_BUFFER_PRE_PADDING + qData->size + LWS_SEND_BUFFER_POST_PADDING);
  strcpy(qData->data + LWS_SEND_BUFFER_PRE_PADDING, data);
  qData->next = NULL;
  // Find end of queue and append
  queueData** last = &gn->writeQueue;
  while(*last) last = &(*last)->next;
  *last = qData;
}

char gamenodeHandle(gamenode* gn)
{
  // Request write for heartbeat if necessary
  time_t now;
  time(&now);
  if(now - gn->sioPreviousHeartbeat > gn->sioHeartbeatInterval / 2)
  {
    queueDataForSending(gn, "2:::");
    gn->sioPreviousHeartbeat = now;
  }

  if (gn->writeQueue)
  {
    libwebsocket_callback_on_writable(gn->wsCtx, gn->ws);
  }
  return libwebsocket_service(gn->wsCtx, 0);
}

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

long int sendMessage(gamenode* gn, long msgId, chckJson* msg)
{
  size_t size;
  char* msgData = chckJsonEncode(msg, &size);
  int headerSize = snprintf(NULL, 0, "3:%d::", msgId);
  char* msgStr = calloc(size + headerSize, sizeof(char));
  sprintf(msgStr, "3:%d::", msgId);
  strcat(msgStr, msgData);
  free(msgData);
  queueDataForSending(gn, msgStr);
  free(msgStr);

  return msgId;
}

void sendMethodList(gamenode* gn)
{
  chckJson* methods = chckJsonNew(CHCK_JSON_TYPE_ARRAY);

  int i;
  for (i = 0; i < gn->numMethodNames; ++i)
  {
    chckJsonChildAppend(methods, chckJsonNewString(gn->methodNames[i]));
  }

  long msgId = gn->gamenodeMessageId++;

  chckJson* msg = chckJsonNew(CHCK_JSON_TYPE_OBJECT);
  chckJsonProperty(msg,  "id", chckJsonNewNumberLong(msgId));
  chckJsonProperty(msg,  "type", chckJsonNewString("methodList"));
  chckJsonProperty(msg,  "content", methods);

  sendMessage(gn, msgId, msg);

  chckJsonFreeAll(msg);
}

static int callback_gamenode(struct libwebsocket_context *context, struct libwebsocket *wsi,
                             enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
{
  gamenode* gn = (gamenode*) libwebsocket_context_user(context);
  if(reason != LWS_CALLBACK_GET_THREAD_ID && reason != LWS_CALLBACK_LOCK_POLL)
    printf("%s\n", LWS_EXT_CALLBACK_STR[reason]);

  switch(reason)
  {
    case LWS_CALLBACK_ESTABLISHED: break;
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    {
      gamenodeEvent event;
      event.type = GAMENODE_ERROR;
      gn->callback(gn, &event);
      break;
    }
    case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH: break;
    case LWS_CALLBACK_CLIENT_ESTABLISHED: break;

    case LWS_CALLBACK_CLOSED: break;
    case LWS_CALLBACK_CLOSED_HTTP: break;
    case LWS_CALLBACK_RECEIVE: break;
    case LWS_CALLBACK_CLIENT_RECEIVE:
    {
      const size_t remaining = libwebsockets_remaining_packet_payload(wsi);
      if(remaining || !libwebsocket_is_final_fragment(wsi))
      {
        // Not last fragment, buffer
        if(!gn->readBuffer)
        {
          gn->readBufferSize = len + remaining + 1;
          gn->readBuffer = calloc(gn->readBufferSize, sizeof(char));
        }
        strcat(gn->readBuffer, (char*)  in);
        return 0;
      }

      char* buffer = (char*) in;
      if(gn->readBuffer)
      {
        // Last fragment, data previously buffered
        strcat(gn->readBuffer, (char*)  in);
        buffer = gn->readBuffer;
      }

      //printf("Received: %s\n", buffer);
      lsio_packet_t* packet = calloc(1, sizeof(lsio_packet_t));
      lsio_packet_init(packet);

      int parseResult = lsio_packet_parse(packet, buffer);

      if(parseResult == LSIO_ERROR)
      {
        printf("Error parsing packet\n");
        break;
      }
      //printf("packet type = %d\n", packet.type);

      switch(packet->type)
      {
        case LSIO_PACKET_TYPE_UNDEFINED: break;
        case LSIO_PACKET_TYPE_DISCONNECT:
          return -1;
        case LSIO_PACKET_TYPE_CONNECT:
        {
          gamenodeEvent event;
          event.type = GAMENODE_CONNECTED;
          sendMethodList(gn);
          gn->callback(gn, &event);
          break;
        }
        case LSIO_PACKET_TYPE_HEARTBEAT:break;
        case LSIO_PACKET_TYPE_MESSAGE:
        {
          chckJsonDecoder* decoder = chckJsonDecoderNew();
          chckJson* msg = chckJsonDecoderDecode(decoder, packet->data);
          chckJsonDecoderFree(decoder);
          chckJson* msgType = chckJsonGetProperty(msg, "type");
          //printf("msgType = %s\n", msgType->string_value);
          if(strcmp(chckJsonGetString(msgType), "response") == 0)
          {
            gamenodeEvent event;
            event.type = GAMENODE_RESPONSE;
            event.response.id = chckJsonGetLong(chckJsonGetProperty(msg, "id"));
            event.response.value = chckJsonGetProperty(msg, "content");
            gn->callback(gn, &event);
          }
          else if(strcmp(chckJsonGetString(msgType), "call") == 0)
          {
            gamenodeEvent event;
            event.type = GAMENODE_METHOD_CALL;
            event.methodCall.id = chckJsonGetLong(chckJsonGetProperty(msg, "id"));
            event.methodCall.methodName= chckJsonGetString(chckJsonGetProperty(msg, "method"));
            event.methodCall.params = chckJsonGetProperty(msg, "params");
            gn->callback(gn, &event);
          }
          else if(strcmp(chckJsonGetString(msgType), "methodList") == 0)
          {
          }
          chckJsonFreeAll(msg);
          break;
        }
        case LSIO_PACKET_TYPE_JSON_MESSAGE: break;
        case LSIO_PACKET_TYPE_EVENT: break;
        case LSIO_PACKET_TYPE_ACK: break;
        case LSIO_PACKET_TYPE_ERROR:
        {
          gamenodeEvent event;
          event.type = GAMENODE_ERROR;
          gn->callback(gn, &event);
          break;
        }
        case LSIO_PACKET_TYPE_NOOP: break;
      }

      lsio_packet_free(packet);
      if(gn->readBuffer)
      {
        free(gn->readBuffer);
        gn->readBuffer = NULL;
      }
      //printf("LWS_CALLBACK_CLIENT_RECEIVE handled\n");
      break;
    }
    case LWS_CALLBACK_CLIENT_RECEIVE_PONG: break;
    case LWS_CALLBACK_CLIENT_WRITEABLE:
    {
      while(gn->writeQueue)
      {
        queueData* d = gn->writeQueue;
        if(lws_send_pipe_choked(wsi))
        {
          printf("Send pipe congested while tried to write, skipping sending: %s\n", d->data + LWS_SEND_BUFFER_PRE_PADDING);
          break;
        }
        //printf("Sending: %s\n", d->data + LWS_SEND_BUFFER_PRE_PADDING);
        libwebsocket_write(wsi, d->data + LWS_SEND_BUFFER_PRE_PADDING, d->size, LWS_WRITE_TEXT);
        gn->writeQueue = d->next;
        free(d->data);
      }
      break;
    }
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
    {
      gamenodeEvent event;
      event.type = GAMENODE_DISCONNECTED;
      gn->callback(gn, &event);
      return -1;
    }
    case LWS_CALLBACK_GET_THREAD_ID: break;
    default: break;
  }

  return 0;
}


void gamenodeSetMethodNames(gamenode* gn, const char** methodNames, unsigned int numMethodNames)
{
  int i;
  if (gn->methodNames)
  {
    for (i = 0; i < gn->numMethodNames; ++i)
    {
      free(gn->methodNames[i]);
    }
    free (gn->methodNames);
  }

  gn->methodNames = calloc(numMethodNames, sizeof(char*));
  gn->numMethodNames = numMethodNames;

  for (i = 0; i < gn->numMethodNames; ++i)
  {
    gn->methodNames[i] = strdup(methodNames[i]);
  }

  if(gn->wsCtx)
  {
    sendMethodList(gn);
  }
}

long gamenodeMethodCall(gamenode* gn, const char* methodName, chckJson* params)
{
  chckJson* msg = chckJsonNew(CHCK_JSON_TYPE_OBJECT);
  long msgId = gn->gamenodeMessageId++;
  chckJsonProperty(msg,  "id", chckJsonNewNumberLong(msgId));
  chckJsonProperty(msg,  "type", chckJsonNewString("call"));
  chckJsonProperty(msg,  "method", chckJsonNewString(methodName));

  if(chckJsonGetType(params) == CHCK_JSON_TYPE_ARRAY)
  {
    chckJsonProperty(msg,  "params", params);
  }
  else
  {
    chckJson* paramArray = chckJsonNew(CHCK_JSON_TYPE_ARRAY);
    chckJsonChildAppend(paramArray, params);
    chckJsonProperty(msg,  "params", paramArray);
  }

  sendMessage(gn, msgId, msg);
  chckJsonFreeAll(msg);
  return msgId;
}


void gamenodeResponse(gamenode* gn, long int msgId, chckJson* value)
{
  chckJson* msg = chckJsonNew(CHCK_JSON_TYPE_OBJECT);
  chckJsonProperty(msg,  "id", chckJsonNewNumberLong(msgId));
  chckJsonProperty(msg,  "type", chckJsonNewString("response"));
  chckJsonProperty(msg,  "content", value);

  sendMessage(gn, ++gn->gamenodeMessageId, msg);
  chckJsonFreeAll(msg);
}

void gamenodeSetUserData(gamenode* gn, void* data)
{
  gn->userData = data;
}


void* gamenodeUserData(gamenode* gn)
{
  return gn->userData;
}
