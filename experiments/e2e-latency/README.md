# High-polling mouse event-age experiment

This branch contains a standalone SDL 3 probe that measures the age of mouse-motion events when the application consumes them.

Historical reference: SDL issue 8756 / PR 8770 changed the Windows raw-input path from per-message `GetRawInputData` handling to buffered `GetRawInputBuffer`.

## Build

Against an installed SDL 3:

```sh
cc -O2 experiments/e2e-latency/mouse-event-age.c \
  -o /tmp/sdl-mouse-event-age \
  $(pkg-config --cflags --libs sdl3)
```

## Run

```sh
/tmp/sdl-mouse-event-age 15

SDL_LATENCY_RELATIVE=1 /tmp/sdl-mouse-event-age 15
```

Sweep the mouse's supported report rates and repeat each state several times.

Record alongside:

```sh
perf stat -e task-clock,cycles,instructions,context-switches,cpu-migrations \
  /tmp/sdl-mouse-event-age 15
```

## Output

The probe reports motion event rate, time spent in the polling loop, and event-age p50/p95/p99/p99.9/max.

On Linux, compare against the direct evdev probe from the kernel experiment branch to measure how much age appears above the kernel input layer.
