#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "ladspa.h"

enum {
  UID_ORBIT = 1,
  UID_DELAY,
  UID_ORBITAL_DELAY,
};

extern const LADSPA_Descriptor
  orbit_descriptor,
  delay_descriptor,
  orbital_delay_descriptor;

#endif
