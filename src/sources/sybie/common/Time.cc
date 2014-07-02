//Time.cc

#include "sybie/common/Time.hh"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <queue>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/timeb.h>
#endif

namespace sybie {
namespace common {

namespace {

template<class T>
std::string NumberToString(const T num,
                           const int insert_to_size = 0,
                           const char insert_char = '0')
{
    char buf[64];
    sprintf(buf, "%lld", (int64_t)num);
    std::string result(buf);
    while (result.size() < std::string::size_type(insert_to_size))
        result = insert_char + result;
    return result;
}

int64_t GetMilliseconds()
{
#ifdef _WIN32
    return timeGetTime();
#else
    timeb t;
    ftime(&t);
    return (int64_t)t.time * MillisecondsPerSecond + t.millitm;
#endif
}

} //namespace

//struct DateTime

DateTime DateTime::Now()
{
    static DateTime init_time(DateTime::FromCTime(time(NULL)));
    static int64_t init_mtime(GetMilliseconds());
    
    int64_t now_mtime(GetMilliseconds());
    return init_time + TimeSpan::FromMilliseconds((double)(now_mtime-init_mtime));
}

DateTime DateTime::MinValue()
{
    return DateTime(INT64_MIN);
}

DateTime DateTime::MaxValue()
{
    return DateTime(INT64_MAX);
}

DateTime DateTime::FromCTime(time_t t)
{
    return DateTime(static_cast<int64_t>(t) * TicksPerSecond);
}

int CurrentTimeZone()
{
    time_t rawtime;
    time(&rawtime);

    int h1 = localtime(&rawtime)->tm_hour;
    int h2 = gmtime(&rawtime)->tm_hour;
    int z = h1-h2;
    if (z > 11)
        z -= 12;
    if (z < -12)
        z += 12;
    return z;
}

DateTime::DateTime()
{ }

DateTime::DateTime(const DateTime& another)
    : _ticks(another._ticks)
{ }

DateTime::DateTime(const int64_t ticks)
    : _ticks(ticks)
{ }

DateTime::~DateTime()
{ }

DateTime& DateTime::operator=(const DateTime& another)
{
    this->_ticks = another._ticks;
    return *this;
}

DateTime& DateTime::operator+=(const TimeSpan& span) 
{
    this->_ticks += span.Ticks();
    return *this;
}

DateTime& DateTime::operator-=(const TimeSpan& span)
{
    this->_ticks -= span.Ticks();
    return *this;
}

DateTime DateTime::operator+(const TimeSpan& span) const
{
    return DateTime(this->_ticks + span.Ticks());
}

DateTime DateTime::operator-(const TimeSpan& span) const
{
    return DateTime(this->_ticks - span.Ticks());
}

TimeSpan DateTime::operator-(const DateTime& another) const
{
    return TimeSpan(this->_ticks - another.Ticks());
}

bool DateTime::operator==(const DateTime& another) const
{
    return this->_ticks == another._ticks;
}

bool DateTime::operator!=(const DateTime& another) const
{
    return this->_ticks != another._ticks;
}

bool DateTime::operator>=(const DateTime& another) const
{
    return this->_ticks >= another._ticks;
}

bool DateTime::operator<=(const DateTime& another) const
{
    return this->_ticks <= another._ticks;
}

bool DateTime::operator>(const DateTime& another) const
{
    return this->_ticks > another._ticks;
}

bool DateTime::operator<(const DateTime& another) const
{
    return this->_ticks <= another._ticks;
}

int64_t DateTime::Ticks() const
{
    return this->_ticks;
}

time_t DateTime::ToCTime() const
{
    return static_cast<time_t>(_ticks/TicksPerSecond);
}

