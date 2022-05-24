// Data Output Unit Firmware / Darius Kellermann <kellermann@protonmail.com>

#include "dou.hpp"

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <string_view>

using str = std::string_view;

namespace std {

  template <typename Tp1_, typename Tp2_>
  std::ostream &operator<<(std::ostream &ost,
                           const std::pair<Tp1_, Tp2_> &rhs) {
    return ost << "{ " << static_cast<int>(rhs.first) << ", " << rhs.second
               << " }";
  }

} // namespace std

namespace dou {

  SCENARIO("decoding of units", "[dou]") {
    GIVEN("there is no decimal point") {
      const auto ds_ = false;

      THEN("it's always counter mode") {
        const auto nml = GENERATE(false, true);
        const auto rng_2 = GENERATE(false, true);
        CHECK(get_unit(nml, rng_2, ds_) == Unit::None);
      }
    }

    GIVEN("there is a decimal point") {
      const auto ds_ = true;

      WHEN("period switch is depressed") {
        const auto rng2 = false;

        THEN("period is measured") {
          WHEN("longest gate time selected") {
            const auto nml = true;

            THEN("the unit is microseconds") {
              CHECK(get_unit(nml, rng2, ds_) == Unit::us);
            }
          }
          WHEN("any other gate time selected") {
            const auto nml = false;

            THEN("the unit is milliseconds") {
              CHECK(get_unit(nml, rng2, ds_) == Unit::ms);
            }
          }
        }
      }

      WHEN("period switch is not depressed") {
        const auto rng2 = true;

        THEN("frequency is measured") {
          WHEN("longest gate time selected") {
            const auto nml = true;

            THEN("the unit is kilohertz") {
              CHECK(get_unit(nml, rng2, ds_) == Unit::kHz);
            }
          }
          WHEN("any other gate time selected") {
            const auto nml = false;

            THEN("the unit is megahertz") {
              CHECK(get_unit(nml, rng2, ds_) == Unit::MHz);
            }
          }
        }
      }
    }
  }

  SCENARIO("printing of units", "[dou]") {
    auto buffer = Array<char, 4>{};

    CHECK(print(buffer, Unit::ms) == 2);
    CHECK(str{buffer.data()} == "ms");

    CHECK(print(buffer, Unit::us) == 2);
    CHECK(str{buffer.data()} == "us");

    CHECK(print(buffer, Unit::MHz) == 3);
    CHECK(str{buffer.data()} == "MHz");

    CHECK(print(buffer, Unit::kHz) == 3);
    CHECK(str{buffer.data()} == "kHz");

    CHECK(print(buffer, Unit::None) == 0);
    CHECK(str{buffer.data()}.empty());
  }

  std::string_view get_display_for_(Bus_decoder &decoder,
                                    const std::string_view display_string) {
    auto bus = Input_state{0,
                           0,
                           false,
                           false,
                           (display_string[7] == 'u')
                               or (display_string[7] == 'k'),
                           display_string[8] == 'H'};
    decoder.transit(bus);

    auto digit_counter = number_of_digits + 1;

    for (const auto character : display_string) {
      if (character == '.') {
        bus.decimal_strobe = true;
        continue;
      }

      --digit_counter;

      if (digit_counter > 0) {
        bus.out = static_cast<i8>(character - '0');
      }

      bus.digit_strobe = digit_counter;

      decoder.transit(bus);
      decoder.transit(bus);

      bus.decimal_strobe = false;
    }

    bus.digit_strobe = 0;

    decoder.transit(bus);
    decoder.transit(bus);

    return decoder.reading();
  }

