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
  TCUnknownEvent = 0,
  TCAlarmOnEvent = 'A',
  TCAlarmOffEvent = 'a'
};

enum TCCommandType {
  TCUnknownCommand = 255,
  TCAlarmOnCommand = 0,
  TCAlarmOffCommand = 1
};

struct TCPacket {
  TCPacketType type;
  uint32_t size;
  uint8_t* data;
};

extern BluetoothSerial SerialBT;

// Setup stuff
void transcoder_init(void);

// Memory stuff
TCPacket* transcoder_alloc_packet(TCPacketType type, uint32_t size); // _DOES_ allocate memory for the packet's data
void transcoder_free_packet(TCPacket* packet); // _DOES_ free a packet's data

// Write stuff
void transcoder_send_packet(enum TCPacketType packetType, uint32_t packetSize, uint8_t* packetData);
void transcoder_send_packet(enum TCPacketType packetType, uint8_t packetData);
void transcoder_send_frame(camera_fb_t* frame);
void transcoder_send_event(TCEventType event);
void transcoder_send_face_detection(float tl_x, float tl_y, float br_x, float br_y);

// Read stuff
TCPacket* transcoder_receive_packet(void);

#endif
