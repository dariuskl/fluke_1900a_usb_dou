// Data Output Unit Firmware / Darius Kellermann <kellermann@protonmail.com>

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include "nostd.hpp"

namespace dou {

  inline int print(char *const buffer, Size const buffer_length,
                   const char *const string) {
    auto i = 0;
    for (; i < buffer_length; ++i) {
      if (i < (buffer_length - 1)) {
        buffer[i] = string[i];
      } else {
        buffer[i] = '\0';
      }
      if (string[i] == '\0') {
        return i;
      }
    }
    return i;
  }

  template <Size string_length_>
  inline int print(char *const buffer, Size const buffer_length,
                   Array<char, string_length_> const &string) {
    return print(buffer, buffer_length, string.data());
  }

  /// \return The number of characters actually written.
  template <typename Head_type_, typename... Tail_types_>
  inline int print(char *const buffer, const Size buffer_length,
                   const Head_type_ &head, const Tail_types_ &...tail) {
    static_assert(sizeof...(Tail_types_) > 0,
                  "there is no implementation of print for `head`");

    auto const used_characters = print(buffer, buffer_length, head);

    if ((used_characters < 0) or (used_characters >= buffer_length)) {
      return -1;
    }

    return print(buffer + used_characters, buffer_length - used_characters,
                 tail...)
           + used_characters;
  }

  template <Size buffer_length_, typename... Args_>
  inline int print(Array<char, buffer_length_> &buffer, const Args_ &...args) {
    return print(buffer.data(), buffer_length_, args...);
  }

} // namespace dou

#endif // UTIL_HPP_
