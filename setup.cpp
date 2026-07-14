#include "setup.h"
#include <vdr/i18n.h>

TvsfSetupData TvsfSetup;

cTvsfMenuSetupPage::cTvsfMenuSetupPage(void) {
  data = TvsfSetup;

  Add(new cMenuEditIntItem(tr("Aktualisierungsintervall (Minuten)"), &data.refreshIntervalMinutes, 1, 120));

  for (int i = 0; i < tvsfFeedCount; i++)
    Add(new cMenuEditBoolItem(TvsfFeeds[i].label, &data.feedEnabled[i]));

  Add(new cMenuEditStrItem(tr("Sender-Whitelist (Komma-getrennt, * als Platzhalter)"),
                            data.senderWhitelist, sizeof(data.senderWhitelist)));
  Add(new cMenuEditStrItem(tr("Sender-Blacklist (nur falls Whitelist leer)"),
                            data.senderBlacklist, sizeof(data.senderBlacklist)));
}

void cTvsfMenuSetupPage::Store(void) {
  TvsfSetup = data;
  SetupStore("RefreshIntervalMinutes", TvsfSetup.refreshIntervalMinutes);
  for (int i = 0; i < tvsfFeedCount; i++) {
    char key[64];
    snprintf(key, sizeof(key), "FeedEnabled.%s", TvsfFeeds[i].key);
    SetupStore(key, TvsfSetup.feedEnabled[i]);
  }
  SetupStore("SenderWhitelist", TvsfSetup.senderWhitelist);
  SetupStore("SenderBlacklist", TvsfSetup.senderBlacklist);
}
