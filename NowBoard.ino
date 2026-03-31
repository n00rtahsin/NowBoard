#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <time.h>

// ---------------- USER SETTINGS ----------------
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// Dhaka
const float LATITUDE  = 23.8103;
const float LONGITUDE = 90.4125;
const char* TZ_INFO = "UTC-6";

// Sports league
// EPL=4328, NBA=4387, NFL=4391, La Liga=4335
const char* SPORTS_LEAGUE_ID = "4328";

// Refresh intervals
const unsigned long WEATHER_UPDATE_MS = 15UL * 60UL * 1000UL;
const unsigned long SPORTS_UPDATE_MS  = 2UL  * 60UL * 1000UL;
const unsigned long NEWS_UPDATE_MS    = 10UL * 60UL * 1000UL;

// Animation
const unsigned long PANEL_SWITCH_MS = 4500;
const unsigned long PANEL_ANIM_MS   = 500;

// ---------------- DISPLAY ----------------
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Layout
const int LEFT_W  = 42;
const int RIGHT_X = 44;
const int RIGHT_W = 84;
const int RIGHT_H = 64;

// ---------------- WEATHER CACHE ----------------
float currentTemp = 0.0;
char weatherText[24] = "Loading";
bool weatherOk = false;

// ---------------- SPORTS CACHE ----------------
char sportsLeague[20] = "League";
char sportsHome[8] = "---";
char sportsAway[8] = "---";
int sportsHomeScore = -1;
int sportsAwayScore = -1;
char sportsStatus[16] = "Loading";
bool sportsOk = false;

// ---------------- NEWS CACHE ----------------
char newsHeadline[96] = "Loading news...";
bool newsOk = false;

// ---------------- TIMERS ----------------
unsigned long lastWeatherUpdate = 0;
unsigned long lastSportsUpdate = 0;
unsigned long lastNewsUpdate = 0;
unsigned long lastPanelSwitch = 0;
unsigned long animStart = 0;

// Right panel pages: 0=weather, 1=sports, 2=news
int currentPanel = 0;
bool animating = false;
int animOffsetY = 0;

// ------------------------------------------------

String weatherCodeToText(int code) {
  switch (code) {
    case 0: return "Clear";
    case 1: return "Mostly clr";
    case 2: return "Partly cld";
    case 3: return "Cloudy";
    case 45:
    case 48: return "Fog";
    case 51:
    case 53:
    case 55: return "Drizzle";
    case 61:
    case 63:
    case 65: return "Rain";
    case 71:
    case 73:
    case 75: return "Snow";
    case 80:
    case 81:
    case 82: return "Showers";
    case 95: return "Thunder";
    default: return "Unknown";
  }
}

String trimSpaces(String s) {
  s.trim();
  while (s.indexOf("  ") >= 0) s.replace("  ", " ");
  return s;
}

String extractTag(String src, const char* startTag, const char* endTag) {
  int s = src.indexOf(startTag);
  if (s < 0) return "";
  s += strlen(startTag);
  int e = src.indexOf(endTag, s);
  if (e < 0) return "";
  return src.substring(s, e);
}

String decodeHtml(String s) {
  s.replace("&amp;", "&");
  s.replace("&quot;", "\"");
  s.replace("&#39;", "'");
  s.replace("&apos;", "'");
  s.replace("&lt;", "<");
  s.replace("&gt;", ">");
  return s;
}

// Known abbreviations first, fallback after that
String teamAbbr(String name) {
  String n = name;
  n.toLowerCase();
  n.trim();

  if (n == "manchester united") return "MUN";
  if (n == "manchester city") return "MCI";
  if (n == "liverpool") return "LIV";
  if (n == "arsenal") return "ARS";
  if (n == "chelsea") return "CHE";
  if (n == "tottenham") return "TOT";
  if (n == "tottenham hotspur") return "TOT";
  if (n == "newcastle") return "NEW";
  if (n == "newcastle united") return "NEW";
  if (n == "aston villa") return "AVL";
  if (n == "everton") return "EVE";
  if (n == "west ham") return "WHU";
  if (n == "west ham united") return "WHU";
  if (n == "brighton") return "BHA";
  if (n == "brighton & hove albion") return "BHA";
  if (n == "wolves") return "WOL";
  if (n == "wolverhampton wanderers") return "WOL";
  if (n == "nottingham forest") return "NFO";
  if (n == "leicester city") return "LEI";
  if (n == "barcelona") return "BAR";
  if (n == "real madrid") return "RMA";
  if (n == "atletico madrid") return "ATM";
  if (n == "bayern munich") return "BAY";
  if (n == "juventus") return "JUV";
  if (n == "inter") return "INT";
  if (n == "inter milan") return "INT";
  if (n == "ac milan") return "MIL";
  if (n == "psg") return "PSG";
  if (n == "paris saint-germain") return "PSG";

  // fallback: take first letters of words
  String out = "";
  bool takeNext = true;
  for (int i = 0; i < name.length(); i++) {
    char c = name.charAt(i);
    if (isAlpha(c) && takeNext) {
      out += (char)toupper(c);
      takeNext = false;
      if (out.length() == 3) break;
    }
    if (c == ' ' || c == '-' || c == '&') takeNext = true;
  }

  // if still short, fill from letters
  for (int i = 0; i < name.length() && out.length() < 3; i++) {
    char c = name.charAt(i);
    if (isAlpha(c)) {
      char up = toupper(c);
      bool exists = false;
      for (int j = 0; j < out.length(); j++) {
        if (out[j] == up) { exists = true; break; }
      }
      if (!exists) out += up;
    }
  }

  while (out.length() < 3) out += "-";
  return out.substring(0, 3);
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
  }
}

