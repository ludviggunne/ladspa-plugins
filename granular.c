/*
 * Plugin name: Granular
 *
 * Description: Tapped delay line granular synthesis
 */

#include <stdlib.h>
#include <string.h>

#include "ladspa.h"
#include "descriptors.h"
#include "utils.h"

enum {
  PORT_INPUT_LEFT = 0,
  PORT_INPUT_RIGHT,
  PORT_OUTPUT_LEFT,
  PORT_OUTPUT_RIGHT,
  PORT_MIN_DELAY,
  PORT_MAX_DELAY,
  PORT_MIN_LENGTH,
  PORT_MAX_LENGTH,
  PORT_MIN_COOLDOWN,
  PORT_MAX_COOLDOWN,
  PORT_MIN_GAIN,
  PORT_MAX_GAIN,
  PORT_SLOTS,
  PORT_MASTER_GAIN,
  _PORT_COUNT,
};

static const LADSPA_PortDescriptor port_descriptors[_PORT_COUNT] = {
  [PORT_INPUT_LEFT]           = LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
  [PORT_INPUT_RIGHT]          = LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
  [PORT_OUTPUT_LEFT]          = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
  [PORT_OUTPUT_RIGHT]         = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
  [PORT_MIN_DELAY]            = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_MAX_DELAY]            = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_MIN_LENGTH]           = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_MAX_LENGTH]           = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_MIN_COOLDOWN]         = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_MAX_COOLDOWN]         = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_MIN_GAIN]             = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_MAX_GAIN]             = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_SLOTS]                = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
  [PORT_MASTER_GAIN]          = LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
};

static const char *const port_names[_PORT_COUNT] = {
  [PORT_INPUT_LEFT]         = "Left input",
  [PORT_INPUT_RIGHT]        = "Right input",
  [PORT_OUTPUT_LEFT]        = "Left output",
  [PORT_OUTPUT_RIGHT]       = "Right output",
  [PORT_MIN_DELAY]          = "Min. delay",
  [PORT_MAX_DELAY]          = "Max. delay",
  [PORT_MIN_LENGTH]         = "Min. length",
  [PORT_MAX_LENGTH]         = "Max. length",
  [PORT_MIN_COOLDOWN]       = "Min. cooldown",
  [PORT_MAX_COOLDOWN]       = "Max. cooldown",
  [PORT_MIN_GAIN]           = "Min. gain",
  [PORT_MAX_GAIN]           = "Max. gain",
  [PORT_SLOTS]              = "Slots",
  [PORT_MASTER_GAIN]        = "Master gain",
};

static const LADSPA_PortRangeHint port_range_hints[_PORT_COUNT] = {
  [PORT_INPUT_LEFT]   = {0},
  [PORT_INPUT_RIGHT]  = {0},
  [PORT_OUTPUT_LEFT]  = {0},
  [PORT_OUTPUT_RIGHT] = {0},
  [PORT_MIN_DELAY]    = {
    .HintDescriptor =
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_DEFAULT_LOW |
      LADSPA_HINT_LOGARITHMIC,
    .LowerBound = .001f,
    .UpperBound = 2.f,
  },
  [PORT_MAX_DELAY]    = {
    .HintDescriptor =
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_DEFAULT_HIGH |
      LADSPA_HINT_LOGARITHMIC,
    .LowerBound = .001f,
    .UpperBound = 2.f,
  },
  [PORT_MIN_LENGTH]   = {
    .HintDescriptor =
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_DEFAULT_LOW |
      LADSPA_HINT_LOGARITHMIC,
    .LowerBound = .001f,
    .UpperBound = 2.f,
  },
  [PORT_MAX_LENGTH]   = {
    .HintDescriptor =
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_DEFAULT_HIGH |
      LADSPA_HINT_LOGARITHMIC,
    .LowerBound = .001f,
    .UpperBound = 2.f,
  },
  [PORT_MIN_COOLDOWN] = {
    .HintDescriptor =
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_DEFAULT_LOW |
      LADSPA_HINT_LOGARITHMIC,
    .LowerBound = .001f,
    .UpperBound = 2.f,
  },
  [PORT_MAX_COOLDOWN] = {
    .HintDescriptor =
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_DEFAULT_HIGH |
      LADSPA_HINT_LOGARITHMIC,
    .LowerBound = .001f,
    .UpperBound = 2.f,
  },
  [PORT_MIN_GAIN]     = {
    .HintDescriptor =
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_DEFAULT_LOW |
      LADSPA_HINT_LOGARITHMIC,
    .LowerBound = .0f,
    .UpperBound = 1.f,
  },
  [PORT_MAX_GAIN]     = {
    .HintDescriptor =
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_DEFAULT_HIGH |
      LADSPA_HINT_LOGARITHMIC,
    .LowerBound = .0f,
    .UpperBound = 1.f,
  },
  [PORT_SLOTS]        = {
    .HintDescriptor =
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER |
      LADSPA_HINT_DEFAULT_MINIMUM,
    .LowerBound = 1.f,
    .UpperBound = 64.f,
  },
  [PORT_MASTER_GAIN]  = {
    .HintDescriptor =
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_DEFAULT_LOW,
    .LowerBound = .0f,
    .UpperBound = 1.f,
  },
};