  SCENARIO("decoding the display bus", "[app]") {
    auto uut = Bus_decoder{};
    WHEN("falling edge on nMUP") {
      const auto overflow = GENERATE(false, true);

      AND_WHEN("digit 6 is strobed") {
        uut.transit({6, 1, false, overflow, false, false});
        uut.transit({6, 1, false, overflow, false, false});

        CHECK(uut.state() == Data_state::Digit6);

        AND_WHEN("digit 5 is strobed") {
          uut.transit({5, 2, false, overflow, false, false});
          uut.transit({5, 2, false, overflow, false, false});

          CHECK(uut.state() == Data_state::Digit5);

          AND_WHEN("digit 4 is strobed") {
            uut.transit({4, 3, false, overflow, false, false});
            uut.transit({4, 3, false, overflow, false, false});

            CHECK(uut.state() == Data_state::Digit4);

            AND_WHEN("digit 3 is strobed") {
              auto strobes = GENERATE(/* digit, DS */
                                      Array<std::pair<i8, bool>, 3>{
                                          // DS inside
                                          {{3, false}, {3, true}, {3, false}}},
                                      Array<std::pair<i8, bool>, 3>{
                                          // DS early
                                          {{0, true}, {3, true}, {3, false}}},
                                      Array<std::pair<i8, bool>, 3>{
                                          // DS late
                                          {{3, false}, {3, true}, {0, true}}});

              CAPTURE(strobes);

              uut.transit({strobes[0].first, 4, strobes[0].second, overflow,
                           false, false});
              uut.transit({strobes[0].first, 4, strobes[0].second, overflow,
                           false, false});
              uut.transit({strobes[1].first, 4, strobes[1].second, overflow,
                           false, false});
              uut.transit({strobes[1].first, 4, strobes[1].second, overflow,
                           false, false});
              uut.transit({strobes[2].first, 4, strobes[2].second, overflow,
                           false, false});
              uut.transit({strobes[2].first, 4, strobes[2].second, overflow,
                           false, false});

              CHECK(uut.state() == Data_state::Digit3);

              AND_WHEN("digit 2 is strobed") {
                uut.transit({2, 5, false, overflow, false, false});
                uut.transit({2, 5, false, overflow, false, false});

                CHECK(uut.state() == Data_state::Digit2);

                AND_WHEN("digit 1 is strobed") {
                  uut.transit({1, 6, false, overflow, false, false});
                  uut.transit({1, 6, false, overflow, false, false});

                  CHECK(uut.state() == Data_state::Digit1);

                  uut.transit({0, 6, false, overflow, false, false});

                  CHECK(uut.state() == Data_state::OverflowUnit);

                  uut.transit({0, 6, false, overflow, false, false});

                  CHECK(uut.state() == Data_state::Init);

                  REQUIRE(uut.is_complete());

                  THEN("the reading is updated with digit 1, the overflow "
                       "indicator, and the unit") {
                    const auto reading = std::string_view{uut.reading()};
                    if (overflow) {
                      CHECK(reading == ">123.456ms\r\n");
                    } else {
                      CHECK(reading == " 123.456ms\r\n");
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    GIVEN("device is in counter mode") {
      CHECK(get_display_for_(uut, "123456") == " 123456\r\n");
    }

    GIVEN("device is in period mode") {
      WHEN("range is ms") {
        WHEN("decimal point after two digits") {
          CHECK(get_display_for_(uut, "12.3456ms") == " 12.3456ms\r\n");
        }

        WHEN("decimal point after three digits") {
          CHECK(get_display_for_(uut, "123.456ms") == " 123.456ms\r\n");
        }

        WHEN("decimal point after four digits") {
          CHECK(get_display_for_(uut, "1234.56ms") == " 1234.56ms\r\n");
        }
      }

      WHEN("range is µs") {
        WHEN("decimal point after two digits") {
          CHECK(get_display_for_(uut, "12.3456us") == " 12.3456us\r\n");
        }

        WHEN("decimal point after three digits") {
          CHECK(get_display_for_(uut, "123.456us") == " 123.456us\r\n");
        }

        WHEN("decimal point after four digits") {
          CHECK(get_display_for_(uut, "1234.56us") == " 1234.56us\r\n");
        }
      }
    }

    GIVEN("device is in frequency mode") {
      WHEN("range is MHz") {
        WHEN("decimal point after two digits") {
          CHECK(get_display_for_(uut, "12.3456MHz") == " 12.3456MHz\r\n");
        }

        WHEN("decimal point after three digits") {
          CHECK(get_display_for_(uut, "123.456MHz") == " 123.456MHz\r\n");
        }

        WHEN("decimal point after four digits") {
          CHECK(get_display_for_(uut, "1234.56MHz") == " 1234.56MHz\r\n");
        }
      }

      WHEN("range is kHz") {
        WHEN("decimal point after two digits") {
          CHECK(get_display_for_(uut, "12.3456kHz") == " 12.3456kHz\r\n");
        }

        WHEN("decimal point after three digits") {
          CHECK(get_display_for_(uut, "123.456kHz") == " 123.456kHz\r\n");
        }

        WHEN("decimal point after four digits") {
          CHECK(get_display_for_(uut, "1234.56kHz") == " 1234.56kHz\r\n");
        }
      }
    }
  }

} // namespace dou
