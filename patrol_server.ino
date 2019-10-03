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
    camera_fb_t* fb = esp_camera_fb_get();
    if(fb) {
      transcoder_send_frame(fb);

      // Free frame
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
