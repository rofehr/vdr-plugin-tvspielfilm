#ifndef __TVSPIELFILM_MENU_H
#define __TVSPIELFILM_MENU_H

#include <vdr/osdbase.h>
#include "rssfetcher.h"

// Detail view for a single program entry.
class cTvsfMenuDetail : public cOsdMenu {
private:
  cTvsfEntry entry;
public:
  cTvsfMenuDetail(const cTvsfEntry &e);
  eOSState ProcessKey(eKeys Key) override;
};

// List of entries for one feed/category.
class cTvsfMenuList : public cOsdMenu {
private:
  eTvsfFeed feed;
  cTvsfFetcher *fetcher;
  cTvsfEntryList entries;
  void Fill(void);
public:
  cTvsfMenuList(eTvsfFeed feed, cTvsfFetcher *fetcher);
  eOSState ProcessKey(eKeys Key) override;
};

// Top-level plugin menu: one entry per feed/category.
class cTvsfMenuMain : public cOsdMenu {
private:
  cTvsfFetcher *fetcher;
public:
  cTvsfMenuMain(cTvsfFetcher *fetcher);
  eOSState ProcessKey(eKeys Key) override;
};

#endif // __TVSPIELFILM_MENU_H
