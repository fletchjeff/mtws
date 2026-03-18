#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "pico/time.h"

#include "knots/solo_engines/floatable/floatable_solo_engine.h"
#include "knots/solo_engines/solo_common/solo_app_base.h"

int main() {
  vreg_set_voltage(VREG_VOLTAGE_1_15);
  sleep_ms(10);
  set_sys_clock_khz(200000, true);

  // Keep the double-buffered render frames out of the main stack.
  static solo_common::SoloAppBase<FloatableSoloEngine> app;
  app.Run();
}
