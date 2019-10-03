#ifndef BLUETOOTH_H
#define BLUETOOTH_H

// Initialize bluetooth
void bluetooth_init(const char* name);
// Send serial data
bool bluetooth_write(char* buf, int len);
// Receive serial data
int bluetooth_read(char* &buf);

#endif
