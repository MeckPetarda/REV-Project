#include "../config.h"

#include "../menu.h"
#include "../per/buttons.h"
#include "../per/lcd.h"
#include "../per/led.h"
#include <stdbool.h>
#include <stdio.h>

static void hp_interrupt() {}

static void lp_interrupt() {}

static void init() {}

static void destructor(void) {}

static void main(void) {}

void register_template(void) {
  registerProgram("TEMPLATE", &init, &destructor, &main, &lp_interrupt, &hp_interrupt);
}