    void SetTimeZone(int& timezone)
    {
        if (timezone == DefaultTimeZone)
            timezone = CurrentTimeZone();
    }

std::string DateTime::ToDate(int timezone) const
{
    return ToString(timezone).substr(0,10);
}

std::string DateTime::ToTime(int timezone) const
{
    return ToString(timezone).substr(11,8);
}

std::string DateTime::ToString(int timezone) const
{
    SetTimeZone(timezone);

    DateTime time_timezone = (*this) + TimeSpan::FromHours(timezone);
    time_t ctime_timezone = time_timezone.ToCTime();
    tm* timeinfo = gmtime(&ctime_timezone);

    char s[50];
    sprintf(s, "%04d-%02d-%02d %02d:%02d:%02d"
        ,timeinfo->tm_year+1900
        ,timeinfo->tm_mon+1
        ,timeinfo->tm_mday
        ,timeinfo->tm_hour
        ,timeinfo->tm_min
        ,timeinfo->tm_sec);
    return s;
}

std::string DateTime::ToLongString(int timezone) const
{
    char m[10];
    sprintf(m, "%03d", (int)(_ticks % MillisecondsPerSecond));
    return ToString(timezone)+"."+m;
}

//struct TimeSpan

  static bool& DefaultTimeSpanFormatIsFixed()
  {
      static bool val = false;
      return val;
  }

void TimeSpan::SetDefaultTimeSpanFormat_Nature()
{
    DefaultTimeSpanFormatIsFixed() = false;
}

void TimeSpan::SetDefaultTimeSpanFormat_Fixed()
{
    DefaultTimeSpanFormatIsFixed() = true;
}

TimeSpan TimeSpan::FromMilliseconds(double milliseconds)
{
    return TimeSpan(static_cast<int64_t>(TicksPerMilliseconds * milliseconds));
}

TimeSpan TimeSpan::FromSeconds(double seconds)
{
    return TimeSpan(static_cast<int64_t>(TicksPerSecond * seconds));
}

TimeSpan TimeSpan::FromMinutes(double minutes)
{
    return TimeSpan(static_cast<int64_t>(TicksPerMinute * minutes));
}

TimeSpan TimeSpan::FromHours(double hours)
{
    return TimeSpan(static_cast<int64_t>(TicksPerHour * hours));
}

TimeSpan TimeSpan::FromDays(double days)
{
    return TimeSpan(static_cast<int64_t>(TicksPerDay * days));
}

TimeSpan TimeSpan::FromWeeks(double hours)
{
    return TimeSpan(static_cast<int64_t>(TicksPerWeek * hours));
}

TimeSpan::TimeSpan()
    : _ticks(0)
{ }

TimeSpan::TimeSpan(const TimeSpan& another)
    : _ticks(another._ticks)
{ }

TimeSpan::TimeSpan(int64_t ticks)
    : _ticks(ticks)
{ }

TimeSpan::~TimeSpan()
{ }

TimeSpan& TimeSpan::operator=(const TimeSpan& another)
{
    _ticks = another._ticks;
    return *this;
}

TimeSpan& TimeSpan::operator+=(const TimeSpan& another)
{
    _ticks += another._ticks;
    return *this;
}

TimeSpan& TimeSpan::operator-=(const TimeSpan& another)
{
    _ticks -= another._ticks;
    return *this;
}

TimeSpan TimeSpan::operator+(const TimeSpan& another) const
{
    return TimeSpan(this->_ticks + another._ticks);
}

DateTime TimeSpan::operator+(const DateTime& datetime) const
{
    return DateTime(this->_ticks + datetime.Ticks());
}

TimeSpan TimeSpan::operator-(const TimeSpan& another) const
{
    return TimeSpan(this->_ticks - another._ticks);
}

TimeSpan TimeSpan::operator*(int64_t times) const
{
    return TimeSpan(this->_ticks * times);
}

TimeSpan TimeSpan::operator*(double times) const
{
    return TimeSpan((int64_t)(this->_ticks * times));
}

bool TimeSpan::operator==(const TimeSpan& another) const
{
    return this->_ticks == another._ticks;
}

bool TimeSpan::operator!=(const TimeSpan& another) const
{
    return this->_ticks != another._ticks;
}

bool TimeSpan::operator>=(const TimeSpan& another) const
{
    return this->_ticks >= another._ticks;
}

bool TimeSpan::operator<=(const TimeSpan& another) const
{
    return this->_ticks <= another._ticks;
}

bool TimeSpan::operator>(const TimeSpan& another) const
{
    return this->_ticks > another._ticks;
}

bool TimeSpan::operator<(const TimeSpan& another) const
{
    return this->_ticks < another._ticks;
}

int64_t TimeSpan::Ticks() const
{
    return _ticks;
}

int TimeSpan::MilliSeconds() const
{
    return (_ticks / TicksPerMilliseconds) % MillisecondsPerSecond;
}

int TimeSpan::Seconds() const
{
    return (_ticks / TicksPerSecond) % SecondsPerMinute;
}

int TimeSpan::Minutes() const
{
    return (_ticks / TicksPerMinute) % MinutesPerHour;
}

int TimeSpan::Hours() const
{
    return (_ticks / TicksPerHour) % HoursPerDay;
}

int TimeSpan::Days() const
{
    return (int)(_ticks / TicksPerDay);
}

double TimeSpan::ToMilliSeconds() const
{
    return double(_ticks) / TicksPerMilliseconds;
}

double TimeSpan::ToSeconds() const
{
    return double(_ticks) / TicksPerSecond;
}

double TimeSpan::ToMinutes() const
{
    return double(_ticks) / TicksPerMinute;
}

double TimeSpan::ToHours() const
{
    return double(_ticks) / TicksPerHour;
}

double TimeSpan::ToDays() const
{
    return double(_ticks) / TicksPerDay;
}

double TimeSpan::ToWeeks() const
{
    return double(_ticks) / TicksPerWeek;
}

std::string TimeSpan::ToString() const
{
    if (DefaultTimeSpanFormatIsFixed())
        return ToFixedFormatString();
    else
        return ToReadFormatString();
}

std::string TimeSpan::ToReadFormatString() const
{
    bool t = false;
    std::string result;
    if (Days() > 0)
    {
        result += NumberToString(Days()) + "days ";
        t = true;
    }
    if (Hours() > 0 || t)
    {
        result += NumberToString(Hours(), t?2:0) + ":";
        t = true;
    }
    if (Minutes() > 0 || t)
    {
        result += NumberToString(Minutes(), t?2:0) + ":";
        t = true;
    }

    if (t)
        result += NumberToString(Seconds(), 2);
    else
        result += NumberToString(Seconds(), 1);
    result += "."+NumberToString(MilliSeconds(), 3);
    if (!t)
        result += "s";

    return result;
}

std::string TimeSpan::ToFixedFormatString() const
{
    return NumberToString(Days() * HoursPerDay + Hours()) + ":"
         + NumberToString(Minutes(), 2) + ":"
         + NumberToString(Seconds(), 2) + "."
         + NumberToString(MilliSeconds(), 3);
}

//class TestTimer

TestTimer::TestTimer(const std::string& content)
    : _start_time(DateTime::Now())
     ,_content(content)
{ }

TestTimer::~TestTimer()
{
    if (_content.size() > 0)
        std::cout<<_content.c_str()
                 <<" completed by: "
                 <<GetTimeSpan()
                 <<std::endl;
}

void TestTimer::Reset()
{
    _start_time = DateTime::Now();
}

TimeSpan TestTimer::GetTimeSpan() const
{
    return DateTime::Now() - _start_time;
}

TimeSpan TestTimer::GetTimeSpanAndReset()
{
    TimeSpan result = GetTimeSpan();
    Reset();
    return result;
}

//class StatingTestTimerGlobal

class StatingTestTimerGlobal
{
public: //static
    static StatingTestTimerGlobal& Get()
    {
        static StatingTestTimerGlobal global_instance;
        return global_instance;
    }
public:
    void Add(const std::string& key, const TimeSpan time)
    {
        _stat_map[key] += time;
    }
    void Reset(const std::string& key)
    {
        _stat_map[key] = TimeSpan::FromSeconds(0);
    }
    const TimeSpan GetStat(const std::string& key)
    {
        return _stat_map[key];
    }
    const void ShowAll(std::ostream& os)
    {
        for (auto& pair_key_time : _stat_map)
        {
            os<<"["<<pair_key_time.first<<"]"<<pair_key_time.second<<std::endl;
        }
    }
    const void ResetAll()
    {
        for (auto& pair_key_time : _stat_map)
        {
            pair_key_time.second = TimeSpan::FromSeconds(0);
        }
    }

