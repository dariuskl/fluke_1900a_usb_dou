// Data Output Unit Firmware / Darius Kellermann <kellermann@protonmail.com>

#include "msp430.hpp"

#include "dou.hpp"
#include "nostd.hpp"

#include <string_view>

namespace dou {

  constexpr auto smclk_frequency_Hz = 16'000'000;

  constexpr auto &tx_port_ = msp430::P1OUT;

  // port 1
  constexpr auto out_b_mask_ = u8{0x01}; // BCD 2
  constexpr auto as_1_mask_ = u8{0x02};  // LSD
  constexpr auto tx_mask_ = u8{0x04};
  constexpr auto rng_2_mask_ = u8{0x08};
  constexpr auto nml_mask_ = u8{0x10};
  constexpr auto ovfl_mask_ = u8{0x20};
  constexpr auto as_3_mask_ = u8{0x40}; // 4SD
  constexpr auto as_2_mask_ = u8{0x80}; // 5SD

  // port 2
  constexpr auto nmup_mask_ = u8{0x01};
  constexpr auto out_c_mask_ = u8{0x02}; // BCD 4
  constexpr auto out_d_mask_ = u8{0x04}; // BCD 8
  constexpr auto as_6_mask_ = u8{0x08};  // MSD
  constexpr auto as_5_mask_ = u8{0x10};  // 2SD
  constexpr auto as_4_mask_ = u8{0x20};  // 3SD
  constexpr auto out_a_mask_ = u8{0x40}; // BCD 1
  constexpr auto ds_mask_ = u8{0x80};

  Input_state decode_signals(const u8 port1, const u8 port2) {
    return {
        static_cast<i8>(
            ((port1 & as_1_mask_) != u8{0})
                ? 1
                : (((port1 & as_2_mask_) != u8{0})
                       ? 2
                       : (((port1 & as_3_mask_) != u8{0})
                              ? 3
                              : (((port2 & as_4_mask_) != u8{0})
                                     ? 4
                                     : (((port2 & as_5_mask_) != u8{0})
                                            ? 5
                                            : (((port2 & as_6_mask_) != u8{0})
                                                   ? 6
                                                   : 0)))))),
        static_cast<i8>((((port2 & out_a_mask_) != u8{0}) ? u8{1} : u8{0})
                        | (((port1 & out_b_mask_) != u8{0}) ? u8{2} : u8{0})
                        | (((port2 & out_c_mask_) != u8{0}) ? u8{4} : u8{0})
                        | (((port2 & out_d_mask_) != u8{0}) ? u8{8} : u8{0})),
        (port2 & ds_mask_) != u8{0},
        (port1 & ovfl_mask_) != u8{0},
        (port1 & nml_mask_) != u8{0},
        (port1 & rng_2_mask_) != u8{0}};
  }

  namespace {
    auto serial = Serial_transmitter{};
    auto decoder = Bus_decoder{};
  } // namespace

  void enable_nmup_interrupt() { store(msp430::P2IE, u8{nmup_mask_}); }
  void disable_nmup_interrupt() { store(msp430::P2IE, u8{0}); }

