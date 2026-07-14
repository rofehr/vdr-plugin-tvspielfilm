#ifndef __TVSPIELFILM_RSSFETCHER_H
#define __TVSPIELFILM_RSSFETCHER_H

#include <vdr/thread.h>
#include <string>
#include <vector>
#include <map>
#include <time.h>

// ---- Feed definitions ------------------------------------------------

enum eTvsfFeed {
  tvsfJetzt = 0,
  tvsfTipps,
  tvsfHighlights,
  tvsfHeute2015,
  tvsfHeute2200,
  tvsfFeedCount
};

struct TvsfFeedInfo {
  eTvsfFeed id;
  const char *key;      // internal key, also used in setup
  const char *file;     // rss file name on tvspielfilm.de
  const char *label;    // OSD display label (German, matches app wording)
};

extern const TvsfFeedInfo TvsfFeeds[tvsfFeedCount];

// ---- One program entry -------------------------------------------------

class cTvsfEntry {
public:
  time_t   time;       // parsed start time (best effort, may be 0 if unparsable)
  std::string timeText; // raw "HH:MM" as delivered by the feed
  std::string channel;  // sender name as given in the feed
  std::string title;    // program title
  std::string description;
  std::string link;
  std::string imageUrl;
  std::string genre;    // heuristic classification, see ClassifyGenre()

  cTvsfEntry(void) : time(0) {}
};

typedef std::vector<cTvsfEntry> cTvsfEntryList;

// ---- Fetcher / background thread ---------------------------------------

class cTvsfFetcher : public cThread {
private:
  cMutex mutex;
  std::map<eTvsfFeed, cTvsfEntryList> data;
  time_t lastUpdate[tvsfFeedCount];
  int refreshIntervalSecs;
  volatile bool refreshNow;
  bool feedEnabled[tvsfFeedCount];
  std::string senderWhitelist; // comma separated, empty = all
  std::string senderBlacklist; // comma separated, only used if whitelist empty

  bool FetchFeed(eTvsfFeed feed, std::string &rawXml);
  void ParseFeed(eTvsfFeed feed, const std::string &rawXml);
  bool SenderAllowed(const std::string &sender) const;
  static std::string ClassifyGenre(const std::string &title, const std::string &description);

protected:
  void Action(void) override;

public:
  cTvsfFetcher(void);
  virtual ~cTvsfFetcher();

  void SetRefreshInterval(int secs) { refreshIntervalSecs = secs; }
  void SetFeedEnabled(eTvsfFeed feed, bool on) { feedEnabled[feed] = on; }
  bool IsFeedEnabled(eTvsfFeed feed) const { return feedEnabled[feed]; }
  void SetSenderWhitelist(const std::string &s) { senderWhitelist = s; }
  void SetSenderBlacklist(const std::string &s) { senderBlacklist = s; }

  // Thread-safe snapshot for the OSD menu code.
  cTvsfEntryList GetEntries(eTvsfFeed feed);
  time_t LastUpdate(eTvsfFeed feed);
  void TriggerRefreshNow(void);
};

#endif // __TVSPIELFILM_RSSFETCHER_H