    ~StatingTestTimerGlobal() {}
private:
    StatingTestTimerGlobal() : _stat_map() { }

    std::map<std::string, TimeSpan> _stat_map;
}; //class StatingTestTimerGlobal

//StatingTestTimer

TimeSpan StatingTestTimer::GetStatTime(const std::string& stat_key)
{
    return StatingTestTimerGlobal::Get().GetStat(stat_key);
}

TimeSpan StatingTestTimer::GetStatTimeAndReset(const std::string& stat_key)
{
    TimeSpan result = StatingTestTimerGlobal::Get().GetStat(stat_key);
    StatingTestTimerGlobal::Get().Reset(stat_key);
    return result;
}

void StatingTestTimer::ShowAll(std::ostream& os)
{
    StatingTestTimerGlobal::Get().ShowAll(os);
}

void StatingTestTimer::ResetAll()
{
    StatingTestTimerGlobal::Get().ResetAll();
}

StatingTestTimer::StatingTestTimer(const std::string& stat_key)
    : _stat_key(stat_key),
      _start_time(DateTime::Now()),
      _finished(false)
{ }

StatingTestTimer::~StatingTestTimer()
{
    Finish();
}

void StatingTestTimer::Finish()
{
    if (!_finished)
    {
        StatingTestTimerGlobal::Get().Add(_stat_key, DateTime::Now() - _start_time);
        _finished = true;
    }
}

class FrequencyTimerImpl : common::Uncopyable
{
public:
    explicit FrequencyTimerImpl(const TimeSpan& time_span)
        : _tick_queue(), _time_span(time_span)
    { }