struct slot {
  LADSPA_Data      pan;
  LADSPA_Data      gain;
  unsigned long    length;
  unsigned long    offset;
  unsigned long    cursor;
  unsigned long    cooldown;
};

struct instance {
  unsigned long    sample_rate;
  unsigned long    buffer_size;
  unsigned long    num_slots;
  unsigned long    cursor;

  LADSPA_Data     *ports[_PORT_COUNT];
  LADSPA_Data     *buffer;

  struct slot     *slots;
};

static LADSPA_Handle instantiate(const struct _LADSPA_Descriptor *descriptor, unsigned long sample_rate);
static void connect_port(LADSPA_Handle instance, unsigned long port, LADSPA_Data *data_location);
static void run(LADSPA_Handle instance, unsigned long sample_count);
static void cleanup(LADSPA_Handle instance);

const LADSPA_Descriptor granular_descriptor = {
  .UniqueID               = UID_GRANULAR,
  .Label                  = "audio",
  .Properties             = 0,
  .Name                   = "Granular",
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
    goto failure;

  instance_->sample_rate = sample_rate;

  const LADSPA_Data max_delay = descriptor->PortRangeHints[PORT_MAX_DELAY].UpperBound;
  instance_->buffer_size = 1 + (unsigned long) (max_delay * (LADSPA_Data) sample_rate);

  instance_->buffer = calloc(sizeof(*instance_->buffer), instance_->buffer_size);
  if (instance_->buffer == NULL)
    goto failure;

  const unsigned long max_slots = (unsigned long) descriptor->PortRangeHints[PORT_SLOTS].UpperBound;
  instance_->slots = calloc(sizeof(*instance_->slots), max_slots);
  if (instance_->slots == NULL)
    goto failure;

  return (LADSPA_Handle) instance_;

failure:
  free(instance_->slots);
  free(instance_->buffer);
  free(instance_);
  return NULL;
}

static void connect_port(LADSPA_Handle instance, unsigned long port, LADSPA_Data *data_location)
{
  struct instance *const instance_ = (struct instance *) instance;
  instance_->ports[port] = data_location;
}