  [[noreturn]] void run() {
    // Clear P2SEL reasonably early, because excess current will flow from
    // the oscillator driver output at P2.7.
    store(msp430::P2SEL, u8{0});

    // The watchdog timer will reset the device, if no nMUP signal has been
    // detected after expiration of the longest gate time of 10 seconds.
    msp430::Watchdog_timer::hold(); // TODO set up watchdog

    store(msp430::BCSCTL1, load(msp430::CAL_BC1_16MHz));
    store(msp430::DCOCTL, load(msp430::CAL_DCO_16MHz));
    store(msp430::BCSCTL3, u8{0x24}); // ACLK=VLOCLK

    store(msp430::P1OUT, u8{0});
    store(msp430::P1DIR, tx_mask_);
    store(msp430::P1IES, u8{0});
    store(msp430::P1REN, u8{0});
    store(msp430::P2DIR, u8{0});
    store(msp430::P2IES, u8{nmup_mask_});
    store(msp430::P2IE, u8{nmup_mask_});
    store(msp430::P2REN, u8{0});

    auto uart_timer = msp430::Timer0_A3{msp430::Timer_A_clock_source::SMCLK, 1};
    uart_timer.enable_interrupt();

    msp430::enable_interrupts();

    msp430::go_to_sleep();

    while (true) {
      const auto port1 = load(msp430::P1IN);
      const auto port2 = load(msp430::P2IN);

      const auto update_memory = (port2 & nmup_mask_) == u8{0};

      const auto input_state = decode_signals(port1, port2);

      if (update_memory) {
        decoder.transit(input_state);
      } else {
        disable_nmup_interrupt();

        const auto buffer = std::string_view{decoder.reading()};

        uart_timer.start_up(u16{(smclk_frequency_Hz / serial_baud_rate) - 1});

        for (const auto& character : buffer) {
          serial.init_transmission(character);
          auto bit = u8{0};
          while (serial.get_next_bit(bit)) {
            // Wait for the first and next bit.
            msp430::go_to_sleep();
            store(tx_port_, (load(tx_port_) & ~tx_mask_)
                                | (bit != u8{0} ? u8{0} : tx_mask_));
          }
          // Wait for the last bit.
          msp430::go_to_sleep();
          store(tx_port_, load(tx_port_) & ~tx_mask_);
        }
        uart_timer.stop();
        decoder = {};

        // Wait for a falling edge on /MUP.
        enable_nmup_interrupt();
        msp430::go_to_sleep();
      }
    }
  }

  namespace {

    /// Called on falling edge on /MUP.
    [[gnu::interrupt]] void on_strobe() {
      store(msp430::P2IFG, u8{0});
      msp430::stay_awake();
    }

    [[gnu::interrupt]] void on_timer() { msp430::stay_awake(); }

    extern "C" [[noreturn, gnu::naked]] void on_reset() {
      // init stack pointer
      extern const uint16_t _stack;
      asm volatile("mov %0, SP" : : "ri"(&_stack));

      // .data
      extern uint16_t _sdata[];        // .data start
      extern uint16_t _edata;          // .data end
      extern const uint16_t _sidata[]; // .data init values
      const auto data_count = &_edata - &(_sdata[0]);
      for (auto i = 0; i < data_count; ++i) {
        _sdata[i] = _sidata[i];
      }

      // .bss
      extern uint16_t _sbss[]; // .bss start
      extern uint16_t _ebss;   // .bss end
      const auto bss_count = &_ebss - &(_sbss[0]);
      for (auto i = 0; i < bss_count; ++i) {
        _sbss[i] = 0U;
      }

      // .preinit_array
      extern void (*_preinit_array_start[])();
      extern void (*_preinit_array_end[])();
      const auto num_preinits = &(_preinit_array_end[0])
                                - &(_preinit_array_start[0]);
      for (auto i = 0; i < num_preinits; ++i) {
        _preinit_array_start[i]();
      }

      // .init_array
      extern void (*_init_array_start[])();
      extern void (*_init_array_end[])();
      const auto num_inits = &(_init_array_end[0]) - &(_init_array_start[0]);
      for (auto i = 0; i < num_inits; ++i) {
        _init_array_start[i]();
      }

      run();
    }

    [[gnu::interrupt]] void default_isr() {}

    constexpr auto vtable_
        [[gnu::used, gnu::section(".vectors")]] = Array<void (*)(), 32>{
            {nullptr,     nullptr,   nullptr,     nullptr,     nullptr, nullptr,
             nullptr,     nullptr,   nullptr,     nullptr,     nullptr, nullptr,
             nullptr,     nullptr,   nullptr,     nullptr,     nullptr, nullptr,
             on_strobe,   on_strobe, default_isr, default_isr, nullptr, nullptr,
             default_isr, on_timer,  default_isr, default_isr, nullptr, nullptr,
             default_isr, on_reset}};

  } // namespace

} // namespace dou

int main() { dou::run(); }
