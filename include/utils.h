#ifndef UTILS_H
#define UTILS_H

#define MAKER "Ludvig Gunne Lindström"
#define COPYRIGHT "None"

#define PI 3.14159265358979f

#define PORT_RANGE_HINTS_BOUNDED_FLOAT(min, max, extra) \
  {                                                     \
    .HintDescriptor =                                   \
      LADSPA_HINT_BOUNDED_BELOW |                       \
      LADSPA_HINT_BOUNDED_ABOVE |                       \
      LADSPA_HINT_DEFAULT_MIDDLE | extra,               \
    .LowerBound = min,                                  \
    .UpperBound = max,                                  \
  }

#endif
