#ifndef PACKET_TRANSCODER_H
#define PACKET_TRANSCODER_H
#define TX_QUEUE_SIZE 8

#include "BluetoothSerial.h"
#include "esp_camera.h"

enum TCPacketType {
  TCUnknownPacket = 255,
  TCFrame = 0,
  TCEvent = 1,
  TCCommand = 2,
  TCFace = 3
};

enum TCEventType {
  TCUnknownEvent = 255,
  TCAlarmEvent = 0
};

extern BluetoothSerial SerialBT;

void transcoder_init(void);
void transcoder_send_packet(enum TCPacketType packetType, uint8_t* packetData, int32_t packetSize);
void transcoder_send_packet(enum TCPacketType packetType, uint8_t packetData);
void transcoder_send_frame(camera_fb_t* frame);
void transcoder_send_event_alarm(void);
void transcoder_send_face_detection(uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br);

#endif