void reconnectIfNeeded() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(250);
  }
}

void setupTimeSync() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", TZ_INFO, 1);
  tzset();

  struct tm timeinfo;
  int retries = 0;
  while (!getLocalTime(&timeinfo) && retries < 20) {
    delay(500);
    retries++;
  }
}

bool updateWeather() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(LATITUDE, 4) +
               "&longitude=" + String(LONGITUDE, 4) +
               "&current=temperature_2m,weather_code&timezone=auto";

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, payload)) return false;
  if (!doc["current"].is<JsonObject>()) return false;

  currentTemp = doc["current"]["temperature_2m"] | 0.0;
  int currentWeatherCode = doc["current"]["weather_code"] | -1;

  String w = weatherCodeToText(currentWeatherCode);
  w.toCharArray(weatherText, sizeof(weatherText));

  weatherOk = true;
  lastWeatherUpdate = millis();
  return true;
}

bool updateSports() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = "https://www.thesportsdb.com/api/v1/json/123/eventspastleague.php?id=" + String(SPORTS_LEAGUE_ID);

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(24576);
  if (deserializeJson(doc, payload)) return false;

  JsonArray events = doc["events"].as<JsonArray>();
  if (events.isNull() || events.size() == 0) return false;

  JsonObject ev = events[0];

  String league = ev["strLeague"] | "League";
  String home   = ev["strHomeTeam"] | "Home";
  String away   = ev["strAwayTeam"] | "Away";
  String status = ev["strStatus"] | "";
  String progress = ev["strProgress"] | "";

  sportsHomeScore = ev["intHomeScore"] | -1;
  sportsAwayScore = ev["intAwayScore"] | -1;

  String homeA = teamAbbr(home);
  String awayA = teamAbbr(away);
  String dispStatus = progress.length() ? progress : (status.length() ? status : "Recent");

  league.toCharArray(sportsLeague, sizeof(sportsLeague));
  homeA.toCharArray(sportsHome, sizeof(sportsHome));
  awayA.toCharArray(sportsAway, sizeof(sportsAway));
  dispStatus.toCharArray(sportsStatus, sizeof(sportsStatus));

  sportsOk = true;
  lastSportsUpdate = millis();
  return true;
}

bool updateNews() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin("https://feeds.bbci.co.uk/news/rss.xml");
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  String xml = http.getString();
  http.end();

  // first item title
  int itemStart = xml.indexOf("<item>");
  if (itemStart < 0) return false;

  String firstItem = xml.substring(itemStart, min((int)xml.length(), itemStart + 1200));
  String title = extractTag(firstItem, "<title>", "</title>");
  title = decodeHtml(trimSpaces(title));

  if (title.length() == 0) return false;

  // avoid too-long garbage
  if (title.length() > 90) title = title.substring(0, 90);

  title.toCharArray(newsHeadline, sizeof(newsHeadline));
  newsOk = true;
  lastNewsUpdate = millis();
  return true;
}

void drawLeftTimePanel() {
  struct tm timeinfo;
  bool timeOk = getLocalTime(&timeinfo);

  char hhmm[8] = "--:--";
  char secBuf[4] = "--";
  char dayBuf[8] = "---";
  char dateBuf[8] = "--";

  if (timeOk) {
    strftime(hhmm, sizeof(hhmm), "%H:%M", &timeinfo);
    strftime(secBuf, sizeof(secBuf), "%S", &timeinfo);
    strftime(dayBuf, sizeof(dayBuf), "%a", &timeinfo);
    strftime(dateBuf, sizeof(dateBuf), "%d", &timeinfo);
  }

  u8g2.drawFrame(0, 0, LEFT_W, 64);

  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(4, 11, dayBuf);

  u8g2.setFont(u8g2_font_logisoso16_tn);
  u8g2.drawStr(2, 33, hhmm);

  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(14, 47, secBuf);
  u8g2.drawStr(14, 61, dateBuf);
}

