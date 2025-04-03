/*
 * Plugin name: Delay
 *
 * Description: Digital delay effect
 */

#include <stdlib.h>
#include <string.h>

#include "ladspa.h"
#include "descriptors.h"
#include "utils.h"

enum {
  PORT_INPUT = 0,
  PORT_OUTPUT,
  PORT_DELAY,
  PORT_FEEDBACK,
  PORT_GAIN,
  PORT_WETDRYMIX,
  _PORT_COUNT,
};

static const LADSPA_PortDescriptor port_descriptors[_PORT_COUNT] = {
  [PORT_INPUT]        = LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
  [PORT_OUTPUT]       = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
  [PORT_DELAY]        = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_FEEDBACK]     = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_GAIN]         = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_WETDRYMIX]    = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
};

static const char *const port_names[_PORT_COUNT] = {
  [PORT_INPUT]        = "Input",
  [PORT_OUTPUT]       = "Output",
  [PORT_DELAY]        = "Delay",
  [PORT_FEEDBACK]     = "Feedback",
  [PORT_GAIN]         = "Gain",
  [PORT_WETDRYMIX]    = "Wet/dry mix",
};

static const LADSPA_PortRangeHint port_range_hints[_PORT_COUNT] = {
  [PORT_INPUT] = {0},
  [PORT_OUTPUT] = {0},
  [PORT_DELAY] = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_FEEDBACK] = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_GAIN] = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_WETDRYMIX] = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
};

struct instance {
  unsigned long    sample_rate;
  LADSPA_Data     *ports[_PORT_COUNT];
  LADSPA_Data     *buffer;
  unsigned long    buffer_size;
  unsigned long    cursor;
};

static LADSPA_Handle instantiate(const struct _LADSPA_Descriptor *descriptor, unsigned long sample_rate);
static void connect_port(LADSPA_Handle instance, unsigned long port, LADSPA_Data *data_location);
static void run(LADSPA_Handle instance, unsigned long sample_count);
static void cleanup(LADSPA_Handle instance);

const LADSPA_Descriptor delay_descriptor = {
  .UniqueID               = UID_DELAY,
  .Label                  = "audio",
  .Properties             = 0,
  .Name                   = "Delay",
  .Maker                  = MAKER,
  .Copyright              = COPYRIGHT,
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

static LADSPA_Handle instantiate(const struct _LADSPA_Descriptor *descriptor, unsigned long sample_rate)
{
  struct instance *const instance_ = calloc(1, sizeof(*instance_));
  if (instance_ == NULL)
    return NULL;

  const LADSPA_Data max_delay = descriptor->PortRangeHints[PORT_DELAY].UpperBound;
  instance_->buffer_size = 1 + (unsigned long) (max_delay * (LADSPA_Data) sample_rate);
  instance_->buffer = malloc(sizeof(*instance_->buffer) * instance_->buffer_size);
  if (instance_->buffer == NULL) {
    free(instance_);
    return NULL;
  }

  memset(instance_->buffer, 0, sizeof(*instance_->buffer) * instance_->buffer_size);

  instance_->sample_rate = sample_rate;
  instance_->cursor = 0;

  return (LADSPA_Handle) instance_;
}

static void connect_port(LADSPA_Handle instance, unsigned long port, LADSPA_Data *data_location)
{
  struct instance *const instance_ = (struct instance *) instance;
  instance_->ports[port] = data_location;
}

static void run(LADSPA_Handle instance, unsigned long sample_count)
{
  struct instance *const instance_ = (struct instance *) instance;

  const LADSPA_Data *const in = instance_->ports[PORT_INPUT];
  LADSPA_Data *const out      = instance_->ports[PORT_OUTPUT];
  const LADSPA_Data delay     = *instance_->ports[PORT_DELAY];
  const LADSPA_Data feedback  = *instance_->ports[PORT_FEEDBACK];
  const LADSPA_Data gain      = *instance_->ports[PORT_GAIN];
  const LADSPA_Data wetdrymix = *instance_->ports[PORT_WETDRYMIX];

  unsigned long offset = (unsigned long) (delay * (LADSPA_Data) instance_->sample_rate);

  for (unsigned long i = 0; i < sample_count; ++i) {

    unsigned long delay_cursor = instance_->cursor + instance_->buffer_size - offset;
    if (delay_cursor >= instance_->buffer_size)
      delay_cursor -= instance_->buffer_size;

    LADSPA_Data mix = instance_->buffer[delay_cursor];
    mix *= gain;
    mix = wetdrymix * in[i] + (1.f - wetdrymix) * mix;
    out[i] = mix;

    instance_->buffer[instance_->cursor++] = in[i] + feedback * out[i];
    if (instance_->cursor == instance_->buffer_size)
      instance_->cursor = 0;
  }
}

static void cleanup(LADSPA_Handle instance)
{
  struct instance *const instance_ = (struct instance *) instance;

  free(instance_->buffer);
  free(instance_);
}

