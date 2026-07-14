#include <vdr/plugin.h>
#include "rssfetcher.h"
#include "menu.h"
#include "setup.h"

static const char *VERSION        = "1.0.0";
static const char *DESCRIPTION    = "TV Spielfilm Programmdaten (RSS) im OSD";
static const char *MAINMENUENTRY  = "TV Spielfilm";

class cPluginTvspielfilm : public cPlugin {
private:
  cTvsfFetcher *fetcher;
public:
  cPluginTvspielfilm(void);
  virtual ~cPluginTvspielfilm();
  virtual const char *Version(void) override { return VERSION; }
  virtual const char *Description(void) override { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void) override;
  virtual bool ProcessArgs(int argc, char *argv[]) override;
  virtual bool Initialize(void) override;
  virtual bool Start(void) override;
  virtual void Stop(void) override;
  virtual const char *MainMenuEntry(void) override { return MAINMENUENTRY; }
  virtual cOsdObject *MainMenuAction(void) override;
  virtual cMenuSetupPage *SetupMenu(void) override;
  virtual bool SetupParse(const char *Name, const char *Value) override;
};

cPluginTvspielfilm::cPluginTvspielfilm(void)
: fetcher(nullptr)
{
}

cPluginTvspielfilm::~cPluginTvspielfilm() {
  delete fetcher;
}

const char *cPluginTvspielfilm::CommandLineHelp(void) {
  return NULL;
}

bool cPluginTvspielfilm::ProcessArgs(int argc, char *argv[]) {
  (void)argc; (void)argv;
  return true;
}

bool cPluginTvspielfilm::Initialize(void) {
  fetcher = new cTvsfFetcher();
  return true;
}

bool cPluginTvspielfilm::Start(void) {
  fetcher->SetRefreshInterval(TvsfSetup.refreshIntervalMinutes * 60);
  for (int i = 0; i < tvsfFeedCount; i++)
    fetcher->SetFeedEnabled(static_cast<eTvsfFeed>(i), TvsfSetup.feedEnabled[i] != 0);
  fetcher->SetSenderWhitelist(TvsfSetup.senderWhitelist);
  fetcher->SetSenderBlacklist(TvsfSetup.senderBlacklist);
  fetcher->Start();
  return true;
}

void cPluginTvspielfilm::Stop(void) {
  // fetcher's destructor calls Cancel(), nothing extra needed here.
}

cOsdObject *cPluginTvspielfilm::MainMenuAction(void) {
  return new cTvsfMenuMain(fetcher);
}

cMenuSetupPage *cPluginTvspielfilm::SetupMenu(void) {
  return new cTvsfMenuSetupPage();
}

bool cPluginTvspielfilm::SetupParse(const char *Name, const char *Value) {
  if (!strcmp(Name, "RefreshIntervalMinutes"))
    TvsfSetup.refreshIntervalMinutes = atoi(Value);
  else if (!strcmp(Name, "SenderWhitelist"))
    strn0cpy(TvsfSetup.senderWhitelist, Value, sizeof(TvsfSetup.senderWhitelist));
  else if (!strcmp(Name, "SenderBlacklist"))
    strn0cpy(TvsfSetup.senderBlacklist, Value, sizeof(TvsfSetup.senderBlacklist));
  else {
    for (int i = 0; i < tvsfFeedCount; i++) {
      char key[64];
      snprintf(key, sizeof(key), "FeedEnabled.%s", TvsfFeeds[i].key);
      if (!strcmp(Name, key)) {
        TvsfSetup.feedEnabled[i] = atoi(Value);
        return true;
      }
    }
    return false;
  }
  return true;
}

VDRPLUGINCREATOR(cPluginTvspielfilm); // Don't touch this!
