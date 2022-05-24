// Data Output Unit Firmware / Darius Kellermann <kellermann@protonmail.com>

#include "dou.hpp"

namespace dou {

  namespace {

    constexpr auto unit_texts_ = Array<Array<char, max_unit_length>, 5>{
        {{"ms"}, {"us"}, {"MHz"}, {"kHz"}, {""}}};

  }

  int print(char *const buffer, const Size buffer_length, const Unit unit) {
    return print(buffer, buffer_length, unit_texts_[to_integral(unit)]);
  }

} // namespace dou
