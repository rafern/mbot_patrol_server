#include "faces.h"
#include "camera.h"
#include "packet_transcoder.h"

/*             /!\ IMPORTANT /!\
 *  MAKE SURE TO TURN THE DEBUG LEVEL TO NONE 
 *  WHEN USING SERIAL TO TALK WITH THE MBOT
 */
 
void setup() {
  Serial.begin(115200);
  transcoder_init();
  faces_init();
  camera_init();
  log_i("Ready");
}

void loop() {
  log_i("Free heap: %zu", ESP.getFreeHeap());
  
  // Only do bluetooth stuff if there is a peer connected
  if(!SerialBT.hasClient()) {
    // Flush all events from the mBot as no devices are connected
    Serial.flush();
    delay(500);
    return;
  }

  // Process serial events from the mBot
  int thisByte;
  while((thisByte = Serial.read()) != -1) {
    TCEventType event = (TCEventType)thisByte;
    switch(event) {
    case 'A':
      transcoder_send_event(TCAlarmOnEvent);
      break;
    case 'a':
      transcoder_send_event(TCAlarmOffEvent);
      break;
    default:
      log_i("Invalid event received: %c (%hhx)", event, event);
    }
  }

  // Process bluetooth commands
  TCPacket* packet = transcoder_receive_packet();
  if(packet) {
    // Ignore non-command or invalid command packets
    if(packet->type == TCCommand && packet->size == 1) {
      TCCommandType command = (TCCommandType)packet->data[0];
      switch(command) {
      case TCAlarmOnCommand:
        Serial.write('A');
        break;
      case TCAlarmOffCommand:
        Serial.write('a');
        break;
      default:
        log_i("Invalid command: %c (%hhx)", command, command);
      }
    }
    else
      log_i("Ignored a bluetooth packet");
    
    // Free packet
    transcoder_free_packet(packet);
  }

  // Get next frame and send it
  camera_fb_t* fb = esp_camera_fb_get();
  if(!fb) {
    log_e("Failed to get frame");
    return;
  }

  transcoder_send_frame(fb);

  // Convert frame to pixel map
  dl_matrix3du_t* matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
  if(matrix) {
    bool conversionSucceeded = fmt2rgb888(fb->buf, fb->len, fb->format, matrix->item);

    // Done with camera
    esp_camera_fb_return(fb);
    
    if(conversionSucceeded) {
      // Do facial detection
      box_array_t* detections = faces_detect(matrix);
      if(detections) {
        for(int i = 0; i < detections->len; ++i)
          transcoder_send_face_detection(detections->box[i].box_p[0], detections->box[i].box_p[1], detections->box[i].box_p[2], detections->box[i].box_p[3]);

        // Free detections struct
        free(detections->score);
        free(detections->box);
        free(detections->landmark);
        free(detections);
      }
    }
    else
      log_e("Failed to convert frame to pixel matrix");

    // Free matrix
    dl_matrix3du_free(matrix);
  }
  else {
    // Done with camera
    esp_camera_fb_return(fb);
    log_e("Failed to allocate pixel matrix");
  }
}
