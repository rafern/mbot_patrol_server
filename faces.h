#ifndef FACES_H
#define FACES_H
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"

void faces_init(void);
box_array_t* faces_detect(dl_matrix3du_t* pixBuffer);

#endif
