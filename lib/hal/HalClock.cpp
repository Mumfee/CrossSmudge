#include "HalClock.h"

#include <Logging.h>
#include <WiFi.h>
#include <esp_netif.h>
#include <esp_netif_sntp.h>
#include <lwip/dns.h>
#include <time.h>

#include <cassert>

HalClock halClock;  // Singleton instance

namespace {
constexpr uint16_t kBaseYear = 2000;
constexpr const char* kMonthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
constexpr char kFullMonthNames[][10] = {"January", "February", "March",     "April",   "May",      "June",
                                        "July",    "August",   "September", "October", "November", "December"};

esp_err_t clearDnsCache(void*) {
  dns_clear_cache();
  return ESP_OK;
}

bool isLeapYear(const uint16_t year) { return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0; }

uint8_t daysInMonth(const uint16_t year, const uint8_t month) {
  static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 0;
  if (month == 2 && isLeapYear(year)) return 29;
  return days[month - 1];
}

bool isValidDate(const uint16_t year, const uint8_t month, const uint8_t day) {
  if (year < kBaseYear || year > 2099) return false;
  const uint8_t monthDays = daysInMonth(year, month);
  return monthDays > 0 && day >= 1 && day <= monthDays;
}

void adjustDateByDays(uint16_t& year, uint8_t& month, uint8_t& day, const int dayDelta) {
  if (dayDelta > 0) {
    const uint8_t monthDays = daysInMonth(year, month);
    if (day < monthDays) {
      day++;
    } else {
      day = 1;
      if (month < 12) {
        month++;
      } else {
        month = 1;
        year++;
      }
    }
  } else if (dayDelta < 0) {
    if (day > 1) {
      day--;
    } else {
      if (month > 1) {
        month--;
      } else {
        month = 12;
        year--;
      }
      day = daysInMonth(year, month);
    }
  }
}
}  // namespace

void HalClock::begin() {
  _available = _sdkRtc.begin();
  LOG_INF("CLK", _available ? "SDK RTC found" : "RTC not found");
}

bool HalClock::getTime(uint8_t& hour, uint8_t& minute) const {
  if (!_available) return false;

  const unsigned long now = millis();
  if (_lastPollMs != 0 && (now - _lastPollMs) < CLOCK_POLL_MS) {
    hour = _cachedHour;
    minute = _cachedMinute;
    return true;
  }

  Rtc::DateTime dt;
  if (!_sdkRtc.now(dt)) {
    if (!_hasCachedTime) return false;
    _lastPollMs = now;
    hour = _cachedHour;
    minute = _cachedMinute;
    return true;
  }

  _cachedHour = dt.hour;
  _cachedMinute = dt.minute;
  _cachedYear = dt.year;
  _cachedMonth = dt.month;
  _cachedDay = dt.day;
  _lastPollMs = now;
  _hasCachedTime = true;
  _hasCachedDate = isValidDate(_cachedYear, _cachedMonth, _cachedDay);

  hour = _cachedHour;
  minute = _cachedMinute;
  return true;
}

bool HalClock::formatTime(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased, bool use12Hour) const {
  if (bufSize < (use12Hour ? 9u : 6u)) return false;
  uint8_t h, m;
  if (!getTime(h, m)) return false;

  // Apply UTC offset: convert biased value to signed quarter-hours.
  // Clamp against corrupted persisted values so display time can't drift outside [-12:00, +14:00].
  if (utcOffsetQuarterHoursBiased > 104) utcOffsetQuarterHoursBiased = 104;
  int offsetQuarterHours = static_cast<int>(utcOffsetQuarterHoursBiased) - 48;
  int totalMinutes = static_cast<int>(h) * 60 + static_cast<int>(m) + offsetQuarterHours * 15;

  totalMinutes = ((totalMinutes % 1440) + 1440) % 1440;

  const int hour24 = totalMinutes / 60;
  const int min = totalMinutes % 60;
  if (use12Hour) {
    const bool pm = hour24 >= 12;
    int hour12 = hour24 % 12;
    if (hour12 == 0) hour12 = 12;
    snprintf(buf, bufSize, "%d:%02d %s", hour12, min, pm ? "PM" : "AM");
  } else {
    snprintf(buf, bufSize, "%02d:%02d", hour24, min);
  }
  return true;
}

bool HalClock::getDate(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute) const {
  if (!_available) return false;

  const unsigned long now = millis();
  if (_lastPollMs != 0 && (now - _lastPollMs) < CLOCK_POLL_MS && _hasCachedDate) {
    year = _cachedYear;
    month = _cachedMonth;
    day = _cachedDay;
    hour = _cachedHour;
    minute = _cachedMinute;
    return true;
  }

  Rtc::DateTime dt;
  if (!_sdkRtc.now(dt)) {
    if (!_hasCachedDate) return false;
    _lastPollMs = now;
    year = _cachedYear;
    month = _cachedMonth;
    day = _cachedDay;
    hour = _cachedHour;
    minute = _cachedMinute;
    return true;
  }

  _cachedYear = dt.year;
  _cachedMonth = dt.month;
  _cachedDay = dt.day;
  _cachedHour = dt.hour;
  _cachedMinute = dt.minute;
  _lastPollMs = now;
  _hasCachedTime = true;
  _hasCachedDate = isValidDate(_cachedYear, _cachedMonth, _cachedDay);
  if (!_hasCachedDate) return false;

  year = _cachedYear;
  month = _cachedMonth;
  day = _cachedDay;
  hour = _cachedHour;
  minute = _cachedMinute;
  return true;
}

