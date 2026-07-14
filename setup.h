#ifndef __TVSPIELFILM_SETUP_H
#define __TVSPIELFILM_SETUP_H

#include <vdr/menuitems.h>
#include "rssfetcher.h"

struct TvsfSetupData {
  int refreshIntervalMinutes = 5;
  int feedEnabled[tvsfFeedCount] = { 1, 1, 1, 1, 1 };
  char senderWhitelist[256] = "";
  char senderBlacklist[256] = "";
};

extern TvsfSetupData TvsfSetup;

class cTvsfMenuSetupPage : public cMenuSetupPage {
private:
  TvsfSetupData data;
protected:
  void Store(void) override;
public:
  cTvsfMenuSetupPage(void);
};

#endif // __TVSPIELFILM_SETUP_H
