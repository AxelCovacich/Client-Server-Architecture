// clock.hpp
#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <chrono>

/**
 * @brief Abstract interface for a clock.
 * This allows injecting a mock clock for testing time-dependent logic.
 */
class IClock {
  public:
    virtual ~IClock() = default;
    virtual std::time_t now() const = 0;
};

/**
 * @brief Real-world clock implementation.
 */
class SystemClock : public IClock {
  public:
    std::time_t now() const override {
        return std::time(nullptr);
    }
};

#endif // CLOCK_HPP