# vdr-plugin-tvspielfilm

VDR-Plugin, das TV-Spielfilm-Programmdaten (Sender/Sendungen, Tipps, Highlights)
per RSS abruft und in einem eigenen OSD-Menü darstellt — als Ergänzung zur
DVB-EPG, angelehnt an die Funktionalität der TV SPIELFILM App
(https://play.google.com/store/apps/details?id=de.tvspielfilm).

## Warum kein "Portieren" der Android-App?

Die App ist eine kompilierte Kotlin/Java-APK mit eigenem UI-Toolkit und
(vermutlich) internem/privatem Backend. Ein 1:1-Port nach C++/VDR ist weder
technisch möglich noch sinnvoll. TV Spielfilm bietet außerdem keine
offizielle öffentliche API. Was es gibt: **öffentliche RSS-Feeds**, die die
Webseite seit Jahren anbietet und die auch von Drittprojekten (z. B. der
ioBroker-Adapter `iobroker-community-adapters/ioBroker.tvspielfilm`, oder
smartVISU) genutzt werden. Dieses Plugin baut auf denselben Feeds auf.

## Datenquelle: RSS-Feeds

Basis-URL: `https://www.tvspielfilm.de/tv-programm/rss/<feed>.xml`

| Feed-Key     | Datei           | Inhalt (App-Pendant)                 |
|--------------|-----------------|---------------------------------------|
| jetzt        | jetzt.xml       | "Jetzt im TV" (laufende Sendungen)     |
| tipps        | tipps.xml       | Redaktionelle TV-Tipps                 |
| highlights   | filme.xml       | Spielfilm-Highlights                   |
| heute2015    | heute2015.xml   | Prime-Time-Programm 20:15              |
| heute2200    | heute2200.xml   | Spätprogramm 22:00                     |

Jeder `<item>` enthält typischerweise:
```xml
<item>
  <title>20:15 | Das Erste | Der Saarland-Krimi: Bruder, Liebe, Tod</title>
  <description>Deutschlands kleinstes Bundesland bekommt einen eigenen ...</description>
  <link>https://www.tvspielfilm.de/tv-programm/sendung/...html</link>
  <pubDate>Thu, 09 Jul 2026 20:15:00 +0200</pubDate>
  <enclosure url="https://a2.tvspielfilm.de/itv/.../xxx_149.jpg" type="image/jpeg"/>
</item>
```
`title` kodiert Uhrzeit, Sendername und Sendungstitel getrennt durch ` | `.

**Bekannte Einschränkung:** Es gibt kein separates Genre-Feld. Genre-artige
Gruppierung im Plugin (`ClassifyGenre()` in `rssfetcher.cpp`) ist eine simple
Stichwort-Heuristik auf Titel/Beschreibung — kein Ersatz für die redaktionelle
Genre-Zuordnung der App. Für persönliche Senderlisten böte sich später
`my.tvspielfilm.de` (Login-Feed) an, das ist hier bewusst nicht umgesetzt
(Login-Handling, Cookies, ToS-Fragen — erstmal nicht Teil des Grundgerüsts).

## Architektur

- `rssfetcher.{h,cpp}` — libcurl-GET pro Feed + schlanker handgeschriebener
  RSS-Parser (kein Expat/TinyXML als zusätzliche BitBake-Dependency nötig,
  da das Feed-Format sehr regelmäßig ist). Läuft als `cThread`, Refresh-
  Intervall konfigurierbar (Default 5 min, wie beim ioBroker-Adapter).
  Datenzugriff über Mutex-geschütztes Snapshot-Interface.
- `menu.{h,cpp}` — `cOsdMenu`-Hauptmenü mit einem Eintrag pro Feed-Kategorie,
  Untermenü listet Sendungen (Zeit | Sender | Titel), OK öffnet Detailansicht
  mit Beschreibung.
- `setup.{h,cpp}` — `cMenuSetupPage`: Refresh-Intervall, Sender-Whitelist/
  -Blacklist (kommagetrennt, wie im ioBroker-Adapter), Feed-Auswahl an/aus.
- `tvspielfilm.c` — Plugin-Einstiegspunkt (`cPluginTvspielfilm`), Start/Stop
  des Fetcher-Threads, `MainMenuAction()`.

## Build

Wie jedes VDR-Plugin: Quelltext nach `VDR/PLUGINS/src/tvspielfilm` (Symlink
oder Kopie), dann aus dem VDR-Wurzelverzeichnis:

```
make plugins
```

Abhängigkeit: `libcurl` (curl-dev). Für BitBake/Yocto:

```bitbake
DEPENDS += "curl vdr"
RDEPENDS:${PN} += "libcurl"
```

## Nächste sinnvolle Ausbaustufen

1. Sender-Mapping: `title`-Sendername auf VDR-`cChannel` matchen, damit man
   aus dem Detailmenü direkt umschalten oder einen Timer setzen kann
   (SVDRP `NEWT`, analog zu deiner XMLTV→SVDRP-Pipeline).
   Sender-Namen aus dem Feed sind teils uneinheitlich (z. B. "SuperRTL" vs.
   "Super RTL") — Fuzzy-Matching o. ä. nötig, hier bewusst nicht enthalten.
2. Bild-Caching der `enclosure`-URLs für die OSD-Vorschau.
3. Persönliche Senderliste via `my.tvspielfilm.de` (Login-Feed).
4. Genre-Filter/-Sortierung verbessern, sobald echte Kategoriedaten
   verfügbar sind.
