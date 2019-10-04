#include "Arduino.h"
#include "packet_transcoder.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

void transcoder_init() {
  if(!SerialBT.begin("mBot Patrol")) {
    log_e("Bluetooth FAILED");
    exit(0);
  }
  log_i("Bluetooth OK");
}

void encode_le_uint32(uint32_t integer, uint8_t* buffer) {
  buffer[0] = (integer >> 24) & 0xFF;
  buffer[1] = (integer >> 16) & 0xFF;
  buffer[2] = (integer >> 8 ) & 0xFF;
  buffer[3] =  integer        & 0xFF;
}

void transcoder_send_packet(enum TCPacketType packetType, uint8_t* packetData, int32_t packetSize) {
  if(packetSize == 0)
    return;

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
  if(SerialBT.write(packetData, packetSize) == 0) {
    log_e("Failed to send BT packet data");
    return;
  }
}

void transcoder_send_packet(enum TCPacketType packetType, uint8_t packetData) {
  uint8_t buffer[1];
  buffer[0] = packetData;
  transcoder_send_packet(packetType, buffer, 1);
}

void transcoder_send_frame(camera_fb_t* frame) {
  transcoder_send_packet(TCFrame, frame->buf, frame->len);
}

void transcoder_send_event_alarm() {
  transcoder_send_packet(TCEvent, (uint8_t)TCAlarmEvent);
}

void transcoder_send_face_detection(float tl_x, float tl_y, float br_x, float br_y) {
  uint8_t pointBuffer[16];
  encode_le_uint32((uint32_t)tl_x, pointBuffer     );
  encode_le_uint32((uint32_t)tl_y, pointBuffer + 4 );
  encode_le_uint32((uint32_t)br_x, pointBuffer + 8 );
  encode_le_uint32((uint32_t)br_y, pointBuffer + 12);
  
  transcoder_send_packet(TCFace, pointBuffer, 16);
}
