# ARMOR-HMI - the voice assistant

The panel has four microphones and a speaker so that it can be the "home assistant with a big screen" of the installation, and so that the AI planned for the Jetson Orin NX can talk
to the people in the house through it. This version has the panel's half of that, in its simplest honest form.

## What there is

* **Push to talk.** A touch on the microphone button opens the microphone; a second touch (or the time limit, 8 seconds by default) closes it. The audio goes to the speech service
  whose address is in the settings, the reply is shown and spoken. Nothing is sent anywhere unless the voice is turned on in the settings and given an address.
* **A conversation as a state machine** (`core/voice_session.hpp`, tested in `tests/test_voice.cpp`): idle, listening, thinking, speaking, failed. Every step has a time limit
  (a reply that never ends does not lock the screen), a touch always stops it, a failure is shown for a few seconds and dismissed by a touch, and a sound right after a conversation
  (the speaker's own) is not taken for a new request.
* **The panel carries out no command by itself.** Whatever changes something (arming, a switch) is confirmed by the service with a question the person answers, as ARMOR-VOICE-AI's
  signed single-use confirmation does; the panel only shows and plays what the service says.

## What there is not

* **A wake word.** "Armor" cannot be said yet: a wake word needs a model for the chip (Espressif's ESP-SR) trained for the name, and the board's four microphones can feed it, but it is
  not part of this version. The setting "name it answers to" is what the screen will show once it exists.
* **The speech service.** ARMOR-VOICE-AI today is an intent gateway with no speech engine (text in, intent out). Speech to text, the answer and text to speech have to be a service on
  the Jetson that speaks the contract below; until it exists the assistant has nobody to talk to.

## The contract with the speech service

```
POST <voice.url>/v1/turn
Content-Type: audio/L16;rate=16000      16 kHz, mono, 16-bit signed little-endian PCM, at most listen_s seconds
X-Armor-Language: en|es|de|fr|it|ja|zh  the language of the panel

200 {"heard":"arm the system", "reply":"Arm the system? Say yes to confirm.", "audio":"<base64 of 16 kHz mono 16-bit PCM>"}
```

`heard` and `audio` are optional (without audio only the text is shown); a reply with neither text nor audio, a non-200 answer or a malformed body is a failure shown on the screen
("I could not do that"). The panel accepts at most 256 KB of answer.

## Privacy

The microphone is open only between the touch and the end of the recording; no audio is stored on the panel and none leaves it unless the voice is on. The address of the service is the
installation's own (a machine of the local network); an `https://` address is checked against the usual certificate authorities.
