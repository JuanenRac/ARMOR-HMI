# ARMOR-HMI - design

ARMOR-HMI is the touch panel of an A.R.M.O.R. installation: a 7-inch screen on a wall that shows the state of the system, lets a person arm, disarm and acknowledge from there, and
is the place where the voice assistant will live. It is two things at once, and the design keeps them apart:

1. **A node of the family.** Like ARMOR-RADAR, ARMOR-SOLAR and ARMOR-ELECTRICAL it keeps its settings and its users in flash, joins the Wi-Fi, has its own web page for the
   settings, can be set up from a phone over Bluetooth, can be updated from its page (with a roll-back if the new firmware does not come up) and tells ARMOR-SERVER that it
   exists. All of that is `ARMOR-COMMON/firmware_base`, synchronised into this repository (`python ../ARMOR-COMMON/tools/sync_firmware_base.py check`).
2. **A client of ARMOR-SERVER**, like the Android app and Studio. It signs in with a user an administrator made for it and asks the server what it needs to draw.

The panel decides nothing. What it shows is what the server says; arming and disarming are the person's, confirmed on the screen, and the server's (it authenticates and authorises
every order exactly as it does for the Android app).

## What is tested where

| Part | Where | How |
| --- | --- | --- |
| The settings (`core/hmi_config.hpp`) | the computer | `tests/test_config.cpp`: defaults, the link to the server, the screen, the voice, the stored document, passwords that never leave |
| What the panel knows of the server (`core/server_view.hpp`) | the computer | `tests/test_view.cpp`: the summary, the state of the link, when data is stale, what is new, the colour of the screen, the night brightness |
| The voice conversation (`core/voice_session.hpp`) | the computer | `tests/test_voice.cpp`: the order of the steps, the time limits, what a touch does |
| The pins of the board (`core/board_s3.hpp`) | the computer | `tests/test_board.cpp` and a `static_assert`: no two functions share a GPIO and none is a pin the chip keeps |
| The words of the screen (`core/screen_text.hpp`) | the computer | `tests/test_board.cpp`: every row in seven languages |
| The web page (`panel/`) | a real browser | `node tools/panel_browser_test.mjs` against `tools/panel_mock.mjs`: every page in seven languages |
| The firmware (`main/`) | **nowhere yet** | it builds with ESP-IDF 5.4 (`tools/build_node.sh generic`) and has never been built or run on a board |

## The link to the server

```
panel                                     ARMOR-SERVER
  | POST /api/v1/studio/session {user, password}        -> 201 + cookie
  | GET  /api/v1/panel/summary  (every poll_s seconds)  -> {mode, nodes{online,total}, alarms{active, unacknowledged, items[<=6]}}
  | POST /api/v1/mode {"mode":"armed"|"disarmed"}       (after the person confirms on the screen)
  | POST /api/v1/alarms/acknowledge                      (the button "Acknowledge all")
```

`GET /api/v1/panel/summary` was added for small screens (ARMOR-SERVER 0.3.7): a few hundred bytes whatever the installation holds, with the alarms nobody has acknowledged first.
The panel never asks for the whole state: a 64 KB limit of its JSON reader would make a large installation impossible to read.

The state of the link is a small machine (`LinkTracker`): *not set up*, *no network*, *connecting*, *online*, *stale* (four polls without an answer: what is on the screen is old, and
the button to arm or disarm is disabled so that nobody acts on old news) and *denied* (the server refused the login twice in a row, or answered 403: someone has to fix the user in the
settings). After a failure the panel waits longer each time (up to a minute), so that a server that is down is not asked every three seconds.

The panel also publishes `armor/node/<id>/health` and, now and then, `armor/node/<id>/info` (its name, firmware and the address of its page) over MQTT, so that Studio lists it with a
link to its web page like any other node.

## The screen

800 by 480, drawn with LVGL 9: the mode of the system in the colour of the worst alarm nobody has seen, a large button to arm or disarm (with a question first), the number of nodes
online, the alarms (the unseen first, six at most) with a button to acknowledge them all, the microphone and a line at the bottom that says what is wrong (no server in the settings,
the server not answering, or how to set the panel up). The night hours dim it, a period without a touch puts it to sleep, and a touch or a new alarm wakes it; a new alarm also plays a
tone. The words are in seven languages (`core/screen_text.hpp`); Japanese and Chinese are drawn with fonts generated for exactly the characters the texts use (see [FONTS](FONTS.md)).

## What is not here

The wake word ("armor"), the speech service the assistant talks to, a view of the cameras on the screen, and a first compilation and first power-up of the firmware. See
[VOICE](VOICE.md) and [HARDWARE](HARDWARE.md).
