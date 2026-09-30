#pragma once

#include <HalClock.h>
#include <HalStorage.h>
#include <InflateStream.h>

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

#include "CrossPointSettings.h"
#include "activities/Activity.h"
#include "activities/apps/assets/DailyOfficePropersData.h"
#include "activities/apps/assets/DailyOfficePsalterData.h"
#include "activities/apps/assets/DailyOfficeTexts.h"
#include "components/UITheme.h"
#include "components/icons/dailyoffice.h"

class DailyOfficeActivity : public Activity {
 public:
  DailyOfficeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("DailyOffice", renderer, mappedInput) {}

  void onEnter() override {
    Activity::onEnter();
    ensureSmudgeDirectories();
    initActiveDate();
    if (!areTextsInstalled()) {
      installBuiltinHours();
      installBuiltinPropers();
      installBuiltinPsalter();
      loadPropersForActiveDate();
    }
    state = State::HoursMenu;
    requestUpdate();
  }

  void onExit() override {
    clearReaderMemory();
    Activity::onExit();
  }

  void loop() override {
    switch (state) {
      case State::HoursMenu:
        loopHoursMenu();
        break;
      case State::PrayerReader:
        loopPrayerReader();
        break;
    }
  }

  void render(RenderLock&& lock) override {
    renderer.clearScreen();
    switch (state) {
      case State::HoursMenu:
        renderHoursMenu();
        break;
      case State::PrayerReader:
        renderPrayerReader();
        break;
    }
    renderer.displayBuffer();
  }

 private:
  enum class State { HoursMenu, PrayerReader };

  enum class LineType {
    HeadingDivider,
    Heading,
    Rubric,
    VersicleFirst,
    VersicleIndent,
    ResponseFirst,
    ResponseIndent,
    Body,
    Blank
  };

  struct FormattedLine {
    LineType type;
    uint32_t textOffset = 0;
  };

  struct PageInfo {
    uint16_t startLine = 0;
    uint16_t lineCount = 0;
  };

  struct HourDef {
    const char* name;
    const char* description;
    const char* filename;
  };

  static constexpr int kHourCount = 7;
  const HourDef kHours[kHourCount] = {
      {"Mattins", "Morning Prayer", "mattins.txt"}, {"Prime", "First Hour", "prime.txt"},
      {"Terce", "Third Hour", "terce.txt"},         {"Sext", "Sixth Hour", "sext.txt"},
      {"None", "Ninth Hour", "none.txt"},           {"Evensong", "Evening Prayer", "evensong.txt"},
      {"Compline", "Night Prayer", "compline.txt"},
  };

  struct PsalterDay {
    const char* mp;
    const char* ep;
  };

  static constexpr PsalterDay k30DayPsalter[31] = {
      {"1, 2, 3, 4, 5", "6, 7, 8"},
      {"9, 10, 11", "12, 13, 14"},
      {"15, 16, 17", "18"},
      {"19, 20, 21", "22, 23"},
      {"24, 25, 26", "27, 28, 29"},
      {"30, 31", "32, 33, 34"},
      {"35, 36", "37"},
      {"38, 39, 40", "41, 42, 43"},
      {"44, 45, 46", "47, 48, 49"},
      {"50, 51, 52", "53, 54, 55"},
      {"56, 57, 58", "59, 60, 61"},
      {"62, 63, 64", "65, 66, 67"},
      {"68", "69, 70"},
      {"71, 72", "73, 74"},
      {"75, 76, 77", "78"},
      {"79, 80, 81", "82, 83, 84, 85"},
      {"86, 87, 88", "89"},
      {"90, 91, 92", "93, 94"},
      {"95, 96, 97", "98, 99, 100, 101"},
      {"102, 103", "104"},
      {"105", "106"},
      {"107", "108, 109"},
      {"110, 111, 112, 113", "114, 115"},
      {"116, 117, 118", "119:1-32"},
      {"119:33-72", "119:73-104"},
      {"119:105-144", "119:145-176"},
      {"120-125", "126-131"},
      {"132-135", "136, 137, 138"},
      {"139, 140, 141", "142, 143"},
      {"144, 145, 146", "147, 148, 149, 150"},
      {"144, 145, 146", "147, 148, 149, 150"},
  };

  struct DayPropers {
    std::string dateKey;
    std::string monthDay;
    std::string title;
    std::string color;
    std::string mp_pss;
    std::string mp_l1;
    std::string mp_l1_text;
    std::string mp_l2;
    std::string mp_l2_text;
    std::string ep_pss;
    std::string ep_l1;
    std::string ep_l1_text;
    std::string ep_l2;
    std::string ep_l2_text;
    std::string collect;
    bool foundInOrdo = false;
  };

  static constexpr char kOfficeDir[] = "/.smudge/daily_office";

  State state = State::HoursMenu;
  int selectedHourIndex = 0;

  // Active Date tracking (Current Day Only)
  int activeYear = 2026;
  int activeMonth = 9;
  int activeDay = 29;
  DayPropers currentPropers;

  // Reader state
  std::string currentHourName = "";
  std::vector<FormattedLine> allLines;
  std::string textPool;
  std::vector<PageInfo> pages;
  int currentPage = 0;

  void clearReaderMemory() {
    allLines.clear();
    allLines.shrink_to_fit();
    pages.clear();
    pages.shrink_to_fit();
    textPool.clear();
    textPool.shrink_to_fit();
  }

  void addLine(LineType type, const char* str = nullptr) {
    uint32_t offset = static_cast<uint32_t>(textPool.size());
    if (str != nullptr && str[0] != '\0') {
      textPool.append(str);
    }
    textPool.push_back('\0');
    allLines.push_back({type, offset});
  }

  void addLine(LineType type, const std::string& str) { addLine(type, str.c_str()); }

  void addWrapped(LineType type, int fontId, const char* text, int maxW, int maxLines = 100,
                  EpdFontFamily::Style style = EpdFontFamily::REGULAR) {
    if (!text || text[0] == '\0') return;
    auto lines = renderer.wrappedText(fontId, text, maxW, maxLines, style);
    for (const auto& l : lines) {
      addLine(type, l.c_str());
    }
  }

  const char* getLineText(const FormattedLine& line) const {
    if (line.textOffset >= textPool.size()) return "";
    return textPool.data() + line.textOffset;
  }

  // --- Date Math Helpers ---

