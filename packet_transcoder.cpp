#include "Arduino.h"
#include "packet_transcoder.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
uint8_t readHeaderBuffer[5];
int readHeaderBufferSize = 0;
TCPacket* readPacket = NULL;
size_t readPacketData = 0;

void encode_le_uint32(uint32_t integer, uint8_t* buffer) {
  buffer[0] = (integer >> 24) & 0xFF;
  buffer[1] = (integer >> 16) & 0xFF;
  buffer[2] = (integer >> 8 ) & 0xFF;
  buffer[3] =  integer        & 0xFF;
}

uint32_t decode_le_uint32(uint8_t* buffer) {
  return (uint32_t)buffer[3]
       + (uint32_t)buffer[2] << 8
       + (uint32_t)buffer[1] << 16
       + (uint32_t)buffer[0] << 24;
}

void transcoder_init() {
  if(!SerialBT.begin("mBot Patrol")) {
    log_e("Bluetooth FAILED");
    exit(0);
  }
  log_i("Bluetooth OK");
}

TCPacket* transcoder_alloc_packet(TCPacketType type, uint32_t size) {
  if(size == 0) {
    log_e("Attempt to allocate packet with 0 size");
    return NULL;
  }

  TCPacket* packet = (TCPacket*)malloc(sizeof(TCPacket));

  if(!packet) {
    log_e("Failed to allocate transcoder packet");
    return NULL;
  }

  packet->data = (uint8_t*)malloc(size);

  if(!packet->data) {
    log_e("Failed to allocate transcoder packet's data buffer");
    free(packet);
    return NULL;
  }

  packet->type = type;
  packet->size = size;
  return packet;
}

void transcoder_free_packet(TCPacket* packet) {
  free(packet->data);
  free(packet);
}

void transcoder_send_packet(enum TCPacketType packetType, uint32_t packetSize, uint8_t* packetData) {
  // Send header
  // Packet type
  if(SerialBT.write((uint8_t)packetType) == 0) {
    log_e("Failed to send BT packet type");
    return;
  }

  // Packet size
  uint8_t sizeField[4];
  encode_le_uint32(packetSize, sizeField);
  if(SerialBT.write(sizeField, 4) == 0) {
    log_e("Failed to send BT packet size");
    return;
  }

  // Packet data
  if(packetSize && SerialBT.write(packetData, packetSize) == 0) {
    log_e("Failed to send BT packet data");
    return;
  }
}

void transcoder_send_packet(enum TCPacketType packetType, uint8_t packetData) {
  uint8_t tmpBuf[1];
  tmpBuf[0] = packetData;
  
  transcoder_send_packet(packetType, 1, tmpBuf);
}

void transcoder_send_frame(camera_fb_t* frame) {
  transcoder_send_packet(TCFrame, frame->len, frame->buf);
}

void transcoder_send_event(TCEventType event) {
  transcoder_send_packet(TCEvent, (uint8_t)event);
}

void transcoder_send_face_detection(float tl_x, float tl_y, float br_x, float br_y) {
  uint8_t pointBuffer[16];
  encode_le_uint32((uint32_t)tl_x, pointBuffer     );
  encode_le_uint32((uint32_t)tl_y, pointBuffer + 4 );
  encode_le_uint32((uint32_t)br_x, pointBuffer + 8 );
  encode_le_uint32((uint32_t)br_y, pointBuffer + 12);
  
  transcoder_send_packet(TCFace, 16, pointBuffer);
}

TCPacket* transcoder_receive_packet(void) {
  if(readPacket) {
    // If the temporary read packet is already allocated, then write data to its buffer
    int thisByte;
    while((thisByte = SerialBT.read()) != -1) {
      readPacket->data[readPacketData++] = thisByte;
      
      if(readPacketData == readPacket->size) {
        // Done with this packet, we no longer care about it, return it
        TCPacket* packet = readPacket;
        readPacket = NULL;
        readPacketData = 0;
        return packet;
      }
    }
  }
  else {
    // No temporary packet, read to header buffer
    int thisByte;
    while((thisByte = SerialBT.read()) != -1) {
      readHeaderBuffer[readHeaderBufferSize++] = (uint8_t)thisByte;
      if(readHeaderBufferSize == 5) {
        // Decode packet type
        TCPacketType packetType = TCUnknownPacket;
        switch(readHeaderBuffer[0]) {
        case TCFrame:
          packetType = TCFrame;
          break;
        case TCEvent:
          packetType = TCEvent;
          break;
        case TCCommand:
          packetType = TCCommand;
          break;
        case TCFace:
          packetType = TCFace;
          break;
        }

        // Decode packet size
        uint32_t packetSize = decode_le_uint32(readHeaderBuffer + 1);

        // Allocate packet
        readPacket = transcoder_alloc_packet(packetType, packetSize);

        // Done with temporary packet header buffer
        readHeaderBufferSize = 0;

        // Try to get the rest of the packet
        return transcoder_receive_packet();
      }
    }
  }

  // No full packets read... yet
  return NULL;
}
