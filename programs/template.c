#include "../config.h"

#include "../buttons.h"
#include "../lcd.h"
#include "../led.h"
#include "../menu.h"
#include <stdbool.h>
#include <stdio.h>

static void hp_interrupt() {}

static void init() {}

static void destructor(void) {}

static void main(void) {}

void register_template(void) {
  registerProgram("TEMPLATE", &init, &destructor, &main, NULL, &interrupt);
}
