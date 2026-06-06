#include <stdint.h>

static volatile uint32_t idle_counter;

int main(void) {
  while (1) {
    idle_counter++;
  }
}
