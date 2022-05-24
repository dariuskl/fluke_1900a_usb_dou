// Data Output Unit Firmware / Darius Kellermann <kellermann@protonmail.com>

#ifndef MSP430_HPP_
#define MSP430_HPP_

#include "nostd.hpp"

namespace msp430 {

  constexpr auto P1IN = Register<const u8>{0x20};
  constexpr auto P1OUT = Register<u8>{0x21};
  constexpr auto P1DIR = Register<u8>{0x22};
  constexpr auto P1IFG = Register<u8>{0x23};
  constexpr auto P1IES = Register<u8>{0x24};
  constexpr auto P1IE = Register<u8>{0x25};
  constexpr auto P1REN = Register<u8>{0x27};

  constexpr auto P2IN = Register<const u8>{0x28};
  constexpr auto P2OUT = Register<u8>{0x29};
  constexpr auto P2DIR = Register<u8>{0x2a};
  constexpr auto P2IFG = Register<u8>{0x2b};
  constexpr auto P2IES = Register<u8>{0x2c};
  constexpr auto P2IE = Register<u8>{0x2d};
  constexpr auto P2SEL = Register<u8>{0x2e};
  constexpr auto P2REN = Register<u8>{0x2f};

  constexpr auto BCSCTL3 = Register<u8>{0x53};
  constexpr auto DCOCTL = Register<u8>{0x56};
  constexpr auto BCSCTL1 = Register<u8>{0x57};

  constexpr auto CAL_BC1_16MHz = Register<u8>{0x10f6 + 0x0003};
  constexpr auto CAL_DCO_16MHz = Register<u8>{0x10f6 + 0x0002};

  [[gnu::always_inline]] inline void go_to_sleep() {
    asm volatile("nop { bis %0, SR { nop" : : "ri"(0x10));
  }

  [[gnu::always_inline]] inline void stay_awake() {
    __bic_SR_register_on_exit(0x10);
  }

  void enable_interrupts() { asm volatile("eint"); }
  void disable_interrupts() { asm volatile("dint { nop"); }

  enum class Watchdog_timer_clock_source { SMCLK, ACLK };
  enum class Watchdog_timer_interval { By32768, By8192, By512, By64 };

  class Watchdog_timer {
    public:
      static void configure(const Watchdog_timer_clock_source clock_source,
                            const Watchdog_timer_interval interval) {
        store(wdtctl_,
              wdt_unlock_ | load(wdtctl_b_)
                  | (clock_source == Watchdog_timer_clock_source::ACLK ? u8{4}
                                                                       : u8{0})
                  | static_cast<u8>(to_integral(interval)));
      }

      static void hold() { store(wdtctl_, wdt_unlock_ | wdt_hold_); }

      static void feed() {
        store(wdtctl_, wdt_unlock_ | load(wdtctl_b_) | wdt_count_clear_);
      }

    private:
      static constexpr auto wdt_unlock_ = u16{0x5a00};
      static constexpr auto wdt_hold_ = u16{0x0080};
      static constexpr auto wdt_count_clear_ = u16{0x0008};
      static constexpr auto wdtctl_ = Register<u16>{0x120};
      static constexpr auto wdtctl_b_ = Register<const u8>{0x120};
  };

  enum class Timer_A_clock_source { TACLK, ACLK, SMCLK, INCLK };

  template <intptr_t base_> class Timer_A_ {
    public:
      Timer_A_(Timer_A_clock_source clock_source, uint8_t clock_divider) {
        store(tactl_,
              (u16{static_cast<uint16_t>(clock_source)} << 8U)
                  | (u16{static_cast<uint16_t>(clock_divider - 1)} << 6U));
      }

      u16 count() const { return load(tar_); }

      void enable_interrupt() {
        store(tacctl0_, load(tacctl0_) | u16{1U << 4U});
      }

      void stop() { store(tactl_, load(tactl_) & ~(u16{3U} << 4U)); }

      void start_up(const u16 top) {
        store(taccr0_, top);
        store(tactl_, (load(tactl_) & ~(u16{3U} << 4U)) | (u16{1U} << 4U));
      }

      void start_continuous() {
        store(tactl_, (load(tactl_) & ~(u16{3U} << 4U)) | (u16{2U} << 4U));
      }

      void start_up_down(const u16 top) {
        store(taccr0_, top);
        store(tactl_, (load(tactl_) & ~(u16{3U} << 4U)) | (u16{3U} << 4U));
      }

    private:
      static constexpr auto tactl_ = Register<u16>{base_};
      static constexpr auto tacctl0_ = Register<u16>{base_ + 0x2};
      static constexpr auto tar_ = Register<const u16>{base_ + 0x10};
      static constexpr auto taccr0_ = Register<u16>{base_ + 0x12};
  };

  using Timer0_A3 = Timer_A_<0x160>;

} // namespace msp430

#endif // MSP430_HPP_