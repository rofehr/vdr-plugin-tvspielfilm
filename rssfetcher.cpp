#include "rssfetcher.h"
#include <curl/curl.h>
#include <vdr/tools.h>
#include <algorithm>
#include <cctype>
#include <cstring>

const TvsfFeedInfo TvsfFeeds[tvsfFeedCount] = {
  { tvsfJetzt,      "jetzt",      "jetzt",      "Jetzt im TV" },
  { tvsfTipps,      "tipps",      "tipps",      "TV-Tipps" },
  { tvsfHighlights, "highlights", "filme",      "Spielfilm-Highlights" },
  { tvsfHeute2015,  "heute2015",  "heute2015",  "Heute um 20:15 Uhr" },
  { tvsfHeute2200,  "heute2200",  "heute2200",  "Heute um 22:00 Uhr" },
};

static const char *BaseUrl = "https://www.tvspielfilm.de/tv-programm/rss/";

// -------------------------------------------------------------------------
// libcurl plumbing
// -------------------------------------------------------------------------

static size_t CurlWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata) {
  std::string *out = static_cast<std::string *>(userdata);
  out->append(ptr, size * nmemb);
  return size * nmemb;
}

bool cTvsfFetcher::FetchFeed(eTvsfFeed feed, std::string &rawXml) {
  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  std::string url = std::string(BaseUrl) + TvsfFeeds[feed].file + ".xml";

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rawXml);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "vdr-plugin-tvspielfilm/1.0");
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

  CURLcode res = curl_easy_perform(curl);
  long httpCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    esyslog("tvspielfilm: curl error fetching %s: %s", url.c_str(), curl_easy_strerror(res));
    return false;
  }
  if (httpCode < 200 || httpCode >= 300) {
    esyslog("tvspielfilm: HTTP %ld fetching %s", httpCode, url.c_str());
    return false;
  }
  return true;
}

// -------------------------------------------------------------------------
// Minimal RSS/XML helpers
//
// We deliberately avoid pulling in an XML library (expat/tinyxml) as an
// extra BitBake dependency, since the feed structure is small and highly
// regular: a flat list of <item>...</item> blocks with a fixed set of
// simple, non-nested child tags. A hand-rolled extractor is sufficient
// and keeps the recipe dependency-free beyond libcurl.
// -------------------------------------------------------------------------

static std::string DecodeEntities(const std::string &in) {
  std::string out;
  out.reserve(in.size());
  for (size_t i = 0; i < in.size(); ) {
    if (in.compare(i, 5, "&amp;") == 0)      { out += '&'; i += 5; }
    else if (in.compare(i, 4, "&lt;") == 0)  { out += '<'; i += 4; }
    else if (in.compare(i, 4, "&gt;") == 0)  { out += '>'; i += 4; }
    else if (in.compare(i, 6, "&quot;") == 0){ out += '"'; i += 6; }
    else if (in.compare(i, 6, "&apos;") == 0){ out += '\''; i += 6; }
    else { out += in[i]; i += 1; }
  }
  return out;
}

static std::string StripHtmlTags(const std::string &in) {
  std::string out;
  out.reserve(in.size());
  bool inTag = false;
  for (char c : in) {
    if (c == '<') { inTag = true; continue; }
    if (c == '>') { inTag = false; continue; }
    if (!inTag) out += c;
  }
  return out;
}