    ~FrequencyTimerImpl() throw()
    { }

    void Tick()
    {
        _tick_queue.push(DateTime::Now());
    }

    int Count()
    {
        DateTime now = DateTime::Now();
        while (now - _tick_queue.front() > _time_span)
            _tick_queue.pop();
        return static_cast<int>(_tick_queue.size());
    }
private:
    std::queue<DateTime> _tick_queue;
    TimeSpan _time_span;
}; //class FrequencyTimerImpl

//class FrequencyTimer

FrequencyTimer::FrequencyTimer() throw()
    : _impl(nullptr)
{ }

FrequencyTimer::FrequencyTimer(const TimeSpan& time_span)
    : _impl(new FrequencyTimerImpl(time_span))
{ }

FrequencyTimer::FrequencyTimer(FrequencyTimer&& another) throw()
    : _impl(nullptr)
{
    Swap(another);
}

FrequencyTimer::~FrequencyTimer() throw()
{
    delete _impl;
}

FrequencyTimer& FrequencyTimer::operator=(FrequencyTimer&& another) throw()
{
    Swap(another);
    return *this;
}

void FrequencyTimer::Swap(FrequencyTimer& another) throw()
{
    std::swap(_impl, another._impl);
}

void FrequencyTimer::Tick()
{
    _impl->Tick();
}

int FrequencyTimer::Count()
{
    return _impl->Count();
}

}  //namespace common
}  //namespace sybie


std::ostream& operator<<(std::ostream& os, const sybie::common::DateTime& x)
{
    os<<x.ToString();
    return os;
}

std::ostream& operator<<(std::ostream& os, const sybie::common::TimeSpan& x)
{
    os<<x.ToString();
    return os;
}
