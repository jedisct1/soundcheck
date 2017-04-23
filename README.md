# SoundCheck

Estimates what an average ambient sound level is, detects abnormal and sustained increases of that level and calls a URL when this happens and when it goes back to normal.

Requires:
- An Arduino board, such as an Arduino Uno.
- A 330 Ohm resistor.
- An electret microphone (recommended: with amplifier) -- connect the analog output (`AO`) to `A0` + ground/+5V.
- For boards without built-in WiFi, a WiFi shield with an up-to-date firmware.
- A HTTP server able to perform some action, such as send a text message, when a query for a fixed URL is received.

SoundCheck joins the WiFi network on demand, and disconnects right after having sent a notification.