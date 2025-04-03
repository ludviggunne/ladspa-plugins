/*
 * Plugin name: Oribtal delay
 *
 * Description: Delay effect with some cool panning
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ladspa.h"
#include "descriptors.h"
#include "utils.h"

enum {
  PORT_INPUT_LEFT = 0,
  PORT_OUTPUT_LEFT,
  PORT_DELAY_LEFT,
  PORT_FEEDBACK_LEFT,
  PORT_GAIN_LEFT,
  PORT_WETDRYMIX_LEFT,
  PORT_CUTOFF_LEFT,
  PORT_INPUT_RIGHT,
  PORT_OUTPUT_RIGHT,
  PORT_DELAY_RIGHT,
  PORT_FEEDBACK_RIGHT,
  PORT_GAIN_RIGHT,
  PORT_WETDRYMIX_RIGHT,
  PORT_CUTOFF_RIGHT,
  PORT_ORBITAL,
  _PORT_COUNT,
};

static const LADSPA_PortDescriptor port_descriptors[_PORT_COUNT] = {
  [PORT_INPUT_LEFT]         = LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
  [PORT_OUTPUT_LEFT]        = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
  [PORT_DELAY_LEFT]         = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_FEEDBACK_LEFT]      = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_GAIN_LEFT]          = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_WETDRYMIX_LEFT]     = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_CUTOFF_LEFT]        = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_INPUT_RIGHT]        = LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
  [PORT_OUTPUT_RIGHT]       = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
  [PORT_DELAY_RIGHT]        = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_FEEDBACK_RIGHT]     = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_GAIN_RIGHT]         = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_WETDRYMIX_RIGHT]    = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_CUTOFF_RIGHT]       = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_ORBITAL]            = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
};

static const char *const port_names[_PORT_COUNT] = {
  [PORT_INPUT_LEFT]         = "Left input",
  [PORT_OUTPUT_LEFT]        = "Left output",
  [PORT_DELAY_LEFT]         = "Left delay",
  [PORT_FEEDBACK_LEFT]      = "Left feedback",
  [PORT_GAIN_LEFT]          = "Left gain",
  [PORT_WETDRYMIX_LEFT]     = "Left wet/dry mix",
  [PORT_CUTOFF_LEFT]        = "Left cutoff",
  [PORT_INPUT_RIGHT]        = "Right input",
  [PORT_OUTPUT_RIGHT]       = "Right output",
  [PORT_DELAY_RIGHT]        = "Right delay",
  [PORT_FEEDBACK_RIGHT]     = "Right feedback",
  [PORT_GAIN_RIGHT]         = "Right gain",
  [PORT_WETDRYMIX_RIGHT]    = "Right wet/dry mix",
  [PORT_CUTOFF_RIGHT]       = "Right cutoff",
  [PORT_ORBITAL]            = "Orbital",
};

static const LADSPA_PortRangeHint port_range_hints[_PORT_COUNT] = {
  [PORT_INPUT_LEFT]      = {0},
  [PORT_OUTPUT_LEFT]     = {0},
  [PORT_DELAY_LEFT]      = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_FEEDBACK_LEFT]   = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_GAIN_LEFT]       = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_WETDRYMIX_LEFT]  = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_CUTOFF_LEFT]     = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_INPUT_RIGHT]     = {0},
  [PORT_OUTPUT_RIGHT]    = {0},
  [PORT_DELAY_RIGHT]     = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_FEEDBACK_RIGHT]  = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_GAIN_RIGHT]      = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_WETDRYMIX_RIGHT] = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_CUTOFF_RIGHT]    = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 1.f, 0),
  [PORT_ORBITAL]         = PORT_RANGE_HINTS_BOUNDED_FLOAT(0.f, 10.f, 0),
};

struct instance {
  unsigned long    sample_rate;
  LADSPA_Data     *ports[_PORT_COUNT];
  LADSPA_Data     *left_buffer;
  LADSPA_Data     *right_buffer;
  unsigned long    buffer_size;
  unsigned long    cursor;
  unsigned long    counter;
};

static LADSPA_Handle instantiate(const struct _LADSPA_Descriptor *descriptor, unsigned long sample_rate);
static void connect_port(LADSPA_Handle instance, unsigned long port, LADSPA_Data *data_location);
static void run(LADSPA_Handle instance, unsigned long sample_count);
static void cleanup(LADSPA_Handle instance);

const LADSPA_Descriptor orbital_delay_descriptor = {
  .UniqueID               = UID_ORBITAL_DELAY,
  .Label                  = "audio",
  .Properties             = 0,
  .Name                   = "Orbital delay",
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

  const LADSPA_Data max_delay = descriptor->PortRangeHints[PORT_DELAY_LEFT].UpperBound;
  instance_->buffer_size = 1 + (unsigned long) (max_delay * (LADSPA_Data) sample_rate);

  instance_->left_buffer = malloc(sizeof(*instance_->left_buffer) * instance_->buffer_size);
  if (instance_->left_buffer == NULL) {
    free(instance_);
    return NULL;
  }

  instance_->right_buffer = malloc(sizeof(*instance_->right_buffer) * instance_->buffer_size);
  if (instance_->right_buffer == NULL) {
    free(instance_->left_buffer);
    free(instance_);
    return NULL;
  }

  memset(instance_->left_buffer, 0, sizeof(*instance_->left_buffer) * instance_->buffer_size);
  memset(instance_->right_buffer, 0, sizeof(*instance_->right_buffer) * instance_->buffer_size);

  instance_->sample_rate = sample_rate;
  instance_->cursor = 0;
  instance_->counter = 0;

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

  const LADSPA_Data *l_in       = instance_->ports[PORT_INPUT_LEFT];
  const LADSPA_Data *r_in       = instance_->ports[PORT_INPUT_RIGHT];
  LADSPA_Data *l_out            = instance_->ports[PORT_OUTPUT_LEFT];
  LADSPA_Data *r_out            = instance_->ports[PORT_OUTPUT_RIGHT];
  const LADSPA_Data l_delay     = *instance_->ports[PORT_DELAY_LEFT];
  const LADSPA_Data r_delay     = *instance_->ports[PORT_DELAY_RIGHT];
  const LADSPA_Data l_feedback  = *instance_->ports[PORT_FEEDBACK_LEFT];
  const LADSPA_Data r_feedback  = *instance_->ports[PORT_FEEDBACK_RIGHT];
  const LADSPA_Data l_gain      = *instance_->ports[PORT_GAIN_LEFT];
  const LADSPA_Data r_gain      = *instance_->ports[PORT_GAIN_RIGHT];
  const LADSPA_Data l_wetdrymix = *instance_->ports[PORT_WETDRYMIX_LEFT];
  const LADSPA_Data r_wetdrymix = *instance_->ports[PORT_WETDRYMIX_RIGHT];
  const LADSPA_Data l_cutoff    = *instance_->ports[PORT_CUTOFF_LEFT];
  const LADSPA_Data r_cutoff    = *instance_->ports[PORT_CUTOFF_RIGHT];

  const LADSPA_Data orbital     = *instance_->ports[PORT_ORBITAL] *
                                  (LADSPA_Data) instance_->sample_rate;

  unsigned long l_offset = (unsigned long) (l_delay * (LADSPA_Data) instance_->sample_rate);
  unsigned long r_offset = (unsigned long) (r_delay * (LADSPA_Data) instance_->sample_rate);

  for (unsigned long i = 0; i < sample_count; ++i) {

    unsigned long l_delay_cursor = instance_->cursor + instance_->buffer_size - l_offset;
    if (l_delay_cursor >= instance_->buffer_size)
      l_delay_cursor -= instance_->buffer_size;

    unsigned long r_delay_cursor = instance_->cursor + instance_->buffer_size - r_offset;
    if (r_delay_cursor >= instance_->buffer_size)
      r_delay_cursor -= instance_->buffer_size;

    LADSPA_Data l_mix = instance_->left_buffer[l_delay_cursor];
    l_mix *= l_gain;
    l_mix = l_wetdrymix * l_in[i] + (1.f - l_wetdrymix) * l_mix;
    l_out[i] = l_mix;

    LADSPA_Data r_mix = instance_->right_buffer[r_delay_cursor];
    r_mix *= r_gain;
    r_mix = r_wetdrymix * r_in[i] + (1.f - r_wetdrymix) * r_mix;
    r_out[i] = r_mix;

    LADSPA_Data pan = -1.f + 2.f * sin(2.f * PI * (LADSPA_Data) instance_->counter / orbital);

    if (instance_->counter++ >= (unsigned long) orbital)
      instance_->counter = 0;

    // LADSPA_Data l_writeback = pan * l_out[i] + (1.f - pan) * r_out[i];
    // LADSPA_Data r_writeback = pan * r_out[i] + (1.f - pan) * l_out[i];

    LADSPA_Data l_writeback = l_in[i] + l_feedback * l_out[i];
    LADSPA_Data r_writeback = r_in[i] + r_feedback * r_out[i];

    l_writeback = pan * l_writeback + (1.f - pan) * r_writeback;
    r_writeback = pan * r_writeback + (1.f - pan) * l_writeback;

    unsigned long cutoff_cursor = instance_->cursor > 0 ?
                                  instance_->cursor - 1 :
                                  instance_->buffer_size - 1;

    instance_->left_buffer[instance_->cursor] =
      (l_cutoff) * l_writeback +
      (1.f - l_cutoff) * instance_->left_buffer[cutoff_cursor];

    instance_->right_buffer[instance_->cursor] =
      (r_cutoff) * r_writeback +
      (1.f - r_cutoff) * instance_->left_buffer[cutoff_cursor];

    if (instance_->cursor++ == instance_->buffer_size)
      instance_->cursor = 0;
  }
}

static void cleanup(LADSPA_Handle instance)
{
  struct instance *const instance_ = (struct instance *) instance;

  free(instance_->left_buffer);
  free(instance_->right_buffer);
  free(instance_);
}

