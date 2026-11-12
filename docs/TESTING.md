# Test Results

Tests were run on 25 June 2026 with both boards, Mosquitto and Node-RED on one router network. Traffic was monitored with:

```
mosquitto_sub -h localhost -t "parking/#" -t "status/#" -v
```

## A. Basic functionality

| # | Test | Result |
|---|---|---|
| A1 | Power on both boards | Pass: both connect, `status/board1` and `status/board2` are `online` |
| A2 | Display on start-up | Pass: shows `-` until the first status arrives, then `5` |
| A3 | Car at the entry sensor | Pass: barrier opens, light turns green |
| A4 | Car passes the entry | Pass: light turns red, `parking/entry` after about 2 s, status `{"free":4,"total":5}`, barrier closes |
| A5 | Display and dashboard after entry | Pass: both show 4 |
| A6 | Car at the exit sensor | Pass: barrier opens, light turns green (exit is always allowed) |
| A7 | Car passes the exit | Pass: `parking/exit`, status back to 5 |
| A8 | User button as test trigger | Pass: triggers the same sequence as the sensor on both boards |

## B. Edge cases

| # | Test | Result |
|---|---|---|
| B1 | Garage full, car at entry | Pass: barrier stays closed, light red, no entry event |
| B2 | One car leaves a full garage | Pass: free spaces 0 to 1, entry opens for the next car |
| B3 | Cold start: reset Board 1 during operation | Pass: correct value restored immediately via the retained status |
| B4 | Count down to 0 and back up to 5 | Pass: counter stops at both limits |
| B5 | Car arrives and backs away without passing | Known limitation: barrier closes correctly, but an entry is counted (see ARCHITECTURE.md) |

## C. Communication and robustness

| # | Test | Result |
|---|---|---|
| C1 | Stop and restart the broker | Pass: both boards reconnect automatically, system continues |
| C2 | Unplug the Ethernet cable of Board 1 | Pass: Last Will `offline` after 71 s (1.5 x keep-alive of 60 s) |
| C3 | Plug the cable back in | Pass: online again immediately with the correct state |
| C4 | Run for several minutes | Pass: no freezes, no resets, no reconnect cycles |
| C5 | Run for over 10 minutes with repeated entries and exits | Pass: no stuck barriers, counter consistent |

## D. Latency

| Measurement | Result |
|---|---|
| Sensor detects car until barrier opens | Below 1 s (sensor at 5 Hz plus 3-sample debounce) |
| `parking/entry` until new `parking/status` | Below 1 s (all 5 runs within the same second) |
| `parking/status` until the display updates | No visible delay |
| Reconnect after broker restart | 23 to 26 s (includes detecting the outage and the retry interval) |