void drawWeatherCard(int y) {
  char tempBuf[16];
  if (weatherOk) snprintf(tempBuf, sizeof(tempBuf), "%.1fC", currentTemp);
  else snprintf(tempBuf, sizeof(tempBuf), "--.-C");

  u8g2.drawFrame(RIGHT_X, y, RIGHT_W, RIGHT_H);
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(RIGHT_X + 4, y + 12, "WEATHER");

  u8g2.setFont(u8g2_font_logisoso16_tn);
  u8g2.drawStr(RIGHT_X + 4, y + 34, tempBuf);

  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(RIGHT_X + 4, y + 50, weatherOk ? weatherText : "Unavailable");
}

void drawSportsCard(int y) {
  char scoreBuf[20];
  if (sportsHomeScore >= 0 && sportsAwayScore >= 0)
    snprintf(scoreBuf, sizeof(scoreBuf), "%s %d-%d %s", sportsHome, sportsHomeScore, sportsAwayScore, sportsAway);
  else
    snprintf(scoreBuf, sizeof(scoreBuf), "%s vs %s", sportsHome, sportsAway);

  u8g2.drawFrame(RIGHT_X, y, RIGHT_W, RIGHT_H);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(RIGHT_X + 4, y + 10, "SPORTS");

  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(RIGHT_X + 4, y + 26, scoreBuf);

  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(RIGHT_X + 4, y + 40, sportsStatus);
  u8g2.drawStr(RIGHT_X + 4, y + 54, sportsLeague);
}

void drawNewsCard(int y) {
  u8g2.drawFrame(RIGHT_X, y, RIGHT_W, RIGHT_H);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(RIGHT_X + 4, y + 10, "NEWS");

  u8g2.setFont(u8g2_font_5x8_tf);

  // Simple 3-line wrap
  String s = newsOk ? String(newsHeadline) : String("Unavailable");
  int maxChars = 22;
  int idx = 0;

  for (int line = 0; line < 3 && idx < s.length(); line++) {
    int end = idx + maxChars;
    if (end < s.length()) {
      int sp = s.lastIndexOf(' ', end);
      if (sp > idx) end = sp;
    } else {
      end = s.length();
    }

    String part = s.substring(idx, end);
    part.trim();

    char buf[32];
    part.toCharArray(buf, sizeof(buf));
    u8g2.drawStr(RIGHT_X + 4, y + 24 + line * 12, buf);

    idx = end + 1;
  }
}

void drawRightPanelStatic(int panel, int y) {
  if (panel == 0) drawWeatherCard(y);
  else if (panel == 1) drawSportsCard(y);
  else drawNewsCard(y);
}

void startPanelAnimation() {
  currentPanel = (currentPanel + 1) % 3;
  animating = true;
  animStart = millis();
  lastPanelSwitch = millis();
}

void updateAnimation() {
  if (!animating) {
    animOffsetY = 0;
    return;
  }

  unsigned long elapsed = millis() - animStart;
  if (elapsed >= PANEL_ANIM_MS) {
    animating = false;
    animOffsetY = 0;
    return;
  }

  animOffsetY = map(elapsed, 0, PANEL_ANIM_MS, 64, 0);
}

void drawScreen() {
  updateAnimation();

  u8g2.clearBuffer();

  drawLeftTimePanel();

  if (animating) {
    int prevPanel = (currentPanel + 2) % 3;
    drawRightPanelStatic(prevPanel, -animOffsetY);
    drawRightPanelStatic(currentPanel, 64 - animOffsetY);
  } else {
    drawRightPanelStatic(currentPanel, 0);
  }

  if (WiFi.status() == WL_CONNECTED) {
    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawStr(LEFT_W - 16, 6, "WiFi");
  }

  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);

  u8g2.begin();
  u8g2.setI2CAddress(0x3C * 2); // if blank, try 0x3D * 2

  connectWiFi();
  setupTimeSync();

  updateWeather();
  updateSports();
  updateNews();

  lastPanelSwitch = millis();
}

void loop() {
  reconnectIfNeeded();

  if (millis() - lastWeatherUpdate > WEATHER_UPDATE_MS) updateWeather();
  if (millis() - lastSportsUpdate  > SPORTS_UPDATE_MS)  updateSports();
  if (millis() - lastNewsUpdate    > NEWS_UPDATE_MS)    updateNews();

  if (!animating && millis() - lastPanelSwitch > PANEL_SWITCH_MS) {
    startPanelAnimation();
  }

  drawScreen();
  delay(40);
}