static void run(LADSPA_Handle instance, unsigned long sample_count)
{
  struct instance *const instance_ = (struct instance *) instance;

  /* Read the ports */
  const unsigned long num_slots    = (unsigned long) *instance_->ports[PORT_SLOTS];
  const LADSPA_Data *const l_in    = instance_->ports[PORT_INPUT_LEFT];
  const LADSPA_Data *const r_in    = instance_->ports[PORT_INPUT_RIGHT];
  LADSPA_Data *const  l_out        = instance_->ports[PORT_OUTPUT_LEFT];
  LADSPA_Data *const  r_out        = instance_->ports[PORT_OUTPUT_RIGHT];
  const unsigned long min_delay    = (unsigned long) (*instance_->ports[PORT_MIN_DELAY] * (LADSPA_Data) instance_->sample_rate);
  unsigned long max_delay          = (unsigned long) (*instance_->ports[PORT_MAX_DELAY] * (LADSPA_Data) instance_->sample_rate);
  const unsigned long min_length   = (unsigned long) (*instance_->ports[PORT_MIN_LENGTH] * (LADSPA_Data) instance_->sample_rate);
  unsigned long max_length         = (unsigned long) (*instance_->ports[PORT_MAX_LENGTH] * (LADSPA_Data) instance_->sample_rate);
  const unsigned long min_cooldown = (unsigned long) (*instance_->ports[PORT_MIN_COOLDOWN] * (LADSPA_Data) instance_->sample_rate);
  unsigned long max_cooldown       = (unsigned long) (*instance_->ports[PORT_MAX_COOLDOWN] * (LADSPA_Data) instance_->sample_rate);
  const LADSPA_Data min_gain       = *instance_->ports[PORT_MIN_GAIN];
  LADSPA_Data max_gain             = *instance_->ports[PORT_MAX_GAIN];
  const LADSPA_Data master_gain    = *instance_->ports[PORT_MASTER_GAIN];

  if (max_delay < min_delay + 1)
    max_delay = min_delay + 1;

  if (max_length < min_length + 1)
    max_length = min_length + 1;

  if (max_cooldown < min_cooldown + 1)
    max_cooldown = min_cooldown + 1;

  if (max_gain < min_gain + 0.001f)
    max_gain = min_gain + 0.001f;

  /* Check if new slots need to be initialized */
  if (num_slots > instance_->num_slots) {
    for (unsigned long i = instance_->num_slots; i < num_slots; ++i) {

      /* Start in cooldown mode so they don't all start playing
       * at the same time */
      struct slot *const slot = &instance_->slots[i];
      slot->offset   = min_delay + rand() % (max_delay - min_delay);
      slot->length   = min_length + rand() % (max_length - min_length);
      slot->gain     = min_gain + ((LADSPA_Data) rand() / (LADSPA_Data) RAND_MAX) * (max_gain - min_gain);
      slot->cooldown = min_cooldown + rand() % (max_cooldown - min_cooldown);
      slot->pan      = (LADSPA_Data) rand() / (LADSPA_Data) RAND_MAX;
      slot->cursor   = 0;
    }
  }

  for (unsigned long i = 0; i < sample_count; ++i) {
    LADSPA_Data l_accumulator = 0.f;
    LADSPA_Data r_accumulator = 0.f;

    instance_->buffer[instance_->cursor++] = .5f * (l_in[i] + r_in[i]);
    if (instance_->cursor == instance_->buffer_size)
      instance_->cursor = 0;

    for (unsigned long j = 0; j < num_slots; ++j) {
      struct slot *const slot = &instance_->slots[j];

      if (slot->cooldown) {
        --slot->cooldown;
        continue;
      }

      if (slot->cursor == slot->length) {
        slot->offset   = min_delay + rand() % (max_delay - min_delay);
        slot->length   = min_length + rand() % (max_length - min_length);
        slot->gain     = min_gain + ((LADSPA_Data) rand() / (LADSPA_Data) RAND_MAX) * (max_gain - min_gain);
        slot->cooldown = min_cooldown + rand() % (max_cooldown - min_cooldown);
        slot->pan      = (LADSPA_Data) rand() / (LADSPA_Data) RAND_MAX;
        slot->cursor   = 0;
        continue;
      }

      unsigned long sample_index = instance_->cursor + instance_->buffer_size - slot->offset;
      if (sample_index >= instance_->buffer_size)
        sample_index -= instance_->buffer_size;

      const LADSPA_Data t = (LADSPA_Data) slot->cursor / (LADSPA_Data) slot->length;
      const LADSPA_Data env = 1.f - (2.f * t - 1.f) * (2.f * t - 1.f);

      LADSPA_Data sample = instance_->buffer[sample_index];
      sample *= env * slot->gain;
      l_accumulator += slot->pan * sample;
      r_accumulator += (1.f - slot->pan) * sample;

      ++slot->cursor;
    }

    l_out[i] = l_accumulator * master_gain;
    r_out[i] = r_accumulator * master_gain;
  }

  instance_->num_slots = num_slots;
}

static void cleanup(LADSPA_Handle instance)
{
  struct instance *const instance_ = (struct instance *) instance;

  free(instance_->slots);
  free(instance_->buffer);
  free(instance_);
}