bool HalClock::formatDate(char* buf, size_t bufSize, uint8_t utcOffsetQuarterHoursBiased, const DateFormat dateFormat,
                          const char numericSeparator) const {
  if (bufSize < 13u) return false;

  uint16_t year;
  uint8_t month, day, hour, minute;
  if (!getDate(year, month, day, hour, minute)) return false;

  if (utcOffsetQuarterHoursBiased > 104) utcOffsetQuarterHoursBiased = 104;
  const int offsetQuarterHours = static_cast<int>(utcOffsetQuarterHoursBiased) - 48;
  const int localMinutes = static_cast<int>(hour) * 60 + static_cast<int>(minute) + offsetQuarterHours * 15;
  const int dayDelta = localMinutes < 0 ? -1 : (localMinutes >= 1440 ? 1 : 0);
  adjustDateByDays(year, month, day, dayDelta);
  if (!isValidDate(year, month, day)) return false;

  const unsigned int displayMonth = month;
  const unsigned int displayDay = day;
  const unsigned int displayYear = year;
  const char separator = numericSeparator == '.' || numericSeparator == '-' ? numericSeparator : '/';
  switch (dateFormat) {
    case DAY_MONTH_YEAR_LONG:
      snprintf(buf, bufSize, "%02u %s %u", displayDay, kMonthNames[month - 1], displayYear);
      break;
    case MONTH_DAY_YEAR_NUMERIC:
      snprintf(buf, bufSize, "%02u%c%02u%c%u", displayMonth, separator, displayDay, separator, displayYear);
      break;
    case DAY_MONTH_YEAR_NUMERIC:
      snprintf(buf, bufSize, "%02u%c%02u%c%u", displayDay, separator, displayMonth, separator, displayYear);
      break;
    case YEAR_MONTH_DAY_NUMERIC:
      snprintf(buf, bufSize, "%u%c%02u%c%02u", displayYear, separator, displayMonth, separator, displayDay);
      break;
    case MONTH_DAY_NUMERIC:
      snprintf(buf, bufSize, "%02u%c%02u", displayMonth, separator, displayDay);
      break;
    case DAY_MONTH_NUMERIC:
      snprintf(buf, bufSize, "%02u%c%02u", displayDay, separator, displayMonth);
      break;
    case MONTH_DAY_LONG:
      snprintf(buf, bufSize, "%s %02u", kFullMonthNames[month - 1], displayDay);
      break;
    case DAY_MONTH_LONG:
      snprintf(buf, bufSize, "%02u %s", displayDay, kFullMonthNames[month - 1]);
      break;
    case MONTH_DAY_YEAR_LONG:
    default:
      snprintf(buf, bufSize, "%s %02u, %u", kMonthNames[month - 1], displayDay, displayYear);
      break;
  }
  return true;
}

bool HalClock::writeDateTimeToRTC(uint16_t year, uint8_t month, uint8_t day, uint8_t weekday, uint8_t hour,
                                  uint8_t minute, uint8_t second) {
  assert(hour < 24);
  assert(minute < 60);
  assert(second < 60);
  assert(isValidDate(year, month, day));
  assert(weekday >= 1 && weekday <= 7);

  Rtc::DateTime dt;
  dt.year = year;
  dt.month = month;
  dt.day = day;
  dt.weekday = weekday;
  dt.hour = hour;
  dt.minute = minute;
  dt.second = second;
  if (!_sdkRtc.set(dt)) {
    LOG_ERR("CLK", "Failed to write date/time to SDK RTC");
    return false;
  }

  _lastPollMs = 0;
  _cachedHour = hour;
  _cachedMinute = minute;
  _cachedYear = year;
  _cachedMonth = month;
  _cachedDay = day;
  _hasCachedTime = true;
  _hasCachedDate = true;
  return true;
}

bool HalClock::syncFromNTP() {
  if (!_available) return false;

  if (!syncSystemTimeFromNTP()) return false;

  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);

  const uint16_t year = static_cast<uint16_t>(timeinfo.tm_year + 1900);
  const uint8_t month = static_cast<uint8_t>(timeinfo.tm_mon + 1);
  const uint8_t day = static_cast<uint8_t>(timeinfo.tm_mday);
  const uint8_t weekday = static_cast<uint8_t>(timeinfo.tm_wday + 1);
  if (!writeDateTimeToRTC(year, month, day, weekday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec)) {
    return false;
  }

  LOG_INF("CLK", "RTC set to %04d-%02d-%02d %02d:%02d:%02d UTC", year, month, day, timeinfo.tm_hour, timeinfo.tm_min,
          timeinfo.tm_sec);
  return true;
}

bool HalClock::syncSystemTimeFromNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    LOG_ERR("CLK", "WiFi not connected, cannot sync NTP");
    return false;
  }

  esp_netif_sntp_deinit();

  LOG_INF("CLK", "Starting NTP sync...");
  // esp-netif owns the small temporary SNTP state/semaphore and routes all
  // lwIP timer changes through its core-lock-safe execution path.
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  const esp_err_t initResult = esp_netif_sntp_init(&config);
  if (initResult != ESP_OK) {
    LOG_ERR("CLK", "Failed to start NTP sync: %s", esp_err_to_name(initResult));
    return false;
  }

  const esp_err_t syncResult = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(5000));
  // An SNTP timeout can leave its DNS callback pending. Drain that callback
  // under lwIP's core lock before stopping SNTP; the next hostname lookup may
  // otherwise invoke it from the application task while clearing the cache.
  const esp_err_t clearResult = esp_netif_tcpip_exec(clearDnsCache, nullptr);
  esp_netif_sntp_deinit();
  if (clearResult != ESP_OK) {
    LOG_ERR("CLK", "Failed to clear DNS after NTP sync: %s", esp_err_to_name(clearResult));
    return false;
  }
  if (syncResult != ESP_OK) {
    LOG_ERR("CLK", "NTP sync failed: %s", esp_err_to_name(syncResult));
    return false;
  }

  return true;
}
