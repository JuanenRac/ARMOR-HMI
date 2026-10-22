# Changelog

All notable changes to this project are documented here.

## [Unreleased]

- The repository has a security policy, a code of conduct, a contributing guide and a support file.
- The banner follows the animated style of the ecosystem banners, with the name, the kind and the technologies of the project.
## [0.3.7] - Wrong passwords per minute as a setting, the broker certificate from a file and a browser test for any Chromium

- **Fixed: a node with stored settings no longer restarts in a loop at power-up.** The start-up code read the stored settings and the copy made before the last update on the stack of the main task (about 3 KB each, plus the reader's own copy); with settings stored the 16 KB stack overflowed and the node restarted again and again before it could start the network. They now live on the heap.
- **The limit on wrong passwords per minute is a setting of the node:** `system.login_limit_per_minute` (30 as always, 0 for none, at most 120) with its field and its explanation in the panel in the eight languages; the limit of each address and of each person is not touched by it.
- **The certificate of the broker can be loaded from a file** on the Broker page (it is checked for the BEGIN and END lines, a private key is refused, and it must fit the 2048 characters the node keeps); nothing is sent to the node until the page is saved.
- **New checks in the host tests:** the Bluetooth session stays under its lock and a request or an answer never crosses to another connection (`check_ble_session.py`); the routes every node serves, the routes the panel calls and the stand-in node the panel is tried against agree (`check_routes.py`); the guard on the paths of a save also keeps the tolerant read of the stored settings, the serial set-up that makes the Wi-Fi the way in, and the answer to a release without an image for the board.
- **The universal image holds no fleet secret unless asked for:** `tools/build_node.sh generic` adds `secrets/generic.conf` only with `ARMOR_WITH_FLEET_SECRET=1`, names that image `*-local` and says it is never to be published, and a public build is refused when the configuration it ended with holds a secret or a set-up code; the option's help says the same.
- **The panel test finds any Chromium-based browser** (`ARMOR_BROWSER`, or the usual places of Windows, macOS and Linux; with none installed it says it was skipped) and goes through the eight languages the panel offers; the stand-in panel has the fields it lacked (`audio.min_severity`, `voice.token_set`, `server.cert_pem`).
- **The shared Kconfig and the stand-in panel name the touch panel's board correctly** (`lcd7box`); the speech service address suggested in the panel and the tests is the port 18090 where the service listens.

## [0.3.6] - Node diagnostics

- **The graphics memory pool lives in the C allocator** (`CONFIG_LV_USE_CLIB_MALLOC`), so large blocks go to the external RAM; the line that asked for it before named a setting that LVGL 9 does not have, and the 64 KB pool inside the internal RAM was the one in use.
- **ESP-IDF 5.5.5 is the one version:** `tools/build_node.sh` uses the 5.5.5 container, and the firmware guide, the requirements of the READMEs and the manifest name it (it said 5.4 in places).
- **How the last run ended:** after a start that was not clean (a crash, a watchdog, a sag of the supply, a failed allocation, a restart by the node's own guard) the health report carries the cause for the first ten minutes, so nobody has to be at the board with a cable.
- **The memory gate and the core dump in the health report:** whether new panel connections are being refused now and whether a core dump is waiting.
- **Crash report in the panel:** a crash writes its core dump (ELF, with a checksum, up to 32 tasks) to the `coredump` partition (128 KB in the table of the repository); the Update page has a *Crash report (core dump)* card for administrators to look at it, download it or erase it. A node flashed before with the older 64 KB table keeps it and may not fit a long dump.
- **Settings that survive an update:** right before a firmware is installed the node keeps a copy of the stored settings next to them, with the version that wrote it and a check number, and reads both with the tolerant reader at every start.
- **Diagnostic package and log on request:** an administrator can ask the node, through the server, for its `diagnostics` with `detail` `package` or `log` (the same two steps and one-time token as the other orders); the answer carries the memory as it is now and the tasks with the least stack they have had free.

## [0.3.5] - The node protects its memory, and the server can control it

- The checksum and the image are read again from GitHub: the HTTP request buffer is 4 KB, because the request that follows GitHub's redirect carries an address of about a thousand characters.
- **The node protects itself when its internal memory runs short:** the tasks it cannot do without (the broker link, the health report and the node's own work) are made at start-up and Bluetooth is started last; a guard looks for them and for the internal memory and, after a minute of trouble, says it (log and an immediate health report) and restarts the node once, cleanly - never more than three times an hour, then it stays up and repeats the alarm.
- **Bluetooth uses less internal memory and closes by itself:** the Bluetooth host stack is allocated in the external RAM and the controller is limited to the three activities it uses; in mode `setup` it switches off after 10 minutes (`CONFIG_ARMOR_BLE_SETUP_MINUTES`) with nobody connected, or two minutes after a node that has users gets an address, and frees its memory; it is not started when less than 40 KB of internal memory is free nor after a restart the node asked for itself.
- **New panel connections are refused while the internal memory is short** (under 28 KB free or a 12 KB biggest block, open again over 40 KB and 16 KB), before anything is allocated for them; the broker link and the node's work are not touched.
- **A failed allocation restarts the node cleanly once** and the next start reports `oom heap=<free>/<largest>`; during a firmware write, or twice within a minute, it aborts as before. A node that hands nothing to a connected broker for ten minutes restarts through the same limit of three an hour, then stays up, degraded.
- **Last will and birth, retained:** the node says `online` true on `armor/health/<node>` each time it connects and the broker keeps its last will (`online` false) there, so a server that starts later knows the state of every node (the radar node does the same on its presence topic).
- **Memory log and the figures of the health report:** the internal memory, the external RAM and the biggest block (see the Memory section of the node's firmware guide), with the stack of the telemetry task kept at 6144 bytes.
- **Smaller fixes:** the MQTT client of the radar node no longer restarts the node when it cannot be created (it tries again), the report of a sensor's firmware and mode is written under the same lock it is read with, the Improv page says `provisioned` only after the Wi-Fi network was joined, and `tools/check_sdkconfig.py` compares a build with `sdkconfig.defaults`.
- **The PIN does not stay on the screen:** an unfinished confirmation closes after 30 seconds and when the screen sleeps, and the request that carried the PIN is overwritten after it is sent.
- **The server can control the node:** restart, pause, resume, standby (kept across restarts, the node stays connected), identify, diagnostics and maintenance on and off, on `armor/control/<node>/command`, with a one-time token bound to the operation; there is no factory reset, no erase and no deep sleep. While the panel is paused or in standby the screen shows a screen of its own in the eight languages, and identify blinks a bar at the top; the fonts were generated again for the new texts. Rules and host tests are shared with the other nodes; nothing of it has run on a board.
- **Planned maintenance:** before the node goes quiet on purpose (a restart, a pause, a standby, a firmware install or copy, a panel upload) it publishes a retained `armor/control/<node>/maintenance` message (`node_maintenance`: active, why and for how long) so that the server holds back the node-offline alert for a bounded time, and clears it (retained too) when it is back; a node that boots into standby keeps saying so until it is resumed, and a window that is never closed ends by itself. The pure parts are in `control_gate.hpp` with host tests; nothing of this has run on a board.
- **The guard and the server's orders work together:** a node the server paused or put in standby is not restarted by the guard for a task it stopped on purpose (nor one with a restart order on its way; the memory rules stay in force), a restart that was ordered leaves a mark so the next start tells it from a restart of the guard and does not count it against the three an hour, the standby kept in flash survives a restart of the guard, and the planned-maintenance signal goes out before an ordered restart, an install and an upload.
- **Fixed:** the log line that closes the Bluetooth set-up window passed a text chosen at run time as the format of the message, which the logging macro does not accept at build time.

## [0.3.4] - Update from GitHub through long redirects, the server's certificate pinned by fingerprint and the green frames of the Summary

- **The panel checks the server's certificate by its fingerprint:** the option to accept any certificate is gone; the SHA-256 fingerprint of the server's certificate is read from the server, compared by a person and saved, and the panel connects only to a server that shows that certificate (or one vouched for by a public authority).
- **One lock for the update:** two web threads and a timer can no longer start two installations or two restarts; the PIN entry is overwritten when it is cleared; the access point is opened only when there is neither a station nor a cable, and a panel with no address after 90 seconds opens the set-up access point again.
- **Update from GitHub works again when GitHub answers with a long redirect:** the checksum file is fetched through a redirect whose headers add up to more than 5 KB, and the 4 KB header buffer made the HTTP client give up. The buffer is now 20 KB, which the memory settings place in the external RAM (above 16 KB), so it costs no internal memory; a board with no external RAM falls back to 6 KB. The log says how much internal memory was free before and after the client was opened.
- **The real reason is told:** when the checksum or the signature of a release cannot be read, the panel says it was the connection, the size or the HTTP status (new texts in the eight languages), and "the release carries no checksum" is only said for a release that really has none.
- **The Summary draws every value inside a green frame,** like the green label of a port that reports, in every node panel (one shared style rule and one helper).
- **The help of the panel is brought up to date in the eight languages:** the limit of 30 failed sign-ins per minute in total, the tolerant reading of the saved settings after an update, the update from GitHub and the notice that a release has no image for the board, Bluetooth set-up (the derived PIN and the answer only to the phone that asked), the web modes and the certificate fingerprint, and the clock and its time server.
- **The node announces that it is gone:** the connection to the broker carries a last will that tells the server the node is offline, and the server never counts it as a report.
- **No more aborts on a failed initialisation:** the network, the stored settings and the Ethernet start-up degrade with a log line instead of rebooting in a loop.
- **Fewer data races:** the cached lengths of the firmware slots, the timer that restarts the node and the flag that the restart after an update was scheduled are guarded.
- **Tests:** the header buffer against a simulated redirect of 5 KB, the reasons for a checksum or signature that cannot be read (and that "no checksum" is not one of them), the green frames in the Summary, and the words every help text must contain in the eight languages.

## [0.3.3] - Tolerant settings, global login limit, Bluetooth race, missing board image

- Settings kept in flash are read as far as they can be after an update: unknown members are ignored and a value that cannot be accepted falls back alone, so the node never starts over because of one field.
- The panel login limits the wrong passwords of all sources together to thirty per minute.
- Bluetooth answers go only to the phone that asked; waiting requests are discarded on connect and disconnect.
- A newer release without an image for this board is reported in the panel (8 languages) instead of passing for up to date.

## [0.3.2] - Shared node base update

- Shared base update: a wider login table, slot-sized download limit, stricter JSON text and a set-up code kept in capitals.
- An alarm that drops out of the short list of the summary and comes back (when others are acknowledged) no longer rings again: the panel remembers the last sixty-four ids it has seen.
- Saving the settings takes a lock, so two saves at once (HTTP and HTTPS) cannot undo each other; the comment of the restart rule says which settings need one; the launchers name this project.

## [0.3.1] - Stored passwords follow their host and a silent alarm after the sixth

- A stored password is tied to its host: changing the broker address (or a backup slot), the server host or the voice service address without typing a new secret erases the stored password or token instead of sending it to the new host, and the panel says under the field that it has to be typed again. The same host keeps it.
- A duress alarm that comes after the sixth line is still left out of the counts, so the screen no longer turns to *check* because of an alarm that must never be shown.
- The revision and the time of the server summary are range-checked before they are converted to whole numbers.
- The access point bandwidth accepts only 20 or 40 MHz (a value in between was accepted and run as 20); an address in the list of networks that may use the panel is refused when a part has a leading zero, as the address parser of the network settings already did.

## [0.3.0] - Sign-in form for password managers

- The sign-in page is a real form with named fields (`username`, `current-password`), so the browser's or system's password manager offers to save and fill the credentials; the set-up page names its fields the same way for the new password.
- A new tick, "Remember my user name on this device", keeps only the user name in the browser (off by default, forgotten as soon as it is unticked, and the panel works with storage blocked); the password is never stored by the panel.
- A short note under the form, in the eight languages, says that the password is saved only by the browser's own password manager. The existing "keep me signed in" tick is unchanged.

## [0.2.9] - Health report

- Every node publishes a health report about once a minute to armor/health/<node> (the node_health contract): uptime, why it last restarted, free and least free heap, the link and Wi-Fi signal, how many times it connected to the broker, tasks short of stack and the last error it logged.
- The report is built by core/node_health.hpp and main/node_health.cpp, shared by the five nodes through ARMOR-COMMON's firmware base; its decisions are covered by the host tests of tests/test_base.cpp. It has not run on a board.
- The broker link counts its connections for the report.

## [0.2.8] - An optional encrypted link to the broker (TLS)

- New settings mqtt.tls (off by default) and mqtt.ca_pem (one certificate in PEM, at most 2 KB; anything else is refused). With TLS on, the node connects with mqtts://, trusts only that authority and checks that the broker's certificate was made for the address it reached; it waits for its clock first (up to two minutes), because certificates have dates. With the switch off nothing changes: the node talks plain MQTT as before.
- The Broker page of the panel has a switch and a box for the certificate of the installation's own authority, in the eight languages, with the new error texts. The saved backup brokers must be mqtts:// too while the switch is on.
- Host tests of the settings: defaults off, the JSON round trip, line ends from a browser, and the refusal of a text that is not PEM, a chain, a text that is too long and a plain address with TLS on. The handshake has not been run on a board.

## [0.2.7] - The screen knows every alarm the server raises, and says that it is a wall panel

- The screen text has the alarm codes that the server raises and the screen did not know (network, solar, electrical, alarm zones, glass break, vibration...), in eight languages, so the panel no longer shows a generic Alarm for them; the fonts of Japanese and Chinese were generated again with the new characters.
- The health message carries kind panel, so the server lists the panel as a wall panel and does not count it as a radar or call its silence a node down.

## [0.2.6] - Only the house may use the panel, wrong passwords counted per person, and a panel that works behind the server's proxy

- **The panel answers only the house.** A new setting, *Networks that may use this panel* (`web.admin_sources`), says which addresses may sign in or change anything: empty means the private ranges of a house (10/8, 172.16/12, 192.168/16, link-local, loopback and 100.64/10, where Tailscale lives), otherwise the list of addresses and networks you write (`192.168.1.0/24, 10.66.0.0/24`). Any other address is told `outside_network`, except the public look at the node's identity, so a port opened by mistake on the router does not let the Internet try passwords here. A list that would shut out the person who is saving it is refused (`would_lock_out`).
- **Wrong passwords are counted for the person too**, not only for the address: five wrong passwords for a name slow that name down from any address (and, behind the server's proxy, where every request has the server's address, one mistyped name no longer stops everybody).
- **The panel works behind the server's proxy:** its page, its style, its script, its calls and the firmware upload use relative paths, so the same panel opens at `http://node/` and at `https://house/api/v1/nodes/<id>/panel/` (the Bluetooth and map sockets keep their own paths).
- **The fingerprint of the node's certificate is public:** `GET /api/v1/session` carries `cert_sha256`, which is what the server pins before it opens the panel of an HTTPS node; the panel already shows it in the web card so that it can be compared by hand.
- The new setting, its hint and the two new refusals are in the panel in the eight languages. Tests: 1,836 checks in the host test programs (new ones cover the list of networks, the key of a person and the setting).
- **A login or a set-up must carry the marker of the panel** (`X-Requested-With`), like every request that changes something, so a page of another site cannot sign a person in to an account it chose; the server's own sign-in to a node (update, adoption) sends it.
- **The current password of the account is counted too:** wrong ones in "change my password" are slowed down like those of a login, because a stolen session could repeat them.
- **A user change is checked before it is applied:** a role is no longer changed and then the new password refused.

## [0.2.5] - A duress alarm that stays silent, old news that cannot be acted on and fonts that have every glyph

- **The alarm of a duress PIN is never shown, sounded or counted by the panel.** A person forced to disarm types the duress PIN and the server raises a critical alarm in silence; the panel used to turn red and sound its siren in front of whoever forced them. The panel now leaves those alarms out of the screen and of the counts, whatever the server sends.
- **Old news is not acted on:** with no network, with a refused login or with a summary that stopped coming, the screen shows what it knew dimmed, in grey, and the button to arm or disarm gives no order; before, only a stale link did, and with the Wi-Fi gone the screen stayed green *ARMED* for hours.
- **An order that could not be sent is dropped after ten seconds** with its PIN, and the screen says it was not done; before, it waited for the link to come back and could disarm the house hours later. Two orders can no longer be given at once.
- **The screen says why an order failed** (no answer from the server, refused, not possible) in the eight languages, as it already did for the PIN.
- **The first touch on a sleeping screen only wakes it** (it used to press the button under the finger, in the dark).
- **The fonts have every glyph the screen draws.** The text added after the fonts were made (the PIN messages, the energy, the French *œ*, the dash that shows before the first answer, the quotes of the Chinese voice text) was drawn as empty squares in Japanese and Chinese; the tool that makes the fonts had stopped finding the texts since Portuguese was added. It is fixed, the fonts are made again, and a test fails when a character of the screen has no glyph.
- **The assistant can read a spoken answer.** The audio of the answer made the document longer than the 64 KB the JSON reader accepts, so almost every reply with sound failed; the audio is now taken out of the text before it is read, an answer that is too long is refused whole instead of cut, and the listening time stops at 15 seconds (what the service accepts). The idle line says *touch the microphone* (there is no wake word).
- The setting for the speech service suggests port 18090, the one the service really uses.
- The text and the documents recommend a user with the role *Panel* for this device (it can read the summary, arm, disarm and acknowledge and nothing else), not an operator.
- The task list of the panel and the early warning of a task running out of stack work (the kernel's trace facility was not on), and a crash leaves its core dump in the partition.
- A user change that could not be written is told as a storage error; HTTP answers 408 and 423 with their own status lines; the panel tells every error the node can answer with in the eight languages (a test fails when a new one has no text); the Bluetooth note no longer calls the pairing *just works*.
- The documents say what the panel does today: how the firmware has been built, the Bluetooth pairing, the voice service and its limits, and how the stale state works. `secrets/node.conf.example` no longer carries a usable set-up code.
- Tests: 1,759 checks in seven programs plus the panel texts and the glyphs of the fonts; new ones cover the duress alarm, old news without a network, the long answer of the speech service and the texts of failures.
- Shared with the other nodes (from ARMOR-COMMON's firmware base): updates are signed for the project, the board, the version and the hash, the pairing of Bluetooth closes after repeated failures, the random generator keeps away from the converter, a user write is all-or-nothing, a repeated key in a JSON document is refused, four-number firmware versions are understood, Improv over USB also tells the node to use the Wi-Fi it was given, and the upload by hand is refused while an installation runs.

## [0.2.4] - The Bluetooth PIN in the panel

- **Signed firmware:** a node installs from GitHub only a firmware whose signature verifies. Each release carries, next to the image and its SHA-256, an Ed25519 signature of that hash (`<image>.sig`); the node downloads the image, hashes it and installs it only when the hash is the published one and the signature verifies with the public key of the releases, which is in the firmware (the private key never leaves the maintainer's computer). A release without a signature, or with a wrong one, is refused and nothing changes.
- An administrator can still upload a firmware file by hand in the panel, as before: that is the administrator's own act and is the way out if a key ever has to change.
- The panel tells the two new refusals (no signature, bad signature) in the eight languages.
- The verification (SHA-512 and Ed25519, with their constants worked out by a script instead of typed) is tested on a computer against signatures made by another implementation, with every kind of damage to the message, the signature and the key.
- An administrator can read the Bluetooth PIN in the panel (Bluetooth card, *Show the Bluetooth PIN*), without going to the USB console: it is asked of the node only then and forgotten when hidden; new route GET /api/v1/ble-pin, administrators only; the texts are in the eight languages.

## [0.2.3] - A PIN for Bluetooth pairing

- Bluetooth pairing now asks for a 6-digit PIN that the node prints on its USB console next to the setup code; the link is authenticated and nothing is accepted over Bluetooth without it (until now any phone in range could pair without a PIN).
- The Bluetooth note of the panel explains the PIN in all eight languages.

## [0.2.2] - Portuguese on the screen and in the panel, a sound for each severity and a warning for the access point and the station at once

- The panel speaks Portuguese, the eighth language, in every page, hint and message of the settings.
- The overview warns when the node's own Wi-Fi network and its connection to your router are on at the same time (they share one radio and one channel), and the Wi-Fi help explains what that costs and when to switch the access point off.
- The screen is available in Portuguese, with every sentence and alarm text.
- A new alarm sounds by its severity: a notice for a warning, the rising pattern for a high alarm and a wailing siren for a critical one, the most serious of the new alarms deciding.
- A new setting, "Sounds for alarms from", chooses the least serious alarm that makes a sound (every alarm, high and critical only, or critical only); the screen still shows the quieter ones.

## [0.2.1] - Slot copy, pinned server certificate, token for the voice service and a retried action after a lost session

- A firmware slot can be copied to the other one from the panel, and the update of a node waits for the image to be confirmed only after the node has an address or its own access point is reachable, rolling back after five minutes otherwise.
- The station connection that was paused to leave the access point free retries after ten minutes at most instead of waiting forever.
- The firmware check against GitHub runs in the background and the panel asks for its result, so a slow search no longer freezes the panel.
- Saving the settings is serialised, so two browsers saving at once cannot mix their changes.
- Logging out needs the page's own token, which stops another page from logging the panel out.
- The server's certificate can be pinned in the settings: the connection is accepted only if the server presents a chain that ends at it, which is safer than accepting any certificate for a self-signed server.
- The voice service's own token is sent with every request and is kept as a secret.
- An arm, disarm or acknowledge that meets an expired session signs in again and repeats once instead of failing.

## [0.2.0] - Copy-log icon and copy of a firmware slot to the other

- **Copy one firmware slot to the other:** next to the switch between ota_0 and ota_1, the Firmware card of the panel has a button that copies the firmware that runs over the other slot (task of its own, with a percentage, the image checked at the end, no restart, no change of the boot slot); `POST /api/v1/ota/copy` starts it and `GET /api/v1/ota/copy` tells how far it is. The slot sizes shown by the status are measured again after a copy or an update.
- **Node panel:** the log of the panel has a small copy icon (two sheets) that puts the whole log on the clipboard and says "Copied" for a moment; the same panel code is shared by all the nodes.

## [0.1.9] - The search on GitHub finds memory for its connection

- Looking for a new version on GitHub no longer fails with connect_failed when the node was short of internal memory (a browser on the HTTPS panel at the same moment): the TLS buffers now live in the external memory, and the search and the download try three times.

## [0.1.8] - Hardening of the web server, the broker link and the sessions

- The web server gives up a request that announces a body and sends nothing (or an upload that stalls) after fifteen seconds, instead of keeping its only thread for ever.
- The restart after an installation from GitHub no longer depends on a browser asking how far it is: a timer of the node watches the installation and restarts it when it is done.
- A "remember me" session is no longer taken for free after thirty minutes when the table fills up, and a session token that starts again replaces the old one instead of living twice.
- On a saved broker that is not the first one, the node tries the first one again every half hour instead of forgetting it until the next restart.

## [0.1.7] - The PIN to disarm on the screen, and no deadlock between the panel and the broker

- **A message is never published while a lock of the node is held:** `mqtt_link::publish()` puts the message in a short queue (48, the oldest dropped first) and a task of its own hands it to the client. Before, a task that held a lock of its module and published could wait for the client's own lock while the client, running the handler of an incoming command, waited for that module lock: both for ever, with nothing to restart the board.
- **Disarming asks for the PIN on the screen:** a keypad (digits, clear, erase) under the question; the PIN goes to the server with the order, which checks it (the server also refuses a user that has none when it requires one), and a wrong, locked or missing PIN is told in the footer for a few seconds, in the seven languages. Arming asks for none. The digits typed are shown as dots and wiped after being sent. `core/pin_entry.hpp` is tested on a computer; the keypad has not been seen on the screen.

## [0.1.6] - The energy on the wall, and the shared improvements

- **The energy of the house on the screen:** under the nodes, what the panels make, what the house draws and the battery (its charge, and whether it is charging or discharging with its power), taken from the new `energy` block of the server's panel summary (a server without it shows nothing; one whose solar equipment is silent says so). Powers are written the way a wall wants them: 850 W, 1.2 kW, 12 kW. The words are in the seven languages. Host tests cover the block, a battery that says nothing, a block that is not an object, numbers out of range and the powers' words.
- **An account of its own for the panel.** The server now has a `panel` role (least privilege): it may read the summary, change the mode and acknowledge alarms, and nothing else (an administrator creates it in Studio's users); a panel that is stolen no longer opens the whole of Studio. Use that account in the panel's settings instead of an operator's.
- **Waits that grow between tries.** A Wi-Fi network that does not answer is tried again after 1 s, then 2, 4, 8... up to 30 s (a little scatter on every wait, so that many nodes that lost the same router do not all knock together), and the broker after 1 s growing to 60 s; a try that works starts the schedule over. Before, every retry was a fixed 3 s (Wi-Fi) or the client's fixed 10 s (broker).
- **Improv Wi-Fi over the USB cable.** A web page that speaks the Improv standard (ESP Web Tools) can give the node its Wi-Fi network through the same USB socket the log comes out of: the node answers its state, identifies itself, describes the device and takes the SSID and password (checked and stored through the same settings document the panel and the Bluetooth set-up use), then restarts to join. A packet is only acted upon whole and with a right checksum. The packets are tested on a computer; the USB side has never run on a board.
- **The stack each task has left**, in the status (`tasks`: the least room ever left, smallest first, with a `low` flag under 512 bytes) and as a warning in the log, once per task, before a near overflow becomes a crash.

## [0.1.5] - The panel can update itself from GitHub

- **The node can look for a newer firmware on GitHub and install it** (*Firmware and log -> Check GitHub for a new version*), next to - never instead of - the upload of a file: it asks the repository's latest release, offers it only when it is newer and the release carries the SHA-256 of the image, downloads **the image built for this board** (`armor_hmi-<board>.bin`) straight into the other firmware slot with a progress bar, and installs it only when the hash matches; then it restarts into it. Administrators only; it needs the node to reach the Internet. The code is the one the radar node already had, now shared by every node (`main/github_update.*`, `core/semver.hpp`, `core/release_assets.hpp` in ARMOR-COMMON's firmware base).
- **The panel and its tests:** the *Firmware and log* page has the card in the seven languages, the stand-in node (`tools/panel_mock.mjs`) answers the three routes (`GET /api/v1/ota/check`, `POST /api/v1/ota/install`, `GET /api/v1/ota/progress`), and the host tests check the comparison of versions and, for a release with and without an image per board, that a board never picks an image that is not its own.
- **The release carries the image under the name of its board** (`armor_hmi-lcd7box.bin`); a release from before, with the plain `armor_hmi.bin`, still serves this board.
- **A node can subscribe to the broker** (`mqtt_link::subscribe`, shared with the other nodes) and the panel's stylesheet gained two small rules; nothing changes for the touch panel itself.

## [0.1.4] - A safer broker link and a login over HTTP after one over HTTPS

- **The broker link no longer races with itself** when the node moves to another saved broker: the client is replaced under a lock and the old one is stopped outside it, so a message being published at that moment can no longer use a client that is being destroyed.
- **The panel's help** (*Firmware and log*) describes the firmware slots and their switch, the copy of the log, in the seven languages.
- **The node says what kind it is.** The public session answer (`/api/v1/session`) now carries `kind` (`radar`, `solar`, `electrical` or `hmi`) next to the node's id, version and board, so Studio's search for nodes in the network can tell a radar node from a solar one before it is added, even when its panel title cannot be read.
- **Copy the log** (*Firmware and log -> Log*): a button copies everything the log shows to the clipboard, in the seven languages. It also works when the panel is opened over plain HTTP, where the browser's own clipboard is not available, through a fallback.
- **A login over HTTP works after one over HTTPS.** The session cookie of the page served over HTTPS now has its own name (`armor_session_tls`); with one shared name, the browser kept the "Secure" cookie of the HTTPS login and refused to let a plain-HTTP page replace it, so the login over HTTP looked as if it did nothing (it worked in a private window).

## [0.1.3] - Switch between the two firmware slots from the panel

- **Firmware slots in the panel** (*Firmware and log -> Update*): a new card shows the two application slots (ota_0 and ota_1) with the version each one holds and which one runs, and a button boots the other one at the next restart - the way back to the version that ran before an update, or forward to the one just installed. It asks for confirmation, needs an administrator, refuses an empty slot or a firmware of another project, and the settings are kept. The same card exists in the radar, solar, electrical and touch-panel nodes, in the seven languages (`POST /api/v1/ota/switch`).

## [0.1.2] - An answer longer than the buffer is no longer cut without a word

- The longest answer of the server the screen keeps goes from 8 KB to 32 KB, and when an answer is still longer it is cut at the limit (it used to lose the whole piece that did not fit) and the log says so.


## [0.1.1] - About and Help pages, date and time, hints everywhere and a panel that can always be reached

- **"Keep me signed in on this browser"** at login: unchecked, nothing changes (30 minutes idle still signs out); checked, the session survives closing the browser and lasts 30 days of actual use.
- **About:** the node's own identity, firmware version, author and licence. **Help:** a tabbed page explaining each part of the menu in plain language, in all seven languages.
- **Real bug (found on ARMOR-RADAR, fixed here too):** saving only ever added the document's backup Wi-Fi networks or brokers to the ones already stored, never replacing the list - so removing one and saving brought it straight back. Saving now replaces the list exactly as sent.
- **Date and time:** a new *Date and time* card on the Overview page shows the node's local time and where it comes from (time server, set by hand, or not set yet). The Network page has a new clock card: the time zone (a list of common zones, with summer time handled by itself), the time server on or off, and, with the server off, a button that sets the node's clock from the browser. The clock now starts by itself as soon as the node has an address - before, it only started together with the broker connection, so a node with no broker never had a time. The time server moved out of the broker settings into a `time` section of the settings file; older files that still carry `mqtt.ntp` load as before.
- **Flash memory:** a new card on the Overview page with the flash size, how much the partitions reserve, what is left unassigned and, for each of the two firmware slots, how much of it the image uses, how much is free, which version it holds and which one is running or boots next.
- **A hint on everything:** pausing the pointer over a field, a button or a menu entry now shows a short explanation in the panel's language (all seven).
- **"Load a configuration file"** now lights up under the pointer like the other buttons.
- **Real bug, found on a bench:** a backup Wi-Fi network or broker lost its password the next time the settings were saved, because the form never holds a stored password and an entry that arrived without one replaced the stored one with an empty password. An entry sent without a password now keeps the one it had (same network name; same user on the same slot or address); a different name starts empty. A secured network tried with an empty password answers "no network with compatible security"; that reason is now shown as a wrong password instead of an anonymous failure.
- **Real bug, found on a bench:** the choice of language made in the panel was never written to the node's settings, so the exported file said English while the panel was in Spanish. Choosing a language now stores it at once, with no restart.
- **Real bug, found on a bench:** the node's rescue network disappeared right after signing in to the panel through it. While the node kept looking for the Wi-Fi network it could not find, every attempt scanned all channels, and the access point shares the radio, so the phone lost the network. While someone is connected to the rescue network the node now pauses those attempts and resumes them ten seconds after the last person leaves.
- **Real bug, found on a bench:** after the first set-up by Wi-Fi the node's own network was never seen again. The set-up code was erased the moment the administrator was created, and that code is the password of the node's own and rescue networks, which could then not start. The code is now always kept (the one derived from the fleet secret, the one built into the image, or a random one stored in flash so it is the same at every start), and the set-up screen accepts a Wi-Fi network for the node to join on every board, also on those with a cable.
- Code only in this release - there is no board of this kind to flash it on yet.

## [0.1.0] - Chip, flash, PSRAM and bootloader version on the Overview page

- **A new "Hardware" card:** the chip and its revision, how many cores, the flash and PSRAM sizes, and the IDF version of both the running firmware and the bootloader (not the same thing - a mismatch there usually means the wrong board file was built). Read straight from the chip and the bootloader's own descriptor, not stored anywhere. Needed `spi_flash` added to the component's own dependencies; built clean with the real toolchain (this board's own: a full build, not just the s3-eth/s3-wifi family).

## [0.0.9] - An eye on the login and set-up passwords

- **Sign-in, the admin password and the Wi-Fi password at set-up** now have an eye button that shows what was typed - the one place a mistyped password locks someone out with no other field to check it against.

## [0.0.8] - Download and load a whole configuration

- **An admin can now download this node's whole configuration** (Network page, secrets included) as a .json file - the same document the flash keeps - and load one back into the form before saving. Built for setting up a batch of identical boards from a single bench node instead of retyping settings by hand. The node's own identity (its node ID) is never overwritten by an import: every board keeps its own. `GET /api/v1/config/export` is a new, admin-only route; loading a file changes nothing until the existing Save button is pressed.

## [0.0.7] - A periodic restart the panel can turn on

- **A new System setting:** "do not apply" or every 1, 2, 3, 4, 6, 12, 24 or 48 hours. It is the same clean restart as every other one (Wi-Fi told it is leaving before the radio powers down), just timed instead of triggered by a save or a firmware update - for a board nobody is going to touch for weeks. Settings-schema change only (`system.auto_restart_hours`, 0 by default): a node left at "do not apply" behaves exactly as before.

## [0.0.6] - Backup Wi-Fi networks and backup brokers

- **Up to 3 backup Wi-Fi networks and 2 backup brokers**, in the panel's Wi-Fi and Broker pages: tried in order, the same way a phone or laptop remembers more than one network, only after the one above has failed to connect for a while - never while it still works. Settings-schema change only (`sta.backup[]`, `mqtt.backup[]`, both empty by default): a node with none configured behaves exactly as before.

## [0.0.5] - A proper goodbye to the access point before restarting

- **The node never told the access point it was leaving before a restart:** `esp_restart()` just cuts the radio, with no deauthentication frame sent; some access points get stuck holding the old association and need restarting themselves before the node can rejoin. It now calls `esp_wifi_disconnect()` and gives it a moment before restarting, on every restart path (the panel, a firmware update).
- **The "Restarting..." screen never appeared after a firmware update:** `S.rebooting = true` was set without calling `render()` on that one path - the panel just sat on the old screen until the auto-reload kicked in on its own six seconds later. Now it shows immediately.
- **The default HTTP header limit (512 bytes) was too small for a real browser:** a session cookie plus a modern browser's own request headers can exceed it, which the panel refused outright - a blank page saying "Header fields are too long". Raised to 2048 bytes.

## [0.0.4] - This panel had never actually received the other nodes' fixes

- **The Bluetooth stack-overflow fix never reached this project:** `ble-worker`'s stack was still 8 KB, the exact size that overflows and restarts the node mid-save on ARMOR-RADAR/-SOLAR/-ELECTRICAL before their fix; it is now 16 KB here too. Found by checking `firmware_base/` against every project's own copy of the files it shares, not by reproducing the crash on this board (it has never run on real hardware yet).
- **Bluetooth still would not advertise without an address:** the same older `wanted` condition (extra `&& setup_mode`) that kept the other three nodes undiscoverable before their own fix was still here; now Bluetooth advertises at every start while in set-up mode, same as the rest of the family.
- **The panel's title said "A.R.M.O.R. node"** instead of naming its own kind, same as the other three nodes already did; now "A.R.M.O.R. hmi".
- No behaviour change beyond catching up: `main/network.cpp/.hpp`, `main/mqtt_link.cpp` and `main/node_store.cpp` were also brought back in line with the shared base. The earlier fixes had been written straight into each project's own copy without updating `ARMOR-COMMON/firmware_base`, so `sync_firmware_base.py check` never had a current base to compare this project against.

## [0.0.3] - A login that actually leaves you signed in

- **The session cookie was garbage:** it was built in a function's own local variable, and the HTTP server only keeps a pointer to a header's text, not a copy of it; by the time the response was really sent, that memory had already been reused for something else. The login or the first-time set-up answered "ok", but no browser ever kept a real session - re-entering the panel always looked like a fresh sign-in. The cookie is now kept alive until the response goes out.
- Built with ESP-IDF 5.5.5.

## [0.0.2] - The firmware builds

- **First real build** with ESP-IDF 5.5.5 for the ESP32-S3: the image is about 2.4 MB, well inside its 6 MB partition.
- **Fixes the build found:** the ESP-IDF headers are no longer wrapped in `extern "C"` (ESP-IDF 5.5 declares C++ overloads there), the GT911 touch configuration is filled by name instead of through the component's macro, and a few standard headers (`<ctime>`, `<cstring>`, `<algorithm>`) are included where they are used.
- The seven READMEs and the build and design documents now say it builds, and that it has never been run on a board.

## [0.0.1] - The touch panel

- **First version.** A new project for the Waveshare ESP32-S3-Touch-LCD-7C-BOX: a 7-inch wall screen that shows the state of the system, arms, disarms and acknowledges, with the microphone and the speaker the voice assistant will use. It is a node of the family (the settings, Wi-Fi, web page, Bluetooth set-up and firmware update with roll-back come from `ARMOR-COMMON/firmware_base`, synchronised into this repository) and a client of ARMOR-SERVER like the Android app.
- **`core/` (tested on a computer, four test programs):** `hmi_config.hpp` (the panel's settings: the link to the server, the screen, the sound, the voice), `server_view.hpp` (the summary of `GET /api/v1/panel/summary`, the state of the link with its back-off and its "old information" rule, what is new, the colour of the screen, the night brightness), `voice_session.hpp` (the conversation as a state machine with time limits), `screen_text.hpp` (the screen's words in seven languages) and `board_s3.hpp` (the board's pins, with a compile-time check that no two functions share a GPIO).
- **`main/` (never built or run on a board):** the screen (RGB panel, GT911 touch, LVGL 9 through `esp_lvgl_port`), the sound (I2S, ES7210 microphones, ES8389 speaker), push-to-talk voice, the HTTP link to ARMOR-SERVER, the I/O expander (backlight, touch reset, amplifier) and the generated fonts (Inter, Noto Sans JP and SC) so that accents, Japanese and Chinese are drawn.
- **The web page** (`panel/`): the pages of the other nodes plus Server, Screen and sound, and Voice assistant, in seven languages; tested in a real browser against `tools/panel_mock.mjs`.
- **Needs ARMOR-SERVER 0.3.7** for `GET /api/v1/panel/summary`.
- **Not yet:** a first compilation and power-up, a wake word, the speech service and a view of the cameras on the screen.
