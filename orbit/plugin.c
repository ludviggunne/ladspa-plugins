/*
 * Plugin name: Orbit
 *
 * Description: Simulates the audio source as moving in a circular orbit
 *              around the listeners head.
 */

#include <stdlib.h>
#include <math.h>
#include <ladspa.h>

#define PI 3.14159265358979f

static const LADSPA_Descriptor descriptor;

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
  if (index != 0)
    return NULL;

  return &descriptor;
}

enum {
  PORT_INPUT = 0,
  PORT_FREQUENCY,
  PORT_RADIUS,
  PORT_OUTPUT_LEFT,
  PORT_OUTPUT_RIGHT,
  _PORT_COUNT,
};

const LADSPA_PortDescriptor port_descriptors[_PORT_COUNT] = {
  [PORT_INPUT]              = LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
  [PORT_FREQUENCY]          = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_RADIUS]             = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_OUTPUT_LEFT]        = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
  [PORT_OUTPUT_RIGHT]       = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
};

const char *const port_names[_PORT_COUNT] = {
  [PORT_INPUT]              = "Input",
  [PORT_FREQUENCY]          = "Frequency",
  [PORT_RADIUS]             = "Radius",
  [PORT_OUTPUT_LEFT]        = "Left output",
  [PORT_OUTPUT_RIGHT]       = "Right output",
};

const LADSPA_PortRangeHint port_range_hints[_PORT_COUNT] = {
  [PORT_INPUT] = {0},
  [PORT_FREQUENCY] = {
    .HintDescriptor =
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_LOGARITHMIC |
      LADSPA_HINT_DEFAULT_LOW,
    .LowerBound = .1f,
    .UpperBound = 440.f
  },
  [PORT_RADIUS] = {
    .HintDescriptor =
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_DEFAULT_LOW,
    .LowerBound = .0f,
    .UpperBound = 10.f
  },
  [PORT_OUTPUT_LEFT] = {0},
  [PORT_OUTPUT_RIGHT] = {0},
};

struct instance {
  unsigned long    sample_rate;
  unsigned long    counter;
  LADSPA_Data     *ports[_PORT_COUNT];
};

LADSPA_Handle instantiate(const struct _LADSPA_Descriptor *descriptor, unsigned long sample_rate);
void connect_port(LADSPA_Handle instance, unsigned long port, LADSPA_Data *data_location);
void run(LADSPA_Handle instance, unsigned long sample_count);
void cleanup(LADSPA_Handle instance);

static const LADSPA_Descriptor descriptor = {
  .UniqueID               = 0,
  .Label                  = "audio",
  .Properties             = 0,
  .Name                   = "Orbit",
  .Maker                  = "Ludvig Gunne Lindström",
  .Copyright              = "None",
  .PortCount              = _PORT_COUNT,
  .PortDescriptors        = port_descriptors,
  .PortNames              = port_names,
  .PortRangeHints         = port_range_hints,
  .ImplementationData     = NULL,
  .instantiate            = instantiate,
  .connect_port           = connect_port,
  .activate               = NULL,
  .run                    = run,
  .run_adding             = NULL,
  .set_run_adding_gain    = NULL,
  .deactivate             = NULL,
  .cleanup                = cleanup,
};

LADSPA_Handle instantiate(const struct _LADSPA_Descriptor *descriptor, unsigned long sample_rate)
{
  (void) descriptor;

  struct instance *const instance_ = calloc(1, sizeof(*instance_));
  if (instance_ == NULL)
    return NULL;

  instance_->sample_rate = sample_rate;
  instance_->counter = 0;

  return (LADSPA_Handle ) instance_;
}

void connect_port(LADSPA_Handle instance, unsigned long port, LADSPA_Data *data_location)
{
  struct instance *const instance_ = (struct instance *) instance;
  instance_->ports[port] = data_location;
}

void run(LADSPA_Handle instance, unsigned long sample_count)
{
  struct instance *const instance_ = (struct instance *) instance;

  for (unsigned long i = 0; i < sample_count; ++i) {

    /* Frequency measured in samples */
    const LADSPA_Data frequency =
      (unsigned long) (*instance_->ports[PORT_FREQUENCY] *
                       (LADSPA_Data) instance_->sample_rate);

    if (instance_->counter++ >= (unsigned long) frequency)
      instance_->counter = 0;

    const LADSPA_Data angle  = (LADSPA_Data) instance_->counter / frequency * 2.f * PI;
    const LADSPA_Data radius = *instance_->ports[PORT_RADIUS];
    const LADSPA_Data dx_l   = radius * cos(angle) - 1.f;
    const LADSPA_Data dx_r   = radius * cos(angle) + 1.f;
    const LADSPA_Data dy     = radius * sin(angle);
    const LADSPA_Data dl2    = dx_l * dx_l + dy * dy;
    const LADSPA_Data dr2    = dx_r * dx_r + dy * dy;

    const LADSPA_Data sample = instance_->ports[PORT_INPUT][i];
    instance_->ports[PORT_OUTPUT_LEFT][i] = sample / dl2;
    instance_->ports[PORT_OUTPUT_RIGHT][i] = sample / dr2;
  }
}

void cleanup(LADSPA_Handle instance)
{
  free(instance);
}

