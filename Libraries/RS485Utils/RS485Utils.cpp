
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "RS485Utils.h"

// XXX: Need to figure out how to use non-static function pointers so this
//      can be a class field.
SoftwareSerial *serial;

RS485Socket::RS485Socket(byte _recvPin, byte _xmitPin, byte _enablePin) 
{
  RS485Socket(_recvPin, _xmitPin, _enablePin, false);
}

RS485Socket::RS485Socket(byte _recvPin, byte _xmitPin, byte _enablePin,
                         boolean _debug) 
{
  recvPin = _recvPin;
  xmitPin = _xmitPin;
  enablePin = _enablePin;
  debug = _debug;

  serial = new SoftwareSerial(recvPin, xmitPin);
  if (debug) {
    channel = new RS485(serialDebugRead, serialAvailable, serialDebugWrite,
                        RS485_RECV_BUFFER);
  } else {
    channel = new RS485(serialRead, serialAvailable, serialWrite,
                        RS485_RECV_BUFFER);
  }

  currentMsgID = 0;
}

void RS485Socket::setup() 
{
  
  serial->begin(28800);
  channel->begin();
  
  pinMode(enablePin, OUTPUT);

  digitalWrite(enablePin, LOW);
}


size_t RS485Socket::serialWrite(const byte value) 
{
  return serial->write(value);
}

size_t RS485Socket::serialDebugWrite(const byte value) 
{
  DEBUG_HEX(DEBUG_HIGH, value);
  DEBUG_PRINT(DEBUG_HIGH, " ");
  return serial->write(value);
}


int RS485Socket::serialRead() 
{
  return serial->read();
}

int RS485Socket::serialDebugRead() 
{
  int value = serial->read();
  DEBUG_HEX(DEBUG_HIGH, value);
  DEBUG_PRINT(DEBUG_HIGH, " ");
  return value;
}

int RS485Socket::serialAvailable() 
{
  return serial->available();
}


byte * RS485Socket::initBuffer(byte * data) 
{
  return data + sizeof (rs485_socket_hdr_t);
}

void RS485Socket::sendMsgTo(byte address,
                            const byte *data,
                            const byte datalength) 
{
  rs485_socket_msg_t *msg =
    (rs485_socket_msg_t *)(data - sizeof (rs485_socket_hdr_t));

  unsigned int msg_len = sizeof (rs485_socket_hdr_t) + datalength;

  msg->hdr.ID = currentMsgID++;
  msg->hdr.length = datalength;
  msg->hdr.address = address;
  msg->hdr.flags = 0;

  if (debug) {
    DEBUG_VALUE(DEBUG_HIGH, "XMIT:", msg_len);
    DEBUG_PRINT(DEBUG_HIGH, " ");
    printSocketMsg(msg);
  }

  digitalWrite(enablePin, HIGH);
  channel->sendMsg((byte *)msg, msg_len);
  digitalWrite(enablePin, LOW);
}

const byte *RS485Socket::getMsg(byte address, unsigned int *retlen) 
{
//  if (debug) Serial.print(F(".")); // Uncomment to print every call
  if (channel->update()) {
    if (debug) {
      DEBUG_VALUE(DEBUG_HIGH, "getMsg:", getLength());
    }

    const rs485_socket_msg_t *msg = (rs485_socket_msg_t *)channel->getData();

    if (debug &&
        (getLength() < sizeof (rs485_socket_hdr_t))) {
      DEBUG_ERR("ERROR-length < header");
    }

    if (debug &&
        (getLength() < (sizeof (rs485_socket_hdr_t) + msg->hdr.length))) {
      DEBUG_ERR("ERROR-length < header + data");
    }

    if (debug) {
      DEBUG_PRINT(DEBUG_HIGH, " RECV: ");
      printSocketMsg(msg);
    }

    if (msg->hdr.address == address) {
      *retlen = msg->hdr.length;
      return &msg->data[0];
    }
  }

  *retlen = 0;
  return NULL;
}

byte RS485Socket::getLength()
{
  return channel->getLength();
}

void printSocketMsg(const rs485_socket_msg_t *msg) 
{
DEBUG_HEXVAL(DEBUG_HIGH, "i:", msg->hdr.ID);
DEBUG_HEXVAL(DEBUG_HIGH, " l:", msg->hdr.length);
DEBUG_HEXVAL(DEBUG_HIGH," a:", msg->hdr.address);
DEBUG_HEXVAL(DEBUG_HIGH, " f:", msg->hdr.flags);
printBuffer(msg->data, msg->hdr.length);
}

void printBuffer(const byte *buff, int length) 
{
  for (int b = 0; b < length; b++) {
    DEBUG_HEXVAL(DEBUG_HIGH, " ", buff[b]);
  }
}
