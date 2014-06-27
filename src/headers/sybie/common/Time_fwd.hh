//sybie/commoon/Time_fwd.hh

#ifndef INCLUDE_SYBIE_COMMON_TIME_FWD_HH
#define INCLUDE_SYBIE_COMMON_TIME_FWD_HH

namespace sybie {
namespace common {

enum
{
    MillisecondsPerSecond = 1000,
    SecondsPerMinute = 60,
    MinutesPerHour = 60,
    HoursPerDay = 24,
    DaysPerWeek = 7,

    TicksPerMilliseconds = 1,
    TicksPerSecond = TicksPerMilliseconds * MillisecondsPerSecond,
    TicksPerMinute = TicksPerSecond * SecondsPerMinute,
    TicksPerHour = TicksPerMinute * MinutesPerHour,
    TicksPerDay = TicksPerHour * HoursPerDay,
    TicksPerWeek = TicksPerDay * DaysPerWeek,

    DefaultTimeZone = 0xffff
};

struct TimeSpan;
struct DateTime;
class TestTimer;
class FrequencyTimer;
class FrequencyTimerImpl;

}  //namespace common
}  //namespace sybie

#endif  //ifndef INCLUDE_SYBIE_COMMON_TIME_FWD_HH
