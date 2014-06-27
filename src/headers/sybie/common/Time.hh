//sybie/common/Time.hh

#ifndef INCLUDE_SYBIE_COMMON_TIME_HH
#define INCLUDE_SYBIE_COMMON_TIME_HH

#include "sybie/common/Time_fwd.hh"

#include <ctime>
#include <cstdint>
#include <string>
#include <ostream>

#include "sybie/common/Uncopyable.hh"

namespace sybie {
namespace common {

struct DateTime
{
public:
    static DateTime Now();
    static DateTime MinValue();
    static DateTime MaxValue();
    static int CurrentTimeZone();
    static DateTime FromCTime(time_t t);
public:
    DateTime();
    DateTime(const DateTime& another);
    explicit DateTime(const int64_t ticks);
    ~DateTime();

    DateTime& operator=(const DateTime& another);
    DateTime& operator+=(const TimeSpan& span);
    DateTime& operator-=(const TimeSpan& span);

    DateTime operator+(const TimeSpan& span) const;
    DateTime operator-(const TimeSpan& span) const;
    TimeSpan operator-(const DateTime& another) const;

    bool operator==(const DateTime& another) const;
    bool operator!=(const DateTime& another) const;
    bool operator>=(const DateTime& another) const;
    bool operator<=(const DateTime& another) const;
    bool operator>(const DateTime& another) const;
    bool operator<(const DateTime& another) const;

    int64_t Ticks() const;
    time_t ToCTime() const;

    std::string ToDate(int timezone = DefaultTimeZone) const; //YYYY::MM:DD
    std::string ToTime(int timezone = DefaultTimeZone) const; //HH:MM:SS
    std::string ToString(int timezone = DefaultTimeZone) const; //YYYY::MM:DD HH:MM:SS
    std::string ToLongString(int timezone = DefaultTimeZone) const; //YYYY::MM:DD HH:MM:SS.xxx
private:
    int64_t _ticks;
}; //struct DateTime

struct TimeSpan
{
public:
    static void SetDefaultTimeSpanFormat_Nature();
    static void SetDefaultTimeSpanFormat_Fixed();
public:
    static TimeSpan FromMilliseconds(double milliseconds);
    static TimeSpan FromSeconds(double seconds);
    static TimeSpan FromMinutes(double minutes);
    static TimeSpan FromHours(double hours);
    static TimeSpan FromDays(double days);
    static TimeSpan FromWeeks(double hours);
public:
    TimeSpan();
    TimeSpan(const TimeSpan& another);
    explicit TimeSpan(int64_t ticks);
    ~TimeSpan();

    TimeSpan& operator=(const TimeSpan& another);
    TimeSpan& operator+=(const TimeSpan& another);
    TimeSpan& operator-=(const TimeSpan& another);
    TimeSpan& operator*=(int64_t);

    TimeSpan operator+(const TimeSpan& another) const;
    DateTime operator+(const DateTime& datetime) const;
    TimeSpan operator-(const TimeSpan& another) const;
    TimeSpan operator*(int64_t times) const;
    TimeSpan operator*(double times) const;

    bool operator==(const TimeSpan& another) const;
    bool operator!=(const TimeSpan& another) const;
    bool operator>=(const TimeSpan& another) const;
    bool operator<=(const TimeSpan& another) const;
    bool operator>(const TimeSpan& another) const;
    bool operator<(const TimeSpan& another) const;

    int64_t Ticks() const;

    int MilliSeconds() const;
    int Seconds() const;
    int Minutes() const;
    int Hours() const;
    int Days() const;

    double ToMilliSeconds() const;
    double ToSeconds() const;
    double ToMinutes() const;
    double ToHours() const;
    double ToDays() const;
    double ToWeeks() const;

    std::string ToString() const;

    std::string ToReadFormatString() const;

    std::string ToFixedFormatString() const;
private:
    int64_t _ticks;
}; //struct TimeSpan

class TestTimer : Uncopyable
{
public:
    explicit TestTimer(const std::string& content = "");
    ~TestTimer();
    void Reset();
    TimeSpan GetTimeSpan() const;
    TimeSpan GetTimeSpanAndReset();
private:
    DateTime _start_time;
    const std::string _content;
}; //class TestTimer

class StatingTestTimer : Uncopyable
{
public:
    static TimeSpan GetStatTime(const std::string& stat_key);
    static TimeSpan GetStatTimeAndReset(const std::string& stat_key);
    static void ShowAll(std::ostream& os);
    static void ResetAll();
public:
    explicit StatingTestTimer(const std::string& stat_key = "");
    ~StatingTestTimer();
private:
    const std::string _stat_key;
    const DateTime _start_time;
}; //class StatingTestTimer

class FrequencyTimer : Uncopyable
{
public:
    FrequencyTimer() throw();
    explicit FrequencyTimer(const TimeSpan& time_span);
    FrequencyTimer(FrequencyTimer&& another) throw();
    ~FrequencyTimer() throw();
    FrequencyTimer& operator=(FrequencyTimer&& another) throw();
    void Swap(FrequencyTimer& another) throw();
public:
    void Tick();
    int Count();
private:
    FrequencyTimerImpl* _impl;
}; //class FrequencyTimer

}  //namespace common
}  //namespace sybie

std::ostream& operator<<(std::ostream& os, const sybie::common::DateTime& x);
std::ostream& operator<<(std::ostream& os, const sybie::common::TimeSpan& x);

#endif  //ifndef INCLUDE_SYBIE_COMMON_TIME_HH
