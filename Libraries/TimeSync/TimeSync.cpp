/*******************************************************************************
 * This class performs time synchronization between modules, providing a
 * synchonized clock API.
 *
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2015
 *
 *******************************************************************************
 *
 * Protocol
 * - This writeup assumes a master/slave relationship, but could also operate
 *   in a masterless protocol
 * 
 * Synchronization:
 *   1) M initiates time synchronization
 *     - M sends sync initiation message (SYNC), 
 *     - M enters hot-wait mode
 *   2) S receives SYNC
 *     - S sends SYNC_ACK at time=t_a
 *     - S enters hot-wait mode
 *   3) M receives SYNC_ACK
 *     - M sends SYNC set with time=t_s
 *     - M exits hot wait
 *   4) S recieves SYNC_SET at time=t_b
 *     - S sets latency = (t_b - t_a)/2 + PROCESSING TIME(calculate this?)
 *     - S sets delta = t_b - (t_s + L)
 *     - S exits hot-wait
 *
 * Resynchronization:
 *   1) M transmits time resync message RESYNC with time=t_r
 *   2) S receives RESYNC at time t=t_a
 *     - sets delta = t_a - (t_r + L)
 *
 ******************************************************************************/

#include <Arduino.h>

#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL DEBUG_HIGH
#endif
#include "Debug.h"

#include "TimeSync.h"

#include "Socket.h"
#include "HMTLTypes.h"
#include "HMTLMessaging.h"
#include "HMTLProtocol.h"

TimeSync::TimeSync() {
  latency = 0;
  delta = 0;
  state = STATE_IDLE;
  last_msg_time = 0;
}

/*
 * Return the current time adjusted based on the derived time delta
 */
unsigned long TimeSync::ms() {
  return millis() + delta;
}

unsigned long TimeSync::s() {
  return ms() / 1000;
}

void TimeSync::sendSyncMsg(Socket *socket, socket_addr_t target, byte phase) {
  if (socket->send_data_size < HMTL_MSG_SIZE(msg_time_sync_t)) {
    DEBUG_ERR("startsync to small");
  }

  msg_hdr_t *msg_hdr = (msg_hdr_t *)socket->send_buffer;
  msg_time_sync_t *msg_time = (msg_time_sync_t *)(msg_hdr + 1);
  msg_time->sync_phase = phase;
  msg_time->timestamp = ms();

  hmtl_msg_fmt(msg_hdr, target, HMTL_MSG_SIZE(msg_time_sync_t), MSG_TYPE_TIMESYNC);
  socket->sendMsgTo(target, socket->send_buffer, HMTL_MSG_SIZE(msg_time_sync_t));

}

/*
 * Run the synchronization procedure
 *
 * Returns true if synchronization is ongoing and hot-wait mode should be
 * maintained.
 */
// TODO: Static method?
boolean TimeSync::synchronize(Socket *socket,
                              socket_addr_t target,
                              msg_hdr_t *msg_hdr) {
  if (msg_hdr == NULL) {
    /* This was called to synchronize the times to a remote address */
    if (state != STATE_IDLE) {
      // XXX: What should we do if a sync was already pending?  For now
      // just continue as if it weren't
    }

    /* Begin the synchronization process */
    DEBUG3_VALUELN("SYNC to:", target);
    sendSyncMsg(socket, target, TIMESYNC_SYNC);
    state = STATE_AWAITING_ACK;
  } else {

    if (msg_hdr->type != MSG_TYPE_TIMESYNC) {
      // TODO: What to do here?
      goto EXIT;
    }

    unsigned long now = millis();
    msg_time_sync_t *msg_time = (msg_time_sync_t *)(msg_hdr + 1);
    socket_addr_t source = socket->sourceFromData(msg_hdr);
    
    switch (state) {
      case STATE_IDLE:
      case STATE_SYNCED: {
        if (msg_time->sync_phase == TIMESYNC_SYNC) {
          // Record the timestamp to compute the latency
          DEBUG3_VALUE("SYNC from", source);
          latency = now;

          // Reply with ack
          sendSyncMsg(socket, source, TIMESYNC_ACK);
          state = STATE_AWAITING_SET;
        } else if (msg_time->sync_phase == TIMESYNC_RESYNC) {
          if (state == STATE_SYNCED) {
            delta = now - (msg_time->timestamp + latency);
            DEBUG3_VALUE("RESYNC from", source);
            DEBUG3_VALUELN(" delta:", delta);
          }
        } else {
          // TODO: What to do with other time sync messages
        }
        break;
      }

      case STATE_AWAITING_ACK: {
        if (msg_hdr != NULL) {
          // Check if this is a response to the message we sent
          if (msg_time->sync_phase == TIMESYNC_ACK) {
            // Send TIMESYNC_SET
            DEBUG3_VALUELN("ACK from:", source);
            sendSyncMsg(socket, source, TIMESYNC_SET);
            state = STATE_IDLE;
          } else {
            // TODO: What to do with other time sync messages?
          }
        }
        break;
      }

      case STATE_AWAITING_SET: {
        if (msg_hdr != NULL) {
          if (msg_time->sync_phase == TIMESYNC_SET) {
            /*
             * Set the latecy to 1/2 the time between sending the ack and receiving
             * this message, and then use that latecy to compute the delta from
             * the sender's time.
             */
            DEBUG3_VALUE("SET from:", source);
            latency = (now - latency) / 2;
            delta = now - (msg_time->timestamp + latency);
            state = STATE_SYNCED;

            DEBUG3_VALUE(" lat:", latency);
            DEBUG3_VALUELN(" delta:", delta);
          } else {
            // TODO: What to do with other time sync messages?
          }
        }
        break;
      }
    }
  }

 EXIT:
  return (state != STATE_IDLE) && (state != STATE_SYNCED);
}

/*
 * Send a resynchronization message with the current timestamp
 */
void TimeSync::resynchronize(Socket *socket, socket_addr_t target) {
  sendSyncMsg(socket, target, TIMESYNC_RESYNC);
}
