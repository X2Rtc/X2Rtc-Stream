#ifndef __X2_CLOCK_H__
#define __X2_CLOCK_H__

#include <chrono>  // NOLINT

namespace x2rtc {

using clock = std::chrono::steady_clock;
using time_point = std::chrono::steady_clock::time_point;
using duration = std::chrono::steady_clock::duration;

class Clock {
 public:
  virtual time_point now() = 0;
  virtual ~Clock() {}
};

class SteadyClock : public Clock {
 public:
  time_point now() override {
    return clock::now();
  }
};

class SimulatedClock : public Clock {
 public:
  SimulatedClock() : now_{clock::now()} {}

  time_point now() override {
    return now_;
  }

  void advanceTime(duration duration) {
    now_ += duration;
  }
 private:
  time_point now_;
};


class ClockUtils {
public:
    static inline int64_t durationToMs(x2rtc::duration duration) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }

    static inline uint64_t timePointToMs(x2rtc::time_point time_point) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count();
    }
};


}  // namespace x2rtc
#endif  // __X2_CLOCK_H__
