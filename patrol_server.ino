#include "faces.h"
#include "camera.h"
#include "packet_transcoder.h"

void setup() {
  Serial.begin(115200);
  camera_init();
  transcoder_init();
  log_i("Ready");
}

void loop() {
  log_i("Free heap: %zu", ESP.getFreeHeap());
  // Only do bluetooth stuff if there is a peer connected
  if(SerialBT.hasClient()) {
    // Read messages
    // TODO do actual message handling instead of just clearing
    int rxMessageCount = SerialBT.available();
    if(rxMessageCount > 0)
      SerialBT.flush();

    // Get next frame and send it
    log_i("get");
    camera_fb_t* fb = esp_camera_fb_get();
    if(fb) {
      log_i("send");
      transcoder_send_frame(fb);

      // Convert frame to pixel map
      log_i("alloc");
      dl_matrix3du_t* matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
      if(matrix) {
        log_i("convert");
        bool conversionSucceeded = fmt2rgb888(fb->buf, fb->len, fb->format, matrix->item);
        
        if(conversionSucceeded) {
          // Do facial detection
          log_i("detect");
          box_array_t* detections = faces_detect(matrix);
          for(int i = 0; i < detections->len; ++i) {
            log_i("send box");
            log_i("Box val: %f,%f,%f,%f", detections->box[i].box_p[0], detections->box[i].box_p[1], detections->box[i].box_p[2], detections->box[i].box_p[3]);
            transcoder_send_face_detection(detections->box[i].box_p[0], detections->box[i].box_p[1], detections->box[i].box_p[2], detections->box[i].box_p[3]);
          }
        }
        else
          log_e("Failed to convert frame to pixel matrix");

        // Free matrix
        dl_matrix3du_free(matrix);
      }
      else
        log_e("Failed to allocate pixel matrix");

      // Done with camera
      log_i("return");
      esp_camera_fb_return(fb);
    }
    else
      log_e("Failed to get frame");

    // Limit to approximately 30 FPS
    // TODO proper FPS correction
    delay(33);
  }
  else {
    SerialBT.flush();
    log_v("Waiting for client");
    delay(500);
  }
}
