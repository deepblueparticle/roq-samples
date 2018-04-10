/* Copyright (c) 2017-2018, Hans Erik Thrane */

#pragma once

namespace examples {
namespace simple {

enum class PositionType {
  StartOfDay,   // today's start of day position
  NewActivity,  // today's new activity
  Current       // current position
};

class Position final {
 public:
  void reset();
  void set_start_of_day(double position);
  void add_new_activity(double quantity);
  double get(PositionType type) const;

 private:
  double _start_of_day = 0.0;
  double _new_activity = 0.0;
};

}  // namespace simple
}  // namespace examples