static std::string Trim(const std::string &s) {
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

// Extracts the text content of the first <tag>...</tag> occurrence inside
// block, unwrapping a CDATA section if present. Returns empty string if
// the tag is not found.
static std::string ExtractTag(const std::string &block, const std::string &tag) {
  std::string openA = "<" + tag + ">";
  std::string openB = "<" + tag; // handles tags with attributes, e.g. <enclosure ...>
  size_t startPos = block.find(openA);
  size_t contentStart = std::string::npos;
  if (startPos != std::string::npos) {
    contentStart = startPos + openA.size();
  } else {
    startPos = block.find(openB);
    if (startPos == std::string::npos)
      return "";
    size_t tagClose = block.find('>', startPos);
    if (tagClose == std::string::npos)
      return "";
    contentStart = tagClose + 1;
  }
  std::string closeTag = "</" + tag + ">";
  size_t endPos = block.find(closeTag, contentStart);
  if (endPos == std::string::npos)
    return "";
  std::string raw = block.substr(contentStart, endPos - contentStart);

  // Unwrap CDATA if present
  size_t cdataStart = raw.find("<![CDATA[");
  if (cdataStart != std::string::npos) {
    size_t cdataEnd = raw.find("]]>", cdataStart);
    if (cdataEnd != std::string::npos)
      raw = raw.substr(cdataStart + 9, cdataEnd - (cdataStart + 9));
  }
  return Trim(StripHtmlTags(DecodeEntities(raw)));
}

// enclosure url="..." is an attribute, not a text node -> special-cased.
static std::string ExtractAttribute(const std::string &block, const std::string &tag, const std::string &attr) {
  size_t tagPos = block.find("<" + tag);
  if (tagPos == std::string::npos)
    return "";
  size_t tagEnd = block.find('>', tagPos);
  if (tagEnd == std::string::npos)
    return "";
  std::string tagContent = block.substr(tagPos, tagEnd - tagPos);
  std::string needle = attr + "=\"";
  size_t attrPos = tagContent.find(needle);
  if (attrPos == std::string::npos)
    return "";
  attrPos += needle.size();
  size_t attrEnd = tagContent.find('"', attrPos);
  if (attrEnd == std::string::npos)
    return "";
  return tagContent.substr(attrPos, attrEnd - attrPos);
}

// title looks like: "20:15 | Das Erste | Der Saarland-Krimi: Bruder, Liebe, Tod"
static void SplitTitle(const std::string &rawTitle, std::string &timeText, std::string &channel, std::string &title) {
  std::vector<std::string> parts;
  size_t start = 0;
  while (true) {
    size_t sep = rawTitle.find(" | ", start);
    if (sep == std::string::npos) {
      parts.push_back(Trim(rawTitle.substr(start)));
      break;
    }
    parts.push_back(Trim(rawTitle.substr(start, sep - start)));
    start = sep + 3;
  }
  if (parts.size() >= 3) {
    timeText = parts[0];
    channel  = parts[1];
    title    = parts[2];
  } else if (parts.size() == 2) {
    timeText = parts[0];
    title    = parts[1];
  } else if (!parts.empty()) {
    title = parts[0];
  }
}

std::string cTvsfFetcher::ClassifyGenre(const std::string &title, const std::string &description) {
  // Simple keyword heuristic. This is NOT the app's editorial genre
  // assignment - just a rough bucket for filtering/sorting in the OSD.
  struct Rule { const char *keyword; const char *genre; };
  static const Rule rules[] = {
    { "Krimi",       "Krimi" },
    { "Tatort",      "Krimi" },
    { "Dokumentation","Dokumentation" },
    { "Doku",        "Dokumentation" },
    { "Reportage",   "Dokumentation" },
    { "Komödie",     "Komödie" },
    { "Show",        "Show" },
    { "Nachrichten", "Nachrichten" },
    { "Sport",       "Sport" },
    { "Fußball",     "Sport" },
    { "Kinderfilm",  "Kinder" },
    { "Zeichentrick","Kinder" },
    { nullptr, nullptr }
  };
  std::string haystack = title + " " + description;
  for (int i = 0; rules[i].keyword; i++) {
    if (haystack.find(rules[i].keyword) != std::string::npos)
      return rules[i].genre;
  }
  return "Sonstiges";
}

bool cTvsfFetcher::SenderAllowed(const std::string &sender) const {
  auto matchList = [&](const std::string &list) -> bool {
    if (list.empty())
      return false;
    size_t start = 0;
    while (start < list.size()) {
      size_t sep = list.find(',', start);
      std::string entry = Trim(sep == std::string::npos ? list.substr(start) : list.substr(start, sep - start));
      if (!entry.empty()) {
        // very small wildcard support: "RTL*" / "*RTL"
        if (entry.front() == '*' && sender.size() >= entry.size() - 1 &&
            sender.compare(sender.size() - (entry.size() - 1), entry.size() - 1, entry.substr(1)) == 0)
          return true;
        if (entry.back() == '*' && sender.compare(0, entry.size() - 1, entry.substr(0, entry.size() - 1)) == 0)
          return true;
        if (entry == sender)
          return true;
      }
      if (sep == std::string::npos) break;
      start = sep + 1;
    }
    return false;
  };

  if (!senderWhitelist.empty())
    return matchList(senderWhitelist);
  if (!senderBlacklist.empty())
    return !matchList(senderBlacklist);
  return true;
}

void cTvsfFetcher::ParseFeed(eTvsfFeed feed, const std::string &rawXml) {
  cTvsfEntryList entries;
  size_t pos = 0;
  while (true) {
    size_t itemStart = rawXml.find("<item>", pos);
    if (itemStart == std::string::npos) break;
    size_t itemEnd = rawXml.find("</item>", itemStart);
    if (itemEnd == std::string::npos) break;
    std::string block = rawXml.substr(itemStart, itemEnd - itemStart);
    pos = itemEnd + 7;

    std::string rawTitle = ExtractTag(block, "title");
    std::string description = ExtractTag(block, "description");
    std::string link = ExtractTag(block, "link");
    std::string imageUrl = ExtractAttribute(block, "enclosure", "url");

    cTvsfEntry e;
    SplitTitle(rawTitle, e.timeText, e.channel, e.title);
    e.description = description;
    e.link = link;
    e.imageUrl = imageUrl;
    e.genre = ClassifyGenre(e.title, e.description);

    if (e.channel.empty() || SenderAllowed(e.channel))
      entries.push_back(e);
  }

  cMutexLock lock(&mutex);
  data[feed] = entries;
  lastUpdate[feed] = time(nullptr);
}

// -------------------------------------------------------------------------
// Thread / public interface
// -------------------------------------------------------------------------

cTvsfFetcher::cTvsfFetcher(void)
: cThread("tvspielfilm-fetcher")
{
  refreshIntervalSecs = 5 * 60; // matches the cadence of the ioBroker adapter
  refreshNow = false;
  for (int i = 0; i < tvsfFeedCount; i++) {
    lastUpdate[i] = 0;
    feedEnabled[i] = true;
  }
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

cTvsfFetcher::~cTvsfFetcher() {
  Cancel(5);
  curl_global_cleanup();
}

void cTvsfFetcher::Action(void) {
  while (Running()) {
    for (int i = 0; i < tvsfFeedCount && Running(); i++) {
      eTvsfFeed feed = static_cast<eTvsfFeed>(i);
      if (!feedEnabled[i])
        continue;
      std::string rawXml;
      if (FetchFeed(feed, rawXml))
        ParseFeed(feed, rawXml);
      else
        esyslog("tvspielfilm: could not update feed '%s'", TvsfFeeds[i].key);
    }
    refreshNow = false;
    for (int s = 0; s < refreshIntervalSecs && Running() && !refreshNow; s++)
      cCondWait::SleepMs(1000);
  }
}

cTvsfEntryList cTvsfFetcher::GetEntries(eTvsfFeed feed) {
  cMutexLock lock(&mutex);
  auto it = data.find(feed);
  if (it == data.end())
    return cTvsfEntryList();
  return it->second;
}

time_t cTvsfFetcher::LastUpdate(eTvsfFeed feed) {
  cMutexLock lock(&mutex);
  return lastUpdate[feed];
}

void cTvsfFetcher::TriggerRefreshNow(void) {
  // Breaks the Action() sleep loop within ~1s so the OSD's "refresh now"
  // command doesn't have to wait for the full interval.
  refreshNow = true;
}
