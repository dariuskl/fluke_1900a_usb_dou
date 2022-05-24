// Data Output Unit Firmware / Darius Kellermann <kellermann@protonmail.com>

#ifndef DOU_HPP_
#define DOU_HPP_

#include "nostd.hpp"
#include "util.hpp"

namespace dou {

  /// Keeps a capture of the decoded momentary bus state.
  struct Input_state {
      i8 digit_strobe;
      i8 out;
      bool decimal_strobe;
      bool overflow;
      bool nml;
      bool rng_2;
  };

  // The baud rate must be sufficient to transmit the whole measurement within
  // the /MUP period of approximately 100 ms.
  constexpr auto serial_baud_rate = 19200; // bps
  constexpr auto serial_data_bits = 7;

  class Serial_transmitter {
    public:
      void init_transmission(const char value) {
        transmit_data_ = (static_cast<u16>(value) << 1U)
                         | (u16{1} << (serial_data_bits + 1));
        bits_to_transmit_ = serial_data_bits + 2;
      }

      [[nodiscard]] bool transmit_complete() const {
        return bits_to_transmit_ <= 0;
      }

      [[nodiscard]] bool get_next_bit(u8 &bit_to_transmit) {
        if (not transmit_complete()) {
          bit_to_transmit = transmit_data_ & u8{1};

          transmit_data_ = transmit_data_ >> 1U;
          bits_to_transmit_ = static_cast<int_fast8_t>(bits_to_transmit_ - 1);

          return true;
        }
        return false;
      }

    private:
      u16 transmit_data_{};
      int_fast8_t bits_to_transmit_{0};
  };

  enum class Unit { ms, us, MHz, kHz, None };

  constexpr Unit get_unit(const bool nml, const bool rng_2,
                          const bool has_decimal_point) {
    if (not has_decimal_point) {
      return Unit::None;
    }
    const auto bcd = (rng_2 ? u8{2} : u8{0}) | (nml ? u8{1} : u8{0});
    return static_cast<Unit>(bcd);
  }

  int print(char *buffer, Size buffer_length, Unit unit);

  constexpr auto max_unit_length = 4;

  constexpr auto number_of_digits = 6;

  enum class Data_state {
    OverflowUnit = 0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Init
  };

  constexpr inline Data_state operator-(const Data_state lhs, const int rhs) {
    return static_cast<Data_state>(to_integral(lhs) - rhs);
  }

  /// Decodes the display bus of the Fluke 1900A.
  ///
  /// Initially, the FSM waits for the `AS_6` strobe, indicating the most
  /// significant digit (MSD). This ensures that decoding starts with the first
  /// complete block of digits (MSD..LSD) while /MUP is low.
  ///    For each strobe, the corresponding digit is captured and appended to
  /// the reading. If the decimal strobe is asserted during a digit strobe, the
  /// decimal point is prepended to the digit.
  ///    Once the least significant digit has been captured, the overflow status
  /// and range signals are evaluated and the reading is marked as complete.
  /// Only complete readings are returned. This prevents erroneous readings,
  /// which can occur due to glitches that appear on the bus when actuating
  /// front panel switches.
  ///    The decoder allows multiple passes (MSD..LSD) and always updates the
  /// reading with the digits from the latest pass.
  class Bus_decoder {
    public:
      Data_state state() const { return state_; }
      bool is_complete() const { return complete_; }
      bool has_decimal_point() const { return decimal_point_digit_ != 0; }

      const char *reading() {
        if (complete_) {
          return reading_.data();
        }
        return "";
      }

      void transit(const Input_state &inp) {
        switch (state_) {
        case Data_state::Init:
          if (inp.digit_strobe == to_integral(Data_state::Digit6)) {
            decimal_point_digit_ = 0;
            state_ = Data_state::Digit6;
          }
          break;

        case Data_state::Digit6:
        case Data_state::Digit5:
        case Data_state::Digit4:
        case Data_state::Digit3:
        case Data_state::Digit2:
        case Data_state::Digit1:
          if (inp.digit_strobe == to_integral(state_)) {
            if (inp.decimal_strobe) {
              decimal_point_digit_ = inp.digit_strobe;
            }
            set_digit(inp.digit_strobe, inp.out);
          } else if (inp.digit_strobe == to_integral(state_ - 1)) {
            state_ = state_ - 1;
          }
          break;

        case Data_state::OverflowUnit:
          set_overflow(inp.overflow);
          set_unit(get_unit(inp.nml, inp.rng_2, has_decimal_point()));
          complete_ = true;
          state_ = Data_state::Init;
          break;
        }
      }

    private:
      int get_index(const i8 digit_strobe) const {
        return number_of_digits + 1 + (has_decimal_point() ? 1 : 0)
               - digit_strobe;
      }

      void set_overflow(const bool overflow) {
        if (overflow) {
          reading_[0] = '>';
        } else {
          reading_[0] = ' ';
        }
      }

      void set_digit(const i8 digit_strobe, const i8 out) {
        if (digit_strobe == decimal_point_digit_) {
          reading_[number_of_digits + 1 - digit_strobe] = '.';
        }
        reading_[get_index(digit_strobe)] = static_cast<char>('0' + out);
      }

      void set_unit(const Unit unit) {
        const auto index = get_index(0);
        print(&(reading_[index]), reading_.size() - index, unit, "\r\n");
      }

      static constexpr auto max_reading_size = number_of_digits
                                               + 1 // overflow indicator
                                               + 1 // decimal point
                                               + max_unit_length
                                               + 2 // line ending
                                               + 1 // terminating zero
          ;

      Data_state state_{Data_state::Digit6};
      Array<char, max_reading_size> reading_{""};
      i8 decimal_point_digit_{0};
      bool complete_{false};
  };

} // namespace dou

#endif // DOU_HPP_
