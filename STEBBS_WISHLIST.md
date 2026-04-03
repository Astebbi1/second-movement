# Stebbs Watch Face Wishlist

## In Progress
- [ ] **Step counter sensitivity tuning** — adjusted thresholds (RAW 24→31, N_ACTIVE 5→7). Needs real-world validation. Step-dependent faces should wait until this is reliable.

## Existing Face Improvements
- [x] **nxtup_face** — scroll starts on activation with leading spaces; speed doubled to 4Hz.

## Approved to Build
- [ ] **awake_face redesign** — rip out step-count-based sleep detection, replace with: (1) 20-min background polls checking accelerometer stillness + chip temperature; (2) 90-min consecutive still+cool polls = sleep confirmed, backdated to when stillness began; (3) temp recovery + motion = awake confirmed; (4) hard tap = "still awake" dead man's switch, resets countdown; (5) prolonged perfect stillness + cool temp = watch off wrist, excluded from awake time entirely. Target accuracy: ~35 min. Shows rolling 24h sleep total and time awake since.
- [ ] **Chess clock** — two-player, button press switches whose time is counting. Done well.
- [ ] **Active life countdown** — estimated minutes of life remaining, actively decrementing every 60 seconds. Configurable for waking hours only.
- [ ] **Memento mori %** — % of avg lifespan elapsed, slow creep upward.
- [ ] **Rhythm tap recognition** — system-level gesture vocabulary using accelerometer tap interrupts. Fuzzy-match timing ratios against known patterns (rimshot, etc.). Any face can opt in.

---

## Face Ideas

### Celestial / Outdoors
- [ ] **Golden hour face** — time until next golden hour, how long it lasts today, whether you're currently in it. Also covers blue hour (sun just below horizon). Built on the same solar math as sunrise_sunset_face. Duration varies significantly by season and latitude which makes it genuinely interesting. Good for photography or just knowing.
- [ ] **Solunar tables** — best hunting/fishing windows based on moon position. Legitimate tool outdoor people pay for apps to get.
- [ ] **Tide predictor** — rough estimate from moon phase + hardcoded location.
- [ ] **Planetary visibility** — which planets are visible tonight. Pure orbital math.
- [ ] **Meteor shower calendar** — countdown to next known annual shower.

### Time / Life / Philosophy
- [ ] **Active life countdown** — shows estimated minutes of life remaining, actively decrementing every 60 seconds. Visceral in a way a percentage isn't. Configurable for waking hours only (subtracts sleep). The point is watching it tick.
- [ ] **Memento mori %** — slower burn version: % of avg lifespan elapsed, creeping up.
- [ ] **Smoking savings tracker** — enter quit date + daily savings ($), shows running dollar total and days smoke-free. Basically days_one_face with multiplication.
- [ ] **Age in days** — companion to days_one_face, counts up instead.

### Activity Modes *(after steps are reliable)*
- [ ] **Auto-detect activity state** — watch detects sustained running cadence and surfaces a suite of relevant faces automatically. No button press to enter run mode.
- [ ] **Running pace face** — real-time pace (min/mile or min/km) from step cadence + configurable stride length.
- [ ] **Run summary face** — elapsed time, steps, estimated distance, pace. Part of the activity mode cluster.
- [ ] **Sedentary / inactivity alert** — flip side: no meaningful movement for X minutes → LED nudge.

### Movement-Gated Interactions
- [ ] **Move-to-dismiss alarm** — alarm only clears after sustained shaking for 3+ seconds. Forces actual waking up.
- [ ] **Rest timer** — detects stillness via accelerometer, starts counting automatically. No button press. Useful between lifting sets.

### Data / Memory Faces *(hold a small piece of info)*
- [ ] **Named counter** — like tally_face but stores a label ("PAGE", "PUSH", "DRINK"). Persistent, one number.
- [ ] **Last-seen tracker** — stores a timestamp on button press, displays time elapsed since. "Last drank water: 2h ago." Covers a lot of quick yes/no glance use cases.
- [ ] **Streak tracker** — date-aware tally. Resets if you miss a day, tracks longest streak. The streak number is the point.
- [ ] **Practice session logger** — log sessions with a button press, accumulates total hours. Good for guitar, studying, anything you want to track over weeks/months.

### Camera / Photography
- [ ] **ND filter exposure calculator** — enter base shutter speed, select ND strength (stops), get corrected exposure. Pure math, useful at a camera.
- [ ] **Long exposure / timelapse timer** — set a duration, LED signals when done. Good for bulb mode or astro.

### Workout / Sports
- [ ] **Chess clock** — two-player, button press switches whose time is counting. Want this done well.
- [ ] **Lap counter with splits** — button press each lap, stores split times.
- [ ] **HIIT timer** — alternating work/rest intervals with round counter. Distinct from interval_face in that it has two different durations and counts rounds to completion.
- [ ] **Cadence metronome** — LED flashes at target cadence (e.g. 180/min) to train running form.
- [ ] **Pomodoro** — 25-min work / 5-min break alternating, counts completed rounds, longer break every 4. Structure matters vs. plain interval timer.

### Rhythm / Gesture Input
- [ ] **Rhythm tap recognition** — record inter-tap timing ratios from accelerometer tap interrupts and fuzzy-match against known patterns. E.g. tap a rimshot or "hot cross buns" rhythm to trigger an action on the current face (log a shower, increment a counter, start a timer). The rhythm ratio stays constant even when tapped at different speeds. Best designed as a system-level gesture vocabulary any face can opt into, rather than one-off per face. Main challenges: false positives during normal wear (probably gate it to active face only), and accelerometer needs to stay on while listening (small battery cost).

### Quick Glance / Decision
- [ ] **Hydration checker** — shows time since last "drank water" button press. Displays urgently if >1h.
- [ ] **Availability indicator** — configure your usual schedule, shows FREE or BUSY by time of day.

### Not Doing
- Golden hour, solunar, tide, planetary visibility, meteor shower, ND filter, long exposure, walking distance, named counter, last-seen tracker, streak tracker, practice logger, habit tracker, sleep log, HIIT timer, lap counter, cadence metronome, pomodoro, hydration checker, availability indicator, UV index, solstice countdown, reaction time tester, smoking savings, age in days, sedentary nudge, rest timer, run mode/pace/summary (revisit after steps reliable)
