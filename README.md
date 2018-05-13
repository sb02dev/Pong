# Multiplayer Pong for ESP32

## About

This is a rewrite of the game [Pong](https://en.wikipedia.org/wiki/Pong) for the [ESP32](https://www.espressif.com/en/products/hardware/esp-wroom-32/overview) microcontroller board.

The game code is based on [this](https://codeincomplete.com/posts/javascript-pong/) JavaScript implementation.

Also the aim of this rewrite was to test the WiFi capabilities of the board and to make a proof of concept on my first attempt at a networked real-time game.

The networking code is using WiFi and TCP but it is more or less separated as I intend to explore a Bluetooth version of the game as well. Optimizations on WiFi could also include an UDP based implementation (help is welcome).

## Installation

Just download or clone from github.

## Compilation

Download [PlatformIO](https://platformio.org/), plug in your board to your computer and run `pio run -t upload`.

## Contribution

Please feel free to modify and improve anything you want. I am also open to improvements especially in the network code.