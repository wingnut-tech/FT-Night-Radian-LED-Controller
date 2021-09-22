## [v1.2.4] - 2021-09-02
### Changed
- Auto-adjusting knob input range
  - No more fiddling with throws and endpoints on your radio! Just sweep your transmitter knob to the low and high ends after powering up, and the controller will learn your endpoints.

## [v1.2.3] - 2021-03-03
### Added
- `TESTMODE` define.
  - Uncommenting and setting this define will skip the RC input code, allowing easy testing of new shows without needing a valid RC input.

### Changed
- New `MAX_ALTIMETER` value and `altitude()` function.
  - The `altitude()` function still indicates the current altitude on the wings/fuse like before, but now it will fill up with white LEDs from 0-400ft, and then start filling in reddish/orange LEDs from 400-800ft.

## [v1.2.2] - 2020-06-07
### Added
- Added status flash to indicate presence of BMP280 module (mainly for easier testing)

### Changed
- Adjusted RC input upper bounds from 1980us down to 1960us. This should help a bit with those that have needed to adjust radio endpoints to be able to select all shows.

## [v1.2.1] - 2020-05-20
### Added
- New RC input status indicators.
  - LEDs flash red when there's no input signal.
  - Once a signal is found, LEDs flash white once for RC input 1, twice for input 2.

### Changed
- MAX_ALTITUDE changed from 400 to 600
- Toned down flashing for program mode indicators

## [v1.2.0] - 2020-05-15
First "official" release.
