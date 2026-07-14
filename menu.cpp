#include "menu.h"
#include <vdr/interface.h>
#include <vdr/i18n.h>

// -------------------------------------------------------------------------
// Detail view
// -------------------------------------------------------------------------

cTvsfMenuDetail::cTvsfMenuDetail(const cTvsfEntry &e)
: cOsdMenu(e.title.c_str())
, entry(e)
{
  SetMenuCategory(mcText);
  // cOsdMenu doesn't do free-form text layout, so we emulate a simple
  // read-only detail page the way other VDR plugins (e.g. epgsearch) do:
  // one non-selectable item per logical line.
  char line[256];
  snprintf(line, sizeof(line), "%s | %s", entry.timeText.c_str(), entry.channel.c_str());
  Add(new cOsdItem(line, osUnknown, false));
  Add(new cOsdItem("", osUnknown, false));

  // naive word-wrap at ~60 chars so long descriptions stay readable
  std::string text = entry.description;
  size_t pos = 0;
  const size_t wrapAt = 60;
  while (pos < text.size()) {
    size_t len = std::min(wrapAt, text.size() - pos);
    if (pos + len < text.size()) {
      size_t lastSpace = text.rfind(' ', pos + len);
      if (lastSpace != std::string::npos && lastSpace > pos)
        len = lastSpace - pos;
    }
    Add(new cOsdItem(text.substr(pos, len).c_str(), osUnknown, false));
    pos += len;
    while (pos < text.size() && text[pos] == ' ')
      pos++;
  }
  if (!entry.genre.empty()) {
    Add(new cOsdItem("", osUnknown, false));
    snprintf(line, sizeof(line), "Kategorie: %s", entry.genre.c_str());
    Add(new cOsdItem(line, osUnknown, false));
  }
}

eOSState cTvsfMenuDetail::ProcessKey(eKeys Key) {
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) {
    switch (Key) {
      case kBack:
      case kOk:
        return osBack;
      default:
        break;
    }
  }
  return state;
}

// -------------------------------------------------------------------------
// Category list
// -------------------------------------------------------------------------

cTvsfMenuList::cTvsfMenuList(eTvsfFeed feed, cTvsfFetcher *fetcher)
: cOsdMenu(TvsfFeeds[feed].label, 6, 20)
, feed(feed)
, fetcher(fetcher)
{
  Fill();
}

void cTvsfMenuList::Fill(void) {
  Clear();
  entries = fetcher->GetEntries(feed);
  if (entries.empty()) {
    Add(new cOsdItem(tr("Keine Daten - noch nicht aktualisiert oder Sender gefiltert"), osUnknown, false));
    return;
  }
  for (const cTvsfEntry &e : entries) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s\t%s\t%s", e.timeText.c_str(), e.channel.c_str(), e.title.c_str());
    Add(new cOsdItem(buf));
  }
}

eOSState cTvsfMenuList::ProcessKey(eKeys Key) {
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) {
    switch (Key) {
      case kOk: {
        int idx = Current();
        if (idx >= 0 && idx < (int)entries.size())
          return AddSubMenu(new cTvsfMenuDetail(entries[idx]));
        return osContinue;
      }
      case kRed: // manual refresh
        fetcher->TriggerRefreshNow();
        return osContinue;
      default:
        break;
    }
  }
  return state;
}

// -------------------------------------------------------------------------
// Main menu
// -------------------------------------------------------------------------

cTvsfMenuMain::cTvsfMenuMain(cTvsfFetcher *fetcher)
: cOsdMenu(tr("TV Spielfilm"))
, fetcher(fetcher)
{
  SetHelp(tr("Aktualisieren"), nullptr, nullptr, nullptr);
  for (int i = 0; i < tvsfFeedCount; i++) {
    if (fetcher->IsFeedEnabled(static_cast<eTvsfFeed>(i)))
      Add(new cOsdItem(TvsfFeeds[i].label));
  }
}

eOSState cTvsfMenuMain::ProcessKey(eKeys Key) {
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) {
    switch (Key) {
      case kOk: {
        int idx = Current();
        if (idx >= 0 && idx < tvsfFeedCount)
          return AddSubMenu(new cTvsfMenuList(static_cast<eTvsfFeed>(idx), fetcher));
        return osContinue;
      }
      case kRed:
        fetcher->TriggerRefreshNow();
        return osContinue;
      default:
        break;
    }
  }
  return state;
}
