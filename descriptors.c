#include <stddef.h>
#include "descriptors.h"

static const LADSPA_Descriptor *descriptors[] = {
  &orbit_descriptor,
  &delay_descriptor,
  &orbital_delay_descriptor,
  NULL,
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
  return descriptors[index];
}