  static bool isLeapYear(int y) { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); }

  static int daysInMonth(int y, int m) {
    static const int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m < 1 || m > 12) return 30;
    if (m == 2 && isLeapYear(y)) return 29;
    return kDays[m - 1];
  }

  static int dayOfWeek(int y, int m, int d) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
  }

  static void adjustDateByDays(uint16_t& year, uint8_t& month, uint8_t& day, int delta) {
    while (delta > 0) {
      const int monthDays = daysInMonth(year, month);
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
      delta--;
    }

    while (delta < 0) {
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
      delta++;
    }
  }

  void initActiveDate() {
    uint16_t yr = 0;
    uint8_t mo = 0, dy = 0, hr = 0, mn = 0;
    bool valid = false;

    const uint8_t offsetQ = std::min<uint8_t>(SETTINGS.clockUtcOffsetQ, 104);
    const int offsetMinutes = (static_cast<int>(offsetQ) - 48) * 15;

    if (halClock.isAvailable()) {
      if (halClock.getDateTime(yr, mo, dy, hr, mn)) {
        if (yr >= 2024 && yr <= 2040 && mo >= 1 && mo <= 12 && dy >= 1 && dy <= 31) {
          int localMinutes = static_cast<int>(hr) * 60 + static_cast<int>(mn) + offsetMinutes;
          while (localMinutes < 0) {
            adjustDateByDays(yr, mo, dy, -1);
            localMinutes += 24 * 60;
          }
          while (localMinutes >= 24 * 60) {
            adjustDateByDays(yr, mo, dy, 1);
            localMinutes -= 24 * 60;
          }
          activeYear = yr;
          activeMonth = mo;
          activeDay = dy;
          valid = true;
        }
      }
    }

    if (!valid) {
      time_t now = time(nullptr);
      if (now > 1700000000) {
        time_t localSec = now + static_cast<time_t>(offsetMinutes * 60);
        struct tm tm_info;
        if (gmtime_r(&localSec, &tm_info)) {
          int y = tm_info.tm_year + 1900;
          int m = tm_info.tm_mon + 1;
          int d = tm_info.tm_mday;
          if (y >= 2024 && y <= 2040 && m >= 1 && m <= 12 && d >= 1 && d <= 31) {
            activeYear = y;
            activeMonth = m;
            activeDay = d;
            valid = true;
          }
        }
      }
    }

    if (!valid) {
      activeYear = 2026;
      activeMonth = 9;
      activeDay = 30;
    }

    loadPropersForActiveDate();
  }

  void loadPropersForActiveDate() { loadPropersForDate(activeYear, activeMonth, activeDay, currentPropers); }

  bool loadPropersForDate(int year, int month, int day, DayPropers& out) {
    out = DayPropers();
    char targetDate[16];
    snprintf(targetDate, sizeof(targetDate), "[%04d-%02d-%02d]", year, month, day);

    char targetMonthDay[16];
    snprintf(targetMonthDay, sizeof(targetMonthDay), "MONTH_DAY=%02d-%02d", month, day);

    char path[128];
    snprintf(path, sizeof(path), "%s/propers.txt", kOfficeDir);

    HalFile file;
    if (!Storage.openFileForRead("Office", path, file)) {
      snprintf(path, sizeof(path), "/daily_office/propers.txt");
      if (!Storage.openFileForRead("Office", path, file)) {
        populateFallbackPropers(year, month, day, out);
        return false;
      }
    }

    if (file.fileSize() < 50) {
      file.close();
      populateFallbackPropers(year, month, day, out);
      return false;
    }

    char chunk[128];
    size_t chunkPos = 0;
    size_t chunkSize = 0;

    auto readNextLine = [&](std::string& lineStr) -> bool {
      lineStr.clear();
      while (true) {
        if (chunkPos >= chunkSize) {
          int r = file.read(reinterpret_cast<uint8_t*>(chunk), sizeof(chunk));
          if (r <= 0) {
            return !lineStr.empty();
          }
          chunkPos = 0;
          chunkSize = static_cast<size_t>(r);
        }

        char ch = chunk[chunkPos++];
        if (ch == '\n') {
          break;
        }
        if (ch != '\r') {
          lineStr.push_back(ch);
        }
      }
      while (!lineStr.empty() && (lineStr.back() == ' ' || lineStr.back() == '\t')) {
        lineStr.pop_back();
      }
      return true;
    };

    std::string line;
    line.reserve(256);
    bool inTarget = false;
    DayPropers fallbackPropers;
    bool inFallback = false;
    bool foundFallback = false;

    while (readNextLine(line)) {
      if (!line.empty() && line[0] == '[') {
        if (inTarget) {
          break;
        }
        if (line.compare(0, 12, targetDate) == 0) {
          inTarget = true;
          out.dateKey = line.substr(1);
          if (!out.dateKey.empty() && out.dateKey.back() == ']') {
            out.dateKey.pop_back();
          }
          out.foundInOrdo = true;
          continue;
        }
        if (inFallback) {
          inFallback = false;
        }
      }

      if (inTarget) {
        if (line.rfind("MONTH_DAY=", 0) == 0) {
          out.monthDay = line.substr(10);
        } else if (line.rfind("TITLE=", 0) == 0) {
          out.title = line.substr(6);
        } else if (line.rfind("COLOR=", 0) == 0) {
          out.color = line.substr(6);
        } else if (line.rfind("MP_PSS=", 0) == 0) {
          out.mp_pss = line.substr(7);
        } else if (line.rfind("MP_L1_TEXT=", 0) == 0) {
          out.mp_l1_text = line.substr(11);
        } else if (line.rfind("MP_L1=", 0) == 0) {
          out.mp_l1 = line.substr(6);
        } else if (line.rfind("MP_L2_TEXT=", 0) == 0) {
          out.mp_l2_text = line.substr(11);
        } else if (line.rfind("MP_L2=", 0) == 0) {
          out.mp_l2 = line.substr(6);
        } else if (line.rfind("EP_PSS=", 0) == 0) {
          out.ep_pss = line.substr(7);
        } else if (line.rfind("EP_L1_TEXT=", 0) == 0) {
          out.ep_l1_text = line.substr(11);
        } else if (line.rfind("EP_L1=", 0) == 0) {
          out.ep_l1 = line.substr(6);
        } else if (line.rfind("EP_L2_TEXT=", 0) == 0) {
          out.ep_l2_text = line.substr(11);
        } else if (line.rfind("EP_L2=", 0) == 0) {
          out.ep_l2 = line.substr(6);
        } else if (line.rfind("COLLECT=", 0) == 0) {
          out.collect = line.substr(8);
        }
      } else if (!foundFallback) {
        if (line.compare(0, 15, targetMonthDay) == 0) {
          inFallback = true;
          foundFallback = true;
          fallbackPropers.monthDay = line.substr(10);
        } else if (inFallback) {
          if (line.rfind("TITLE=", 0) == 0) {
            fallbackPropers.title = line.substr(6);
          } else if (line.rfind("COLOR=", 0) == 0) {
            fallbackPropers.color = line.substr(6);
          } else if (line.rfind("MP_PSS=", 0) == 0) {
            fallbackPropers.mp_pss = line.substr(7);
          } else if (line.rfind("MP_L1_TEXT=", 0) == 0) {
            fallbackPropers.mp_l1_text = line.substr(11);
          } else if (line.rfind("MP_L1=", 0) == 0) {
            fallbackPropers.mp_l1 = line.substr(6);
          } else if (line.rfind("MP_L2_TEXT=", 0) == 0) {
            fallbackPropers.mp_l2_text = line.substr(11);
          } else if (line.rfind("MP_L2=", 0) == 0) {
            fallbackPropers.mp_l2 = line.substr(6);
          } else if (line.rfind("EP_PSS=", 0) == 0) {
            fallbackPropers.ep_pss = line.substr(7);
          } else if (line.rfind("EP_L1_TEXT=", 0) == 0) {
            fallbackPropers.ep_l1_text = line.substr(11);
          } else if (line.rfind("EP_L1=", 0) == 0) {
            fallbackPropers.ep_l1 = line.substr(6);
          } else if (line.rfind("EP_L2_TEXT=", 0) == 0) {
            fallbackPropers.ep_l2_text = line.substr(11);
          } else if (line.rfind("EP_L2=", 0) == 0) {
            fallbackPropers.ep_l2 = line.substr(6);
          } else if (line.rfind("COLLECT=", 0) == 0) {
            fallbackPropers.collect = line.substr(8);
          }
        }
      }
    }
    file.close();

    if (!out.foundInOrdo) {
      if (foundFallback) {
        out = fallbackPropers;
        char dk[16];
        snprintf(dk, sizeof(dk), "%04d-%02d-%02d", year, month, day);
        out.dateKey = dk;
        out.foundInOrdo = true;
      } else {
        populateFallbackPropers(year, month, day, out);
      }
    }

    return true;
  }

  void populateFallbackPropers(int year, int month, int day, DayPropers& out) {
    char dk[16];
    snprintf(dk, sizeof(dk), "%04d-%02d-%02d", year, month, day);
    out.dateKey = dk;
    char md[8];
    snprintf(md, sizeof(md), "%02d-%02d", month, day);
    out.monthDay = md;

    if (month == 9 && day == 30) {
      out.title = "Saint Jerome, Priest and Doctor of the Church Memorial";
      out.color = "White";
      out.mp_pss = "144, 145, 146";
      out.mp_l1 = "1 Mc 4:26-35";
      out.mp_l1_text =
          "Those of the foreigners who escaped went and reported to Lysias all that had happened. 27 When he heard "
          "it, he was perplexed and discouraged, for things had not happened to Israel as he had intended, nor had "
          "they turned out as the king had commanded him. 28 But the next year he mustered sixty thousand picked "
          "infantrymen and five thousand cavalry to subdue them. 29 They came into Idumea and encamped at Beth-zur, "
          "and Judas met them with ten thousand men. 30 When he saw that the army was strong, he prayed, saying, "
          "\"Blessed art thou, O Savior of Israel, who didst crush the attack of the mighty warrior by the hand of "
          "thy servant David, and didst give the camp of the Philistines into the hands of Jonathan, the son of "
          "Saul, and of the man who carried his armor. 31 So do thou hem in this army by the hand of thy people "
          "Israel, and let them be ashamed of their troops and their cavalry. 32 Fill them with cowardice; melt the "
          "boldness of their strength; let them tremble in their destruction. 33 Strike them down with the sword of "
          "those who love thee, and let all who know thy name praise thee with hymns.\" 34 Then both sides attacked, "
          "and there fell of the army of Lysias five thousand men; they fell in action. 35 And when Lysias saw the "
          "rout of his troops and observed the boldness which inspired those of Judas, and how ready they were either "
          "to live or to die nobly, he departed to Antioch and enlisted mercenaries, to invade Judea again with an "
          "even larger army.";
      out.mp_l2 = "Phlm";
      out.mp_l2_text =
          "Paul, a prisoner for Christ Jesus, and Timothy our brother, To Philemon our beloved fellow worker 2 and "
          "Apphia our sister and Archippus our fellow soldier, and the church in your house: 3 Grace to you and peace "
          "from God our Father and the Lord Jesus Christ. 4 I thank my God always when I remember you in my prayers, "
          "5 because I hear of your love and of the faith which you have toward the Lord Jesus and all the saints, 6 "
          "and I pray that the sharing of your faith may promote the knowledge of all the good that is ours in "
          "Christ. 7 For I have derived much joy and comfort from your love, my brother, because the hearts of the "
          "saints have been refreshed through you. 8 Accordingly, though I am bold enough in Christ to command you "
          "to do what is required, 9 yet for love\'s sake I prefer to appeal to you--I, Paul, an ambassador and now a "
          "prisoner also for Christ Jesus-- 10 I appeal to you for my child, Onesimus, whose father I have become in "
          "my imprisonment. 11 (Formerly he was useless to you, but now he is indeed useful to you and to me.) 12 I "
          "am sending him back to you, sending my very heart. 13 I would have been glad to keep him with me, in order "
          "that he might serve me on your behalf during my imprisonment for the gospel; 14 but I preferred to do "
          "nothing without your consent in order that your goodness might not be by compulsion but of your own free "
          "will. 15 Perhaps this is why he was parted from you for a while, that you might have him back for ever, 16 "
          "no longer as a slave but more than a slave, as a beloved brother, especially to me but how much more to "
          "you, both in the flesh and in the Lord. 17 So if you consider me your partner, receive him as you would "
          "receive me. 18 If he has wronged you at all, or owes you anything, charge that to my account. 19 I, Paul, "
          "write this with my own hand, I will repay it--to say nothing of your owing me even your own self. 20 Yes, "
          "brother, I want some benefit from you in the Lord. Refresh my heart in Christ. 21 Confident of your "
          "obedience, I write to you, knowing that you will do even more than I say. 22 At the same time, prepare a "
          "guest room for me, for I am hoping through your prayers to be granted to you. 23 Epaphras, my fellow "
          "prisoner in Christ Jesus, sends greetings to you, 24 and so do Mark, Aristarchus, Demas, and Luke, my "
          "fellow workers. 25 The grace of the Lord Jesus Christ be with your spirit.";
      out.ep_pss = "147, 148, 149, 150";
      out.ep_l1 = "1 Mc 4:36-end";
      out.ep_l1_text =
          "Then said Judas and his brothers, \"Behold, our enemies are crushed; let us go up to cleanse the sanctuary "
          "and dedicate it.\" 37 So all the army assembled and they went up to Mount Zion. 38 And they saw the "
          "sanctuary desolate, the altar profaned, and the gates burned. In the courts they saw bushes sprung up as "
          "in a thicket, or as on one of the mountains. They saw also the chambers of the priests in ruins. 39 Then "
          "they rent their clothes, and mourned with great lamentation, and sprinkled themselves with ashes. 40 They "
          "fell face down on the ground, and sounded the signal on the trumpets, and cried out to Heaven. 41 Then "
          "Judas detailed men to fight against those in the citadel until he had cleansed the sanctuary. 42 He chose "
          "blameless priests devoted to the law, 43 and they cleansed the sanctuary and removed the defiled stones to "
          "an unclean place. 44 They deliberated what to do about the altar of burnt offering, which had been "
          "profaned. 45 And they thought it best to tear it down, lest it bring reproach upon them, for the Gentiles "
          "had defiled it. So they tore down the altar, 46 and stored the stones in a convenient place on the temple "
          "hill until there should come a prophet to tell what to do with them. 47 Then they took unhewn stones, as "
          "the law directs, and built a new altar like the former one. 48 They also rebuilt the sanctuary and the "
          "interior of the temple, and consecrated the courts. 49 They made new holy vessels, and brought the "
          "lampstand, the altar of incense, and the table into the temple. 50 Then they burned incense on the altar "
          "and lighted the lamps on the lampstand, and these gave light in the temple. 51 They placed the bread on the "
          "table and hung up the curtains. Thus they finished all the work they had undertaken. 52 Early in the "
          "morning on the twenty-fifth day of the ninth month, which is the month of Chislev, in the one hundred and "
          "forty-eighth year, 53 they rose and offered sacrifice, as the law directs, on the new altar of burnt "
          "offering which they had built. 54 At the very season and on the very day that the Gentiles had profaned it, "
          "it was dedicated with songs and harps and lutes and cymbals. 55 All the people fell on their faces and "
          "worshiped and blessed Heaven, who had prospered them. 56 So they celebrated the dedication of the altar for "
          "eight days, and offered burnt offerings with gladness; they offered a sacrifice of deliverance and praise. "
          "57 They decorated the front of the temple with golden crowns and small shields; they restored the gates and "
          "the chambers for the priests, and furnished them with doors. 58 There was very great gladness among the "
          "people, and the reproach of the Gentiles was removed. 59 Then Judas and his brothers and all the assembly "
          "of Israel determined that every year at that season the days of dedication of the altar should be observed "
          "with gladness and joy for eight days, beginning with the twenty-fifth day of the month of Chislev. 60 At "
          "that time they fortified Mount Zion with high walls and strong towers round about, to keep the Gentiles "
          "from "
          "coming and trampling them down as they had done before. 61 And he stationed a garrison there to hold it. "
          "He also fortified Beth-zur, so that the people might have a stronghold that faced Idumea.";
      out.ep_l2 = "Jn 18:28-end";
      out.ep_l2_text =
          "Then they led Jesus from the house of Caiaphas to the praetorium. It was early. They themselves did not "
          "enter the praetorium, so that they might not be defiled, but might eat the passover. 29 So Pilate went "
          "out to them and said, \"What accusation do you bring against this man?\" 30 They answered him, \"If this "
          "man were not an evildoer, we would not have handed him over.\" 31 Pilate said to them, \"Take him "
          "yourselves and judge him by your own law.\" The Jews said to him, \"It is not lawful for us to put any man "
          "to death.\" 32 This was to fulfill the word which Jesus had spoken to show by what death he was to die. 33 "
          "Pilate entered the praetorium again and called Jesus, and said to him, \"Are you the King of the Jews?\" 34 "
          "Jesus answered, \"Do you say this of your own accord, or did others say it to you about me?\" 35 Pilate "
          "answered, \"Am I a Jew? Your own nation and the chief priests have handed you over to me; what have you "
          "done?\" 36 Jesus answered, \"My kingship is not of this world; if my kingship were of this world, my "
          "servants would fight, that I might not be handed over to the Jews; but my kingship is not from the "
          "world.\" 37 Pilate said to him, \"So you are a king?\" Jesus answered, \"You say that I am a king. For "
          "this I was born, and for this I have come into the world, to bear witness to the truth. Every one who is of "
          "the truth hears my voice.\" 38 Pilate said to him, \"What is truth?\" After he had said this, he went out "
          "to the Jews again, and told them, \"I find no crime in him. 39 But you have a custom that I should release "
          "one man for you at the Passover; will you have me release for you the King of the Jews?\" 40 They cried "
          "out again, \"Not this man, but Barabbas!\" Now Barabbas was a robber.";
      out.collect =
          "O GOD, who didst give unto thy Priest Saint Jerome a singular love for the holy Scriptures: grant that thy "
          "people may be more abundantly nourished by thy word, and find therein the source of eternal life; through "
          "Jesus Christ thy Son our Lord, who liveth and reigneth with thee, in the unity of the Holy Spirit, ever "
          "one God, world without end. Amen.";
      out.foundInOrdo = true;
      return;
    }

    int w = dayOfWeek(year, month, day);
    static const char* kDayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    if (w == 0) {
      out.title = "Sunday in Ordinary Time";
      out.color = "Green";
    } else {
      char tbuf[64];
      snprintf(tbuf, sizeof(tbuf), "%s Feria", kDayNames[w]);
      out.title = tbuf;
      out.color = "Green";
    }

    int pIndex = (day >= 1 && day <= 31) ? (day - 1) : 0;
    out.mp_pss = k30DayPsalter[pIndex].mp;
    out.ep_pss = k30DayPsalter[pIndex].ep;

    out.mp_l1 = "Appointed First Lesson";
    out.mp_l2 = "Appointed Second Lesson";
    out.ep_l1 = "Appointed First Lesson";
    out.ep_l2 = "Appointed Second Lesson";
    out.collect =
        "LORD, we pray thee that thy grace may always precede and follow us, and make us continually to be given to "
        "all good works; through Jesus Christ thy Son our Lord, who liveth and reigneth with thee, in the unity of "
        "the Holy Spirit, ever one God, world without end. Amen.";
    out.foundInOrdo = false;
  }

  void ensureSmudgeDirectories() {
    if (!Storage.exists("/.smudge")) {
      Storage.mkdir("/.smudge");
    }
    if (!Storage.exists(kOfficeDir)) {
      Storage.mkdir(kOfficeDir);
    }
  }

  bool areTextsInstalled() const {
    for (int i = 0; i < kHourCount; ++i) {
      char path[128];
      snprintf(path, sizeof(path), "%s/%s", kOfficeDir, kHours[i].filename);
      HalFile file;
      if (!Storage.openFileForRead("Office", path, file)) {
        return false;
      }
      size_t sz = file.fileSize();
      file.close();
      if (sz < 500) {
        return false;
      }
    }

    char propersPath[128];
    snprintf(propersPath, sizeof(propersPath), "%s/propers.txt", kOfficeDir);
    HalFile propersFile;
    if (!Storage.openFileForRead("Office", propersPath, propersFile)) {
      return false;
    }
    size_t propersSz = propersFile.fileSize();
    propersFile.close();
    if (propersSz < 200000) {
      return false;
    }

    char psalterPath[128];
    snprintf(psalterPath, sizeof(psalterPath), "%s/psalter.txt", kOfficeDir);
    HalFile psalterFile;
    if (!Storage.openFileForRead("Office", psalterPath, psalterFile)) {
      return false;
    }
    size_t psalterSz = psalterFile.fileSize();
    psalterFile.close();
    if (psalterSz < 100000) {
      return false;
    }

    return true;
  }

  void resetAllData() {
    for (int i = 0; i < kHourCount; ++i) {
      char path[128];
      snprintf(path, sizeof(path), "%s/%s", kOfficeDir, kHours[i].filename);
      if (Storage.exists(path)) {
        Storage.remove(path);
      }
    }
    char propersPath[128];
    snprintf(propersPath, sizeof(propersPath), "%s/propers.txt", kOfficeDir);
    if (Storage.exists(propersPath)) {
      Storage.remove(propersPath);
    }
    char tmpPath[128];
    snprintf(tmpPath, sizeof(tmpPath), "%s/propers.tmp", kOfficeDir);
    if (Storage.exists(tmpPath)) {
      Storage.remove(tmpPath);
    }
    char psalterPath[128];
    snprintf(psalterPath, sizeof(psalterPath), "%s/psalter.txt", kOfficeDir);
    if (Storage.exists(psalterPath)) {
      Storage.remove(psalterPath);
    }
    char tmpPsalterPath[128];
    snprintf(tmpPsalterPath, sizeof(tmpPsalterPath), "%s/psalter.tmp", kOfficeDir);
    if (Storage.exists(tmpPsalterPath)) {
      Storage.remove(tmpPsalterPath);
    }

    installBuiltinHours();
    installBuiltinPropers();
    installBuiltinPsalter();
    initActiveDate();
    loadPropersForActiveDate();
    selectedHourIndex = 0;
    state = State::HoursMenu;
    requestUpdate();
  }

  void installBuiltinHours() {
    ensureSmudgeDirectories();
    for (int i = 0; i < kHourCount; ++i) {
      char path[128];
      snprintf(path, sizeof(path), "%s/%s", kOfficeDir, kHours[i].filename);
      const char* text = DailyOfficeTexts::getTextForFilename(kHours[i].filename);
      if (text != nullptr && text[0] != '\0') {
        HalFile file;
        if (Storage.openFileForWrite("Office", path, file)) {
          size_t len = strlen(text);
          file.write(reinterpret_cast<const uint8_t*>(text), len);
          file.close();
        }
      }
    }
  }

  bool decompressPropersToSd(const char* targetPath) {
    char tmpPath[128];
    snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", targetPath);

    if (Storage.exists(tmpPath)) {
      Storage.remove(tmpPath);
    }

    HalFile destFile;
    if (!Storage.openFileForWrite("Office", tmpPath, destFile)) {
      LOG_ERR("OFFICE", "Failed to open %s for write", tmpPath);
      return false;
    }

    InflateStream stream;
    if (!stream.init(true)) {
      LOG_ERR("OFFICE", "Failed to init InflateStream");
      destFile.close();
      Storage.remove(tmpPath);
      return false;
    }

    stream.setSource(DailyOfficePropersData::kCompressedPropers, DailyOfficePropersData::kCompressedSize);

    uint8_t outBuf[512];
    bool ok = true;
    size_t totalWritten = 0;
    while (true) {
      size_t produced = 0;
      InflateStream::Status st = stream.readAtMost(outBuf, sizeof(outBuf), &produced);
      if (produced > 0) {
        if (destFile.write(outBuf, produced) != produced) {
          LOG_ERR("OFFICE", "Failed writing %zu bytes to %s", produced, tmpPath);
          ok = false;
          break;
        }
        totalWritten += produced;
      }
      if (st == InflateStream::Status::Done) {
        break;
      }
      if (st == InflateStream::Status::Error) {
        LOG_ERR("OFFICE", "InflateStream error during propers inflate");
        ok = false;
        break;
      }
    }
    destFile.close();

    if (!ok || totalWritten < 1000) {
      Storage.remove(tmpPath);
      return false;
    }

    if (Storage.exists(targetPath)) {
      Storage.remove(targetPath);
    }
    Storage.rename(tmpPath, targetPath);
    LOG_INF("OFFICE", "Successfully unpacked propers: %zu bytes written", totalWritten);
    return true;
  }

  void installBuiltinPropers() {
    char propersPath[128];
    snprintf(propersPath, sizeof(propersPath), "%s/propers.txt", kOfficeDir);
    bool needInstall = true;
    HalFile propersFile;
    if (Storage.openFileForRead("Office", propersPath, propersFile)) {
      if (propersFile.fileSize() >= 200000) {
        needInstall = false;
      }
      propersFile.close();
    }
    if (needInstall) {
      decompressPropersToSd(propersPath);
    }
  }

  bool decompressPsalterToSd(const char* targetPath) {
    char tmpPath[128];
    snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", targetPath);

    if (Storage.exists(tmpPath)) {
      Storage.remove(tmpPath);
    }

    HalFile destFile;
    if (!Storage.openFileForWrite("Office", tmpPath, destFile)) {
      LOG_ERR("OFFICE", "Failed to open %s for write", tmpPath);
      return false;
    }

    InflateStream stream;
    if (!stream.init(true)) {
      LOG_ERR("OFFICE", "Failed to init InflateStream for psalter");
      destFile.close();
      Storage.remove(tmpPath);
      return false;
    }

    stream.setSource(DailyOfficePsalterData::kCompressedPsalter, DailyOfficePsalterData::kCompressedSize);

    uint8_t outBuf[512];
    bool ok = true;
    size_t totalWritten = 0;
    while (true) {
      size_t produced = 0;
      InflateStream::Status st = stream.readAtMost(outBuf, sizeof(outBuf), &produced);
      if (produced > 0) {
        if (destFile.write(outBuf, produced) != produced) {
          LOG_ERR("OFFICE", "Failed writing %zu bytes to %s", produced, tmpPath);
          ok = false;
          break;
        }
        totalWritten += produced;
      }
      if (st == InflateStream::Status::Done) {
        break;
      }
      if (st == InflateStream::Status::Error) {
        LOG_ERR("OFFICE", "InflateStream error during psalter inflate");
        ok = false;
        break;
      }
    }
    destFile.close();

    if (!ok || totalWritten < 100000) {
      Storage.remove(tmpPath);
      return false;
    }

    if (Storage.exists(targetPath)) {
      Storage.remove(targetPath);
    }
    Storage.rename(tmpPath, targetPath);
    LOG_INF("OFFICE", "Successfully unpacked psalter: %zu bytes written", totalWritten);
    return true;
  }

  void installBuiltinPsalter() {
    char psalterPath[128];
    snprintf(psalterPath, sizeof(psalterPath), "%s/psalter.txt", kOfficeDir);
    bool needInstall = true;
    HalFile psalterFile;
    if (Storage.openFileForRead("Office", psalterPath, psalterFile)) {
      if (psalterFile.fileSize() >= 100000) {
        needInstall = false;
      }
      psalterFile.close();
    }
    if (needInstall) {
      decompressPsalterToSd(psalterPath);
    }
  }

  std::string loadLessonTextFromSd(const std::string& dateKey, const char* sectionTag) {
    char path[128];
    snprintf(path, sizeof(path), "%s/lessons/%s.txt", kOfficeDir, dateKey.c_str());
    HalFile file;
    if (!Storage.openFileForRead("Office", path, file)) {
      snprintf(path, sizeof(path), "/daily_office/lessons/%s.txt", dateKey.c_str());
      if (!Storage.openFileForRead("Office", path, file)) {
        return "";
      }
    }
    std::string targetTag = std::string("[") + sectionTag + "]";
    char chunk[256];
    size_t chunkPos = 0, chunkSize = 0;
    auto readNextLine = [&](std::string& lineStr) -> bool {
      lineStr.clear();
      while (true) {
        if (chunkPos >= chunkSize) {
          int r = file.read(reinterpret_cast<uint8_t*>(chunk), sizeof(chunk));
          if (r <= 0) return !lineStr.empty();
          chunkPos = 0;
          chunkSize = static_cast<size_t>(r);
        }
        char ch = chunk[chunkPos++];
        if (ch == '\n') break;
        if (ch != '\r') lineStr.push_back(ch);
      }
      while (!lineStr.empty() && (lineStr.back() == ' ' || lineStr.back() == '\t')) lineStr.pop_back();
      return true;
    };

    std::string line;
    std::string result;
    bool inTarget = false;
    while (readNextLine(line)) {
      if (!line.empty() && line[0] == '[') {
        if (inTarget) break;
        if (line == targetTag) {
          inTarget = true;
          continue;
        }
      }
      if (inTarget) {
        if (!result.empty()) result.push_back('\n');
        result.append(line);
      }
    }
    file.close();
    return result;
  }

  bool loadPsalmFromSd(const std::string& psalmRef, int contentW) {
    char path[128];
    snprintf(path, sizeof(path), "%s/psalter.txt", kOfficeDir);
    HalFile file;
    if (!Storage.openFileForRead("Office", path, file)) {
      snprintf(path, sizeof(path), "/daily_office/psalter.txt");
      if (!Storage.openFileForRead("Office", path, file)) {
        return false;
      }
    }
    std::string targetTag = "[PSALM " + psalmRef + "]";
    char chunk[256];
    size_t chunkPos = 0, chunkSize = 0;
    auto readNextLine = [&](std::string& lineStr) -> bool {
      lineStr.clear();
      while (true) {
        if (chunkPos >= chunkSize) {
          int r = file.read(reinterpret_cast<uint8_t*>(chunk), sizeof(chunk));
          if (r <= 0) return !lineStr.empty();
          chunkPos = 0;
          chunkSize = static_cast<size_t>(r);
        }
        char ch = chunk[chunkPos++];
        if (ch == '\n') break;
        if (ch != '\r') lineStr.push_back(ch);
      }
      while (!lineStr.empty() && (lineStr.back() == ' ' || lineStr.back() == '\t')) lineStr.pop_back();
      return true;
    };

    std::string line;
    bool inTarget = false;
    bool firstHeading = true;
    while (readNextLine(line)) {
      if (!line.empty() && line[0] == '[') {
        if (inTarget) break;
        if (line == targetTag) {
          inTarget = true;
          continue;
        }
      }
      if (inTarget) {
        if (line.empty()) {
          if (!allLines.empty() && allLines.back().type != LineType::Blank) {
            addLine(LineType::Blank);
          }
          continue;
        }
        if (firstHeading && line.rfind("Psalm ", 0) == 0) {
          firstHeading = false;
          addWrapped(LineType::Heading, UI_12_FONT_ID, line.c_str(), contentW - 20, 10, EpdFontFamily::BOLD);
          addLine(LineType::Blank);
        } else {
          addWrapped(LineType::Body, UI_12_FONT_ID, line.c_str(), contentW);
        }
      }
    }
    file.close();
    return inTarget;
  }

  static std::vector<std::string> parsePsalmList(const std::string& str) {
    std::vector<std::string> res;
    if (str.empty()) return res;
    if (str.find("119:") != std::string::npos) {
      res.push_back(str);
      return res;
    }
    size_t dash = str.find('-');
    if (dash != std::string::npos && str.find(',') == std::string::npos) {
      int start = atoi(str.substr(0, dash).c_str());
      int end = atoi(str.substr(dash + 1).c_str());
      if (start > 0 && end >= start && end <= 150) {
        for (int i = start; i <= end; ++i) {
          res.push_back(std::to_string(i));
        }
        return res;
      }
    }
    size_t start = 0;
    while (start < str.size()) {
      size_t comma = str.find(',', start);
      std::string token = (comma == std::string::npos) ? str.substr(start) : str.substr(start, comma - start);
      while (!token.empty() && (token.front() == ' ' || token.front() == '\t')) token.erase(0, 1);
      while (!token.empty() && (token.back() == ' ' || token.back() == '\t')) token.pop_back();
      if (!token.empty()) {
        size_t subDash = token.find('-');
        if (subDash != std::string::npos) {
          int s = atoi(token.substr(0, subDash).c_str());
          int e = atoi(token.substr(subDash + 1).c_str());
          if (s > 0 && e >= s && e <= 150) {
            for (int i = s; i <= e; ++i) res.push_back(std::to_string(i));
          } else {
            res.push_back(token);
          }
        } else {
          res.push_back(token);
        }
      }
      if (comma == std::string::npos) break;
      start = comma + 1;
    }
    return res;
  }

  void renderTextBlocks(const std::string& text, int contentW) {
    size_t p = 0;
    while (p < text.size()) {
      size_t np = text.find('\n', p);
      std::string para = (np == std::string::npos) ? text.substr(p) : text.substr(p, np - p);
      p = (np == std::string::npos) ? text.size() : np + 1;
      while (!para.empty() && (para.back() == '\r' || para.back() == ' ')) para.pop_back();
      if (para.empty()) {
        if (!allLines.empty() && allLines.back().type != LineType::Blank) {
          addLine(LineType::Blank);
        }
        continue;
      }
      addWrapped(LineType::Body, UI_12_FONT_ID, para.c_str(), contentW);
    }
  }

  static std::string getInvitatoryAntiphon(const std::string& title, int month, int day) {
    if (title.find("Saint") != std::string::npos || title.find("Martyr") != std::string::npos ||
        title.find("Apostle") != std::string::npos || title.find("Doctor") != std::string::npos ||
        title.find("Confessor") != std::string::npos || title.find("Bishop") != std::string::npos ||
        title.find("Virgin") != std::string::npos || title.find("Pastor") != std::string::npos ||
        title.find("Abbat") != std::string::npos) {
      return "The Lord is glorious in his Saints: * O come, let us adore him.";
    }
    if (title.find("Mary") != std::string::npos || title.find("B.V.M.") != std::string::npos ||
        title.find("Annunciation") != std::string::npos || title.find("Visitation") != std::string::npos ||
        title.find("Immaculate Conception") != std::string::npos) {
      return "The Word was made flesh of the Blessed Virgin Mary: * O come, let us adore him.";
    }
    if ((month == 12 && day >= 25) || (month == 1 && day <= 5)) {
      return "Unto us a Child is born: * O come, let us adore him.";
    }
    if (month == 1 && day >= 6 && day <= 13) {
      return "The Lord hath manifested forth his glory: * O come, let us adore him.";
    }
    int mod = day % 3;
    if (mod == 0) {
      return "The Lord is gracious and merciful: * O come, let us adore him.";
    } else if (mod == 1) {
      return "The earth is the Lord's for he made it: * O come, let us adore him.";
    } else {
      return "Worship the Lord in the beauty of holiness: * O come, let us adore him.";
    }
  }

  static std::string getBenedictusAntiphon(const std::string& title, int month, int day) {
    if (title.find("Doctor") != std::string::npos) {
      std::string sName = "Jerome";
      if (title.find("Jerome") != std::string::npos)
        sName = "Jerome";
      else if (title.find("Thérèse") != std::string::npos)
        sName = "Thérèse";
      else if (title.find("Augustine") != std::string::npos)
        sName = "Augustine";
      else if (title.find("Thomas") != std::string::npos)
        sName = "Thomas";
      return "O Teacher right excellent, light of holy Church, blessed " + sName +
             ", lover of the divine law: entreat for us the Son of God.";
    }
    if (title.find("Martyrs") != std::string::npos) {
      return "For theirs is the kingdom of heaven, who have despised earthly pleasures, * and have won the rewards of "
             "the kingdom: and have washed their robes in the blood of the Lamb.";
    }
    if (title.find("Martyr") != std::string::npos) {
      return "Except a corn of wheat fall into the ground and die, it abideth alone: * but if it die, it bringeth "
             "forth much fruit.";
    }
    if (title.find("Apostle") != std::string::npos || title.find("Evangelist") != std::string::npos) {
      return "Ye which have forsaken all, * and followed me, shall receive an hundredfold, and shall inherit "
             "everlasting life.";
    }
    if (title.find("Mary") != std::string::npos || title.find("B.V.M.") != std::string::npos) {
      return "Blessed art thou, * O Mary, for thou hast believed; and there shall be a performance in thee of those "
             "things which were told thee from the Lord, alleluia.";
    }
    if (title.find("Saint") != std::string::npos || title.find("Bishop") != std::string::npos ||
        title.find("Confessor") != std::string::npos) {
      return "Well done, good and faithful servant; thou hast been faithful over a few things: enter thou into the joy "
             "of thy Lord.";
    }
    return "In holiness and righteousness let us serve the Lord all our days.";
  }

  static std::string getMagnificatAntiphon(const std::string& title, int month, int day) {
    if (title.find("Doctor") != std::string::npos) {
      std::string sName = "Jerome";
      if (title.find("Jerome") != std::string::npos)
        sName = "Jerome";
      else if (title.find("Thérèse") != std::string::npos)
        sName = "Thérèse";
      return "O Teacher right excellent, light of holy Church, blessed " + sName +
             ", lover of the divine law: entreat for us the Son of God.";
    }
    if (title.find("Martyrs") != std::string::npos) {
      return "In the heavenly kingdom the souls of the Saints are rejoicing, who followed the footsteps of Christ "
             "their Master: * and since for love of him they freely poured forth their life-blood, therefore with "
             "Christ they reign for ever and ever.";
    }
    if (title.find("Martyr") != std::string::npos) {
      return "This is indeed a Martyr who for the Name of Christ poured forth his life-blood: * who feared not the "
             "threats of judges, but with joy attained unto the heavenly kingdom.";
    }
    if (title.find("Mary") != std::string::npos || title.find("B.V.M.") != std::string::npos) {
      return "All generations shall call me blessed: * for God hath regarded his lowly handmaiden.";
    }
    if (title.find("Saint") != std::string::npos) {
      return "O how glorious is the kingdom wherein all the Saints rejoice with Christ; * arrayed in white robes, they "
             "follow the Lamb whithersoever he goeth.";
    }
    return "My soul doth magnify the Lord, and my spirit hath rejoiced in God my Saviour.";
  }

  struct OfficeHymn {
    const char* title = nullptr;
    const char* text = nullptr;
    const char* v = nullptr;
    const char* r = nullptr;
  };

  static OfficeHymn getOfficeHymn(int hourIndex, const std::string& title, int month, int day) {
    OfficeHymn h;
    if (title.find("Doctor") != std::string::npos || title.find("Confessor") != std::string::npos ||
        title.find("Jerome") != std::string::npos || title.find("Pastor") != std::string::npos ||
        title.find("Abbat") != std::string::npos) {
      h.title = "ISTE CONFESSOR";
      h.text =
          "This the Confessor of the Lord, whose triumph\n"
          "Now all the faithful celebrate with gladness,\n"
          "On this his feast day entered into glory,\n"
          "Holy and blessed.\n\n"
          "Pious and prudent, humble, and of pure life,\n"
          "Gentle and peaceful, sober and observant,\n"
          "Never was any seen to strive more firmly\n"
          "Heavenward to travel.\n\n"
          "Ofttimes already, through his holy merit,\n"
          "Many of sick folks that with aches were burdened\n"
          "Unto their former health have been restored\n"
          "From all diseases.\n\n"
          "Wherefore in song we celebrate his honour,\n"
          "And to his memory dedicate our praises,\n"
          "That by his prayers we may obtain of Jesus\n"
          "Grace for all ages.\n\n"
          "Glory and honour to our God be given,\n"
          "God and the Son and with them both the Spirit,\n"
          "Whom through all ages in a sacred triad\n"
          "Heavens adoreth. Amen.";
      h.v = "The Lord loved him, and adorned him.";
      h.r = "He clothed him with a robe of glory.";
      return h;
    }

    if (title.find("Martyr") != std::string::npos) {
      h.title = "DEUS TUORUM MILITUM";
      h.text =
          "O God, thy soldiers\' great reward,\n"
          "Their lot, their crown, their scenic Lord,\n"
          "From all transgressions set us free\n"
          "Who sing thy Martyr\'s victory.\n\n"
          "By wisdom taught he learned to know\n"
          "The vanity of all below,\n"
          "The fleeting firms of earthly toys,\n"
          "And hastened to eternal joys.\n\n"
          "He bravely ran his painful race,\n"
          "And look the death he would not flee;\n"
          "For thee he poured his life-blood out,\n"
          "And won eternal life in thee.\n\n"
          "We therefore pray thee, Lord of love,\n"
          "Regard us from thy throne above;\n"
          "On this thy Martyr\'s triumph day\n"
          "Wash every stain of sin away.\n\n"
          "All praise to God the Father be,\n"
          "All praise, eternal Son, to thee,\n"
          "Whom with the Spirit we adore\n"
          "For ever and for evermore. Amen.";
      h.v = "The righteous shall blossom as the lily.";
      h.r = "He shall flourish for ever before the Lord.";
      return h;
    }

    if (title.find("Apostle") != std::string::npos || title.find("Evangelist") != std::string::npos) {
      h.title = "AETERNA CHRISTI MUNERA";
      h.text =
          "The eternal gifts of Christ the King,\n"
          "The Apostles\' glorious deeds, we sing;\n"
          "And while due hymns of praise we pay,\n"
          "Our thankful hearts cast grief away.\n\n"
          "The Church in them hath won renown,\n"
          "Such leaders of her host to crown;\n"
          "Triumphant soldiers of the skies,\n"
          "True lights for fainting mortal eyes.\n\n"
          "In them the steadfast faith of Saints,\n"
          "The unconquered hope that never faints,\n"
          "The love of Christ that knows no end,\n"
          "O\'er all the prince of death transcend.\n\n"
          "To God the Father, and the Son,\n"
          "And Holy Spirit, Three in One,\n"
          "Be endless glory, as before\n"
          "The world began, so evermore. Amen.";
      h.v = "Their sound is gone out into all lands.";
      h.r = "And their words into the ends of the world.";
      return h;
    }

    if (title.find("Mary") != std::string::npos || title.find("B.V.M.") != std::string::npos) {
      if (hourIndex == 0) {
        h.title = "QUEM TERRA, PONTUS, AETHERA";
        h.text =
            "The God whom earth, and sea, and sky\n"
            "Adore, and laud, and magnify,\n"
            "Who o\'er their threefold fabric reigns,\n"
            "The Virgin\'s spotless womb contains.\n\n"
            "The Lord, whose presence none can bound,\n"
            "By whom the heavens were made and crowned,\n"
            "In humble maiden\'s low abode\n"
            "Is wrapped, our Brother and our God.\n\n"
            "O Mother blest! the chosen shrine\n"
            "Wherein the Architect divine,\n"
            "Whose hand contains both land and sea,\n"
            "In all his fullness pleased to be.\n\n"
            "All honour, laud, and glory be,\n"
            "O Jesu, Virgin-born, to thee;\n"
            "Whom with the Father we adore\n"
            "And Holy Ghost for evermore. Amen.";
        h.v = "Full of grace are thy lips.";
        h.r = "Because God hath blessed thee for ever.";
      } else {
        h.title = "AVE MARIS STELLA";
        h.text =
            "Star of ocean, lead us,\n"
            "God\'s own Mother blest,\n"
            "Ever-sinless Virgin,\n"
            "Gate of heavenly rest.\n\n"
            "Taking that sweet Ave\n"
            "Which from Gabriel came,\n"
            "Peace confirm within us,\n"
            "Changing Eva\'s name.\n\n"
            "Break the captive\'s fetters,\n"
            "Light on blindness pour,\n"
            "All our ills expelling,\n"
            "Every bliss implore.\n\n"
            "Praise to God the Father,\n"
            "Honour to the Son,\n"
            "In the Holy Spirit,\n"
            "Be the glory done. Amen.";
        h.v = "Blessed art thou among women.";
        h.r = "And blessed is the fruit of thy womb.";
      }
      return h;
    }

    if (hourIndex == 0) {
      h.title = "AETERNE RERUM CONDITOR";
      h.text =
          "Maker of all, eternal King,\n"
          "Who day and night about dost bring,\n"
          "And by the change of time to show\n"
          "Thy wisdom to thy flock below.\n\n"
          "The herald of the day is heard,\n"
          "The night\'s watcher, vigilant bird;\n"
          "A light to travellers on their way,\n"
          "Dividing darkness from the day.\n\n"
          "Do thou, O Christ, our eyes unclose,\n"
          "Awaken us from sin\'s repose;\n"
          "Thy light upon our darkness pour,\n"
          "And to our souls true peace restore.\n\n"
          "To God the Father glory be,\n"
          "And to his sole-begotten Son,\n"
          "Praise to the Holy Paraclete,\n"
          "While everlasting ages run. Amen.";
      h.v = "Lord, thou hast been our refuge.";
      h.r = "From one generation to another.";
    } else {
      h.title = "LUCIS CREATOR OPTIME";
      h.text =
          "O blest Creator of the light,\n"
          "Who mak\'st the day with splendour bright,\n"
          "And forming new-made realms of light,\n"
          "Didst in the world\'s first birth delight.\n\n"
          "Who gently blending dusk with ray,\n"
          "Didst name the morn and evening day:\n"
          "Thick darkness falls across the skies;\n"
          "Hear, Father, hear our weeping cries.\n\n"
          "O Father, that we ask be done,\n"
          "Through Jesus Christ, thine only Son;\n"
          "Who, with the Holy Ghost and thee,\n"
          "Doth live and reign eternally. Amen.";
      h.v = "Let my prayer be set forth, O Lord.";
      h.r = "In thy sight as the incense.";
    }
    return h;
  }

  void handleHourMenuSelection(int index) {
    if (index < kHourCount) {
      openHour(index);
    } else {
      resetAllData();
    }
  }

  void loopHoursMenu() {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      finish();
      return;
    }

    const int totalMenuItems = kHourCount + 1;

    // Both Up/Down and Left/Right move up and down the list of hours
    if (mappedInput.wasReleased(MappedInputManager::Button::Down) ||
        mappedInput.wasReleased(MappedInputManager::Button::Right) ||
        mappedInput.wasReleased(MappedInputManager::Button::PageForward)) {
      selectedHourIndex = (selectedHourIndex + 1) % totalMenuItems;
      requestUpdate();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
               mappedInput.wasReleased(MappedInputManager::Button::Left) ||
               mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
      selectedHourIndex = (selectedHourIndex - 1 + totalMenuItems) % totalMenuItems;
      requestUpdate();
    }

    int touchItem = -1;
    if (mappedInput.wasItemTouchedDown(touchItem)) {
      if (touchItem >= 0 && touchItem < totalMenuItems) {
        selectedHourIndex = touchItem;
        handleHourMenuSelection(selectedHourIndex);
        return;
      }
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      handleHourMenuSelection(selectedHourIndex);
    }
  }

  void openHour(int index) {
    if (index < 0 || index >= kHourCount) return;

    currentHourName = kHours[index].name;

    std::string textContent = getBaseOfficeText(kHours[index].filename);
    if (textContent.size() < 500) {
      const char* builtIn = DailyOfficeTexts::getTextForFilename(kHours[index].filename);
      if (builtIn) textContent = builtIn;
    }

    buildPages(index, textContent);
    currentPage = 0;
    state = State::PrayerReader;
    requestUpdate();
  }

  void buildPages(int hourIndex, const std::string& text) {
    clearReaderMemory();

    // Pre-reserve capacities once on the clean heap
    textPool.reserve(32768);
    allLines.reserve(800);
    pages.reserve(48);

    const int pageWidth = renderer.getScreenWidth();
    const int pageHeight = renderer.getScreenHeight();
    const auto& metrics = UITheme::getInstance().getMetrics();

    const int contentX = metrics.contentSidePadding;
    const int contentW = pageWidth - contentX * 2;

    const int headerSpace = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing + 4;
    const int footerSpace = metrics.buttonHintsHeight + 16;
    const int availableHeight = pageHeight - headerSpace - footerSpace;

    const int lineH = renderer.getLineHeight(UI_12_FONT_ID) + 4;
    const int rubricLineH = renderer.getLineHeight(SMALL_FONT_ID) + 2;
    const int headingLineH = renderer.getLineHeight(UI_12_FONT_ID) + 8;
    const int dividerH = 14;

    // 1. Commemoration Header
    int w = dayOfWeek(activeYear, activeMonth, activeDay);
    static const char* kDayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    static const char* kMonthNames[] = {"January", "February", "March",     "April",   "May",      "June",
                                        "July",    "August",   "September", "October", "November", "December"};
    char dateBanner[128];
    snprintf(dateBanner, sizeof(dateBanner), "%s, %s %d, %04d", kDayNames[w], kMonthNames[activeMonth - 1], activeDay,
             activeYear);

    addLine(LineType::Heading, dateBanner);
    std::string feastLine = currentPropers.title;
    if (!currentPropers.color.empty()) {
      feastLine += " (" + currentPropers.color + ")";
    }
    addLine(LineType::Rubric, feastLine);
    addLine(LineType::HeadingDivider);

    // 2. Stream-based line parsing without string mutations
    size_t pos = 0;
    bool skippingOldLessonRubric = false;
    bool skippingOldCollectRubric = false;
    bool skippingOldPsalmody = false;
    bool skippingOldHymn = false;
    bool inInvitatory = false;
    bool inBenedictus = false;
    bool inMagnificat = false;
    bool inMinorCollect = false;
    bool lastWasHeading = false;

    while (pos < text.size()) {
      size_t nextPos = text.find('\n', pos);
      std::string line = (nextPos == std::string::npos) ? text.substr(pos) : text.substr(pos, nextPos - pos);
      pos = (nextPos == std::string::npos) ? text.size() : nextPos + 1;

      while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

      if (line.empty()) {
        if (!allLines.empty() && allLines.back().type != LineType::Blank &&
            allLines.back().type != LineType::HeadingDivider) {
          addLine(LineType::Blank);
        }
        continue;
      }

      // Skip redundant header lines from base text
      if (line.rfind("Personal Ordinariate", 0) == 0 || line.rfind("=== DIVINE WORSHIP", 0) == 0) {
        continue;
      }

      if (line.rfind("===", 0) == 0) {
        skippingOldLessonRubric = false;
        skippingOldCollectRubric = false;
        skippingOldPsalmody = false;
        skippingOldHymn = false;

        std::string title = line;
        while (!title.empty() && (title.front() == '=' || title.front() == ' ')) title.erase(0, 1);
        while (!title.empty() && (title.back() == '=' || title.back() == ' ')) title.pop_back();

        if (title.empty()) continue;

        if (title.find("HYMN: ECCE JAM NOCTIS") != std::string::npos) {
          skippingOldHymn = true;
          continue;
        }

        if (!allLines.empty() && !lastWasHeading && allLines.back().type != LineType::HeadingDivider) {
          addLine(LineType::HeadingDivider);
        }

        if (title.find("BENEDICTUS") != std::string::npos && hourIndex == 0) {
          OfficeHymn hymn = getOfficeHymn(hourIndex, currentPropers.title, activeMonth, activeDay);
          if (hymn.title != nullptr && hymn.text != nullptr) {
            std::string hTitle = std::string("=== HYMN: ") + hymn.title + " ===";
            addLine(LineType::Heading, hTitle);
            addLine(LineType::Blank);
            renderTextBlocks(hymn.text, contentW);
            addLine(LineType::Blank);
            if (hymn.v != nullptr && hymn.r != nullptr) {
              addLine(LineType::VersicleFirst, hymn.v);
              addLine(LineType::ResponseFirst, hymn.r);
              addLine(LineType::Blank);
            }
            addLine(LineType::HeadingDivider);
          }

          std::string benAnt = getBenedictusAntiphon(currentPropers.title, activeMonth, activeDay);
          if (!benAnt.empty()) {
            std::string antStr = "¶ Antiphon: " + benAnt;
            addWrapped(LineType::Rubric, SMALL_FONT_ID, antStr.c_str(), contentW - 24);
            addLine(LineType::Blank);
          }
          inBenedictus = true;
        } else if (title.find("MAGNIFICAT") != std::string::npos && hourIndex == 5) {
          OfficeHymn hymn = getOfficeHymn(hourIndex, currentPropers.title, activeMonth, activeDay);
          if (hymn.title != nullptr && hymn.text != nullptr) {
            std::string hTitle = std::string("=== HYMN: ") + hymn.title + " ===";
            addLine(LineType::Heading, hTitle);
            addLine(LineType::Blank);
            renderTextBlocks(hymn.text, contentW);
            addLine(LineType::Blank);
            if (hymn.v != nullptr && hymn.r != nullptr) {
              addLine(LineType::VersicleFirst, hymn.v);
              addLine(LineType::ResponseFirst, hymn.r);
              addLine(LineType::Blank);
            }
            addLine(LineType::HeadingDivider);
          }

          std::string magAnt = getMagnificatAntiphon(currentPropers.title, activeMonth, activeDay);
          if (!magAnt.empty()) {
            std::string antStr = "¶ Antiphon: " + magAnt;
            addWrapped(LineType::Rubric, SMALL_FONT_ID, antStr.c_str(), contentW - 24);
            addLine(LineType::Blank);
          }
          inMagnificat = true;
        }

        addWrapped(LineType::Heading, UI_12_FONT_ID, title.c_str(), contentW - 20, 10, EpdFontFamily::BOLD);
        lastWasHeading = true;

        if (title.find("INVITATORY") != std::string::npos && hourIndex == 0) {
          std::string invAnt = getInvitatoryAntiphon(currentPropers.title, activeMonth, activeDay);
          if (!invAnt.empty()) {
            std::string antStr = "¶ Antiphon: " + invAnt;
            addWrapped(LineType::Rubric, SMALL_FONT_ID, antStr.c_str(), contentW - 24);
            addLine(LineType::Blank);
          }
          inInvitatory = true;
        } else if (title.find("PSALMODY") != std::string::npos && (hourIndex == 0 || hourIndex == 5)) {
          std::string pss = (hourIndex == 0) ? currentPropers.mp_pss : (hourIndex == 5 ? currentPropers.ep_pss : "");
          int pDay = (activeDay >= 1 && activeDay <= 31) ? (activeDay - 1) : 0;
          const char* tablePss = (hourIndex == 0) ? k30DayPsalter[pDay].mp : k30DayPsalter[pDay].ep;

          std::string activePss = (pss.empty() || currentPropers.title.find("Memorial") != std::string::npos ||
                                   currentPropers.title.find("Feria") != std::string::npos)
                                      ? tablePss
                                      : pss;

          std::string pssRubric = "¶ Appointed Psalms for today: Psalm " + activePss;
          addWrapped(LineType::Rubric, SMALL_FONT_ID, pssRubric.c_str(), contentW - 24);

          std::string tableRubric =
              "¶ (30-Day Psalter: Day " + std::to_string(activeDay) + ", Psalms " + tablePss + ")";
          addWrapped(LineType::Rubric, SMALL_FONT_ID, tableRubric.c_str(), contentW - 24);
          addLine(LineType::Blank);

          auto psalmRefs = parsePsalmList(activePss);
          bool loadedAny = false;
          for (const auto& ref : psalmRefs) {
            if (loadPsalmFromSd(ref, contentW)) {
              loadedAny = true;
              addLine(LineType::Blank);
              addLine(LineType::Body, "Glory be to the Father, and to the Son :");
              addLine(LineType::Body, "and to the Holy Ghost;");
              addLine(LineType::Body, "As it was in the beginning, is now, and ever shall be :");
              addLine(LineType::Body, "world without end. Amen.");
              addLine(LineType::Blank);
              addLine(LineType::HeadingDivider);
            }
          }

          if (!loadedAny) {
            addWrapped(LineType::Rubric, SMALL_FONT_ID,
                       "¶ (Coverdale Psalter not found on SD. Reinstall data to restore.)", contentW - 24);
            addLine(LineType::Blank);
          }

          skippingOldPsalmody = true;
        } else if (title.find("FIRST LESSON") != std::string::npos && (hourIndex == 0 || hourIndex == 5)) {
          std::string l1Citation =
              (hourIndex == 0) ? currentPropers.mp_l1 : (hourIndex == 5 ? currentPropers.ep_l1 : "");
          std::string l1Text =
              (hourIndex == 0) ? currentPropers.mp_l1_text : (hourIndex == 5 ? currentPropers.ep_l1_text : "");
          if (l1Text.empty()) {
            l1Text = loadLessonTextFromSd(currentPropers.dateKey, (hourIndex == 0) ? "MP_L1" : "EP_L1");
          }

          if (!l1Citation.empty()) {
            std::string l1Rubric = "¶ Appointed First Lesson: " + l1Citation;
            addWrapped(LineType::Rubric, SMALL_FONT_ID, l1Rubric.c_str(), contentW - 24);
            addLine(LineType::Blank);
          }

          if (!l1Text.empty()) {
            renderTextBlocks(l1Text, contentW);
            addLine(LineType::Blank);
          } else {
            addWrapped(LineType::Rubric, SMALL_FONT_ID, "¶ Here is read the First Lesson from Holy Scripture.",
                       contentW - 24);
            addWrapped(LineType::Rubric, SMALL_FONT_ID, "¶ (Place offline lessons in /.smudge/daily_office/)",
                       contentW - 24);
            addLine(LineType::Blank);
          }

          addLine(LineType::Rubric, "¶ After the Lesson:");
          addLine(LineType::VersicleFirst, "Here endeth the First Lesson.");
          addLine(LineType::ResponseFirst, "Thanks be to God.");
          addLine(LineType::Blank);
          skippingOldLessonRubric = true;
        } else if (title.find("SECOND LESSON") != std::string::npos && (hourIndex == 0 || hourIndex == 5)) {
          std::string l2Citation =
              (hourIndex == 0) ? currentPropers.mp_l2 : (hourIndex == 5 ? currentPropers.ep_l2 : "");
          std::string l2Text =
              (hourIndex == 0) ? currentPropers.mp_l2_text : (hourIndex == 5 ? currentPropers.ep_l2_text : "");
          if (l2Text.empty()) {
            l2Text = loadLessonTextFromSd(currentPropers.dateKey, (hourIndex == 0) ? "MP_L2" : "EP_L2");
          }

          if (!l2Citation.empty()) {
            std::string l2Rubric = "¶ Appointed Second Lesson: " + l2Citation;
            addWrapped(LineType::Rubric, SMALL_FONT_ID, l2Rubric.c_str(), contentW - 24);
            addLine(LineType::Blank);
          }

          if (!l2Text.empty()) {
            renderTextBlocks(l2Text, contentW);
            addLine(LineType::Blank);
          } else {
            addWrapped(LineType::Rubric, SMALL_FONT_ID, "¶ Here is read the Second Lesson from Holy Scripture.",
                       contentW - 24);
            addWrapped(LineType::Rubric, SMALL_FONT_ID, "¶ (Place offline lessons in /.smudge/daily_office/)",
                       contentW - 24);
            addLine(LineType::Blank);
          }

          addLine(LineType::Rubric, "¶ After the Lesson:");
          addLine(LineType::VersicleFirst, "Here endeth the Second Lesson.");
          addLine(LineType::ResponseFirst, "Thanks be to God.");
          addLine(LineType::Blank);
          skippingOldLessonRubric = true;
        } else if (title.find("COLLECT OF THE DAY") != std::string::npos) {
          if (!currentPropers.collect.empty()) {
            addWrapped(LineType::Body, UI_12_FONT_ID, currentPropers.collect.c_str(), contentW);
            addLine(LineType::Blank);
            skippingOldCollectRubric = true;
          }
        } else if (title.find("COLLECT FOR TERCE") != std::string::npos ||
                   title.find("COLLECT FOR SEXT") != std::string::npos ||
                   title.find("COLLECT FOR NONE") != std::string::npos) {
          inMinorCollect = true;
        }
        continue;
      }

      if (skippingOldHymn) {
        if (line.rfind("===", 0) == 0) {
          skippingOldHymn = false;
        } else {
          continue;
        }
      }

      if (skippingOldPsalmody) {
        if (line.rfind("===", 0) == 0) {
          skippingOldPsalmody = false;
        } else {
          continue;
        }
      }

      if (skippingOldLessonRubric) {
        if (line.find("After the Lesson:") != std::string::npos || line.rfind("V:", 0) == 0) {
          skippingOldLessonRubric = false;
        } else {
          continue;
        }
      }

      if (skippingOldCollectRubric) {
        if (line.rfind("===", 0) == 0 || line.find("COLLECT") != std::string::npos) {
          skippingOldCollectRubric = false;
        } else {
          continue;
        }
      }

      lastWasHeading = false;

      if (inMinorCollect && (line.rfind("V:", 0) == 0 || line.rfind("¶", 0) == 0)) {
        if (!currentPropers.collect.empty()) {
          addLine(LineType::Blank);
          addLine(LineType::Rubric, "¶ Or the Collect of the Day:");
          addWrapped(LineType::Body, UI_12_FONT_ID, currentPropers.collect.c_str(), contentW);
          addLine(LineType::Blank);
        }
        inMinorCollect = false;
      }

      if (line.rfind("¶", 0) == 0 || line.rfind("[Rubric]", 0) == 0 || (line.front() == '[' && line.back() == ']')) {
        addWrapped(LineType::Rubric, SMALL_FONT_ID, line.c_str(), contentW - 24);
      } else if (line.rfind("V:", 0) == 0 || line.rfind("V.", 0) == 0) {
        std::string content = line.substr(2);
        while (!content.empty() && content.front() == ' ') content.erase(0, 1);
        auto wrapped = renderer.wrappedText(UI_12_FONT_ID, content.c_str(), contentW - 28, 100);
        for (size_t i = 0; i < wrapped.size(); ++i) {
          addLine(i == 0 ? LineType::VersicleFirst : LineType::VersicleIndent, wrapped[i].c_str());
        }
      } else if (line.rfind("R:", 0) == 0 || line.rfind("R.", 0) == 0) {
        std::string content = line.substr(2);
        while (!content.empty() && content.front() == ' ') content.erase(0, 1);
        auto wrapped = renderer.wrappedText(UI_12_FONT_ID, content.c_str(), contentW - 28, 100);
        for (size_t i = 0; i < wrapped.size(); ++i) {
          addLine(i == 0 ? LineType::ResponseFirst : LineType::ResponseIndent, wrapped[i].c_str());
        }
      } else {
        addWrapped(LineType::Body, UI_12_FONT_ID, line.c_str(), contentW);

        if (line.find("world without end. Amen.") != std::string::npos) {
          if (inInvitatory) {
            std::string invAnt = getInvitatoryAntiphon(currentPropers.title, activeMonth, activeDay);
            if (!invAnt.empty()) {
              addLine(LineType::Blank);
              std::string antStr = "¶ Antiphon: " + invAnt;
              addWrapped(LineType::Rubric, SMALL_FONT_ID, antStr.c_str(), contentW - 24);
              addLine(LineType::Blank);
            }
            inInvitatory = false;
          } else if (inBenedictus) {
            std::string benAnt = getBenedictusAntiphon(currentPropers.title, activeMonth, activeDay);
            if (!benAnt.empty()) {
              addLine(LineType::Blank);
              std::string antStr = "¶ Antiphon: " + benAnt;
              addWrapped(LineType::Rubric, SMALL_FONT_ID, antStr.c_str(), contentW - 24);
              addLine(LineType::Blank);
            }
            inBenedictus = false;
          } else if (inMagnificat) {
            std::string magAnt = getMagnificatAntiphon(currentPropers.title, activeMonth, activeDay);
            if (!magAnt.empty()) {
              addLine(LineType::Blank);
              std::string antStr = "¶ Antiphon: " + magAnt;
              addWrapped(LineType::Rubric, SMALL_FONT_ID, antStr.c_str(), contentW - 24);
              addLine(LineType::Blank);
            }
            inMagnificat = false;
          }
        }
      }
    }

    // Direct pagination from allLines
    uint16_t curPageStart = 0;
    int curPageH = 0;

    for (uint16_t i = 0; i < allLines.size(); ++i) {
      const auto& item = allLines[i];
      int itemH = lineH;
      switch (item.type) {
        case LineType::HeadingDivider:
          itemH = dividerH;
          break;
        case LineType::Heading:
          itemH = headingLineH;
          break;
        case LineType::Rubric:
          itemH = rubricLineH;
          break;
        case LineType::VersicleFirst:
        case LineType::VersicleIndent:
        case LineType::ResponseFirst:
        case LineType::ResponseIndent:
        case LineType::Body:
          itemH = lineH;
          break;
        case LineType::Blank:
          itemH = lineH / 2;
          break;
      }

      if (i == curPageStart && (item.type == LineType::Blank || item.type == LineType::HeadingDivider)) {
        curPageStart++;
        continue;
      }

      if (curPageH + itemH > availableHeight && i > curPageStart) {
        pages.push_back({curPageStart, static_cast<uint16_t>(i - curPageStart)});
        curPageStart = i;
        curPageH = 0;
        if (item.type == LineType::Blank || item.type == LineType::HeadingDivider) {
          curPageStart++;
          continue;
        }
      }

      curPageH += itemH;
    }

    if (curPageStart < allLines.size()) {
      pages.push_back({curPageStart, static_cast<uint16_t>(allLines.size() - curPageStart)});
    }

    if (pages.empty()) {
      addLine(LineType::Body, "(No content)");
      pages.push_back({0, 1});
    }
  }

  void loopPrayerReader() {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      clearReaderMemory();
      state = State::HoursMenu;
      requestUpdate();
      return;
    }

    int total = static_cast<int>(pages.size());

    // Page navigation via buttons
    if (mappedInput.wasReleased(MappedInputManager::Button::Down) ||
        mappedInput.wasReleased(MappedInputManager::Button::Right) ||
        mappedInput.wasReleased(MappedInputManager::Button::PageForward) ||
        mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      if (currentPage + 1 < total) {
        currentPage++;
        requestUpdate();
      } else {
        clearReaderMemory();
        state = State::HoursMenu;
        requestUpdate();
      }
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Up) ||
               mappedInput.wasReleased(MappedInputManager::Button::Left) ||
               mappedInput.wasReleased(MappedInputManager::Button::PageBack)) {
      if (currentPage > 0) {
        currentPage--;
        requestUpdate();
      }
    }

    // Touch screen navigation
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTouchDown(tx, ty)) {
      const int pageWidth = renderer.getScreenWidth();
      const int pageHeight = renderer.getScreenHeight();
      const auto& metrics = UITheme::getInstance().getMetrics();

      if (ty > metrics.topPadding + metrics.headerHeight && ty < pageHeight - metrics.buttonHintsHeight) {
        if (tx > pageWidth / 2) {
          if (currentPage + 1 < total) {
            currentPage++;
            requestUpdate();
          } else {
            clearReaderMemory();
            state = State::HoursMenu;
            requestUpdate();
          }
        } else {
          if (currentPage > 0) {
            currentPage--;
            requestUpdate();
          }
        }
      }
    }
  }

  // --- Render Handlers ---

  void renderHoursMenu() {
    const int pageWidth = renderer.getScreenWidth();
    const int pageHeight = renderer.getScreenHeight();
    const auto& metrics = UITheme::getInstance().getMetrics();

    const int headerY = metrics.topPadding;
    const int headerH = metrics.headerHeight;

    GUI.drawHeader(renderer, Rect{0, headerY, pageWidth, headerH}, "Daily Office", "Ordinariate N.A.");

    // Current Day banner (loads current day automatically)
    int dateY = headerY + headerH + 6;
    int w = dayOfWeek(activeYear, activeMonth, activeDay);
    static const char* kDayNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    static const char* kMonthNames[] = {"January", "February", "March",     "April",   "May",      "June",
                                        "July",    "August",   "September", "October", "November", "December"};
    char dateBanner[96];
    snprintf(dateBanner, sizeof(dateBanner), "%s, %s %d, %04d", kDayNames[w], kMonthNames[activeMonth - 1], activeDay,
             activeYear);
    renderer.drawCenteredText(UI_12_FONT_ID, dateY, dateBanner, true, EpdFontFamily::BOLD);

    // Feast title & liturgical color
    int feastY = dateY + renderer.getLineHeight(UI_12_FONT_ID) + 4;
    std::string feastText = currentPropers.title;
    if (!currentPropers.color.empty()) {
      feastText += " (" + currentPropers.color + ")";
    }
    feastText = renderer.truncatedText(SMALL_FONT_ID, feastText.c_str(), pageWidth - metrics.contentSidePadding * 2);
    renderer.drawCenteredText(SMALL_FONT_ID, feastY, feastText.c_str(), true);

    int menuY = feastY + renderer.getLineHeight(SMALL_FONT_ID) + 8;
    int menuHeight = pageHeight - menuY - metrics.buttonHintsHeight - 6;
    const int totalMenuItems = kHourCount + 1;

    GUI.drawButtonMenu(
        renderer, Rect{0, menuY, pageWidth, menuHeight}, totalMenuItems, selectedHourIndex,
        [this](int index) {
          static char buf[96];
          if (index < kHourCount) {
            snprintf(buf, sizeof(buf), "%s (%s)", kHours[index].name, kHours[index].description);
          } else {
            snprintf(buf, sizeof(buf), "Reset & Reinstall All Data");
          }
          return buf;
        },
        nullptr);

    // Bottom buttons move up and down the list of hours
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  void renderPrayerReader() {
    const int pageWidth = renderer.getScreenWidth();
    const auto& metrics = UITheme::getInstance().getMetrics();

    char pageInfo[32];
    int total = static_cast<int>(pages.size());
    snprintf(pageInfo, sizeof(pageInfo), "%d / %d", currentPage + 1, std::max(1, total));

    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, currentHourName.c_str(),
                   pageInfo);

    const int contentX = metrics.contentSidePadding;
    const int contentW = pageWidth - contentX * 2;
    int y = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing + 4;

    const int lineH = renderer.getLineHeight(UI_12_FONT_ID) + 4;
    const int rubricLineH = renderer.getLineHeight(SMALL_FONT_ID) + 2;
    const int headingLineH = renderer.getLineHeight(UI_12_FONT_ID) + 8;
    const int dividerH = 14;

    if (currentPage >= 0 && currentPage < total) {
      const auto& page = pages[currentPage];
      for (uint16_t i = 0; i < page.lineCount; ++i) {
        if (page.startLine + i >= allLines.size()) break;
        const auto& line = allLines[page.startLine + i];
        const char* lineText = getLineText(line);
        switch (line.type) {
          case LineType::HeadingDivider: {
            int mid = contentX + contentW / 2;
            renderer.drawLine(mid - 35, y + 6, mid + 35, y + 6, true);
            y += dividerH;
            break;
          }
          case LineType::Heading: {
            renderer.drawCenteredText(UI_12_FONT_ID, y, lineText, true, EpdFontFamily::BOLD);
            y += headingLineH;
            break;
          }
          case LineType::Rubric: {
            renderer.drawText(SMALL_FONT_ID, contentX + 14, y, lineText, true);
            y += rubricLineH;
            break;
          }
          case LineType::VersicleFirst: {
            renderer.drawText(UI_12_FONT_ID, contentX, y, "V:", true, EpdFontFamily::BOLD);
            renderer.drawText(UI_12_FONT_ID, contentX + 24, y, lineText, true);
            y += lineH;
            break;
          }
          case LineType::VersicleIndent: {
            renderer.drawText(UI_12_FONT_ID, contentX + 24, y, lineText, true);
            y += lineH;
            break;
          }
          case LineType::ResponseFirst: {
            renderer.drawText(UI_12_FONT_ID, contentX, y, "R:", true, EpdFontFamily::BOLD);
            renderer.drawText(UI_12_FONT_ID, contentX + 24, y, lineText, true);
            y += lineH;
            break;
          }
          case LineType::ResponseIndent: {
            renderer.drawText(UI_12_FONT_ID, contentX + 24, y, lineText, true);
            y += lineH;
            break;
          }
          case LineType::Body: {
            renderer.drawText(UI_12_FONT_ID, contentX, y, lineText, true);
            y += lineH;
            break;
          }
          case LineType::Blank: {
            y += lineH / 2;
            break;
          }
        }
      }
    }

    const char* prevLabel = (currentPage > 0) ? "Prev" : nullptr;
    const char* nextLabel = (currentPage + 1 < total) ? "Next" : "Done";
    const auto labels = mappedInput.mapLabels(tr(STR_BACK), nullptr, prevLabel, nextLabel);
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  std::string getBaseOfficeText(const char* fname) {
    char path[128];
    snprintf(path, sizeof(path), "%s/%s", kOfficeDir, fname);
    HalFile file;
    if (!Storage.openFileForRead("Office", path, file)) {
      snprintf(path, sizeof(path), "/daily_office/%s", fname);
      Storage.openFileForRead("Office", path, file);
    }
    if (file) {
      size_t sz = file.fileSize();
      if (sz > 500) {
        std::vector<char> buf(sz + 1, 0);
        file.read(reinterpret_cast<uint8_t*>(buf.data()), sz);
        file.close();
        return std::string(buf.data(), sz);
      }
      file.close();
    }
    const char* builtIn = DailyOfficeTexts::getTextForFilename(fname);
    return (builtIn != nullptr) ? std::string(builtIn) : std::string();
  }
};
