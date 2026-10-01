#include <SDL3/SDL.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static int cmp_u64(const void *a, const void *b)
{
    const Uint64 aa = *(const Uint64 *)a;
    const Uint64 bb = *(const Uint64 *)b;
    return (aa > bb) - (aa < bb);
}

static Uint64 percentile(Uint64 *samples, size_t count, double q)
{
    if (!count) {
        return 0;
    }
    size_t index = (size_t)((count - 1) * q);
    return samples[index];
}

int main(int argc, char **argv)
{
    const double seconds = argc > 1 ? SDL_atof(argv[1]) : 10.0;
    const bool relative = SDL_getenv("SDL_LATENCY_RELATIVE") != NULL;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL input event-age probe", 960, 540, 0);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    if (relative && !SDL_SetWindowRelativeMouseMode(window, true)) {
        SDL_Log("relative mouse mode failed: %s", SDL_GetError());
    }

    size_t capacity = 1u << 20;
    size_t count = 0;
    Uint64 *ages = SDL_malloc(capacity * sizeof(*ages));
    if (!ages) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Uint64 start = SDL_GetTicksNS();
    Uint64 deadline = start + (Uint64)(seconds * 1000000000.0);
    Uint64 motion_events = 0;
    Uint64 polls = 0;
    Uint64 poll_ns = 0;

    while (SDL_GetTicksNS() < deadline) {
        SDL_Event event;
        Uint64 poll_begin = SDL_GetTicksNS();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                deadline = 0;
                break;
            }

            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                Uint64 now = SDL_GetTicksNS();
                Uint64 age = now >= event.motion.timestamp ? now - event.motion.timestamp : 0;

                if (count == capacity) {
                    size_t next_capacity = capacity * 2;
                    Uint64 *next = SDL_realloc(ages, next_capacity * sizeof(*ages));
                    if (!next) {
                        deadline = 0;
                        break;
                    }
                    ages = next;
                    capacity = next_capacity;
                }

                ages[count++] = age;
                motion_events++;
            }
        }

        poll_ns += SDL_GetTicksNS() - poll_begin;
        polls++;
        SDL_DelayNS(100000);
    }

    qsort(ages, count, sizeof(*ages), cmp_u64);

    printf("duration_s=%.3f\n", seconds);
    printf("relative=%d\n", relative ? 1 : 0);
    printf("motion_events=%" PRIu64 "\n", motion_events);
    printf("events_per_s=%.3f\n", seconds > 0.0 ? motion_events / seconds : 0.0);
    printf("poll_calls=%" PRIu64 "\n", polls);
    printf("poll_total_ms=%.3f\n", poll_ns / 1000000.0);
    printf("event_age_p50_us=%.3f\n", percentile(ages, count, 0.50) / 1000.0);
    printf("event_age_p95_us=%.3f\n", percentile(ages, count, 0.95) / 1000.0);
    printf("event_age_p99_us=%.3f\n", percentile(ages, count, 0.99) / 1000.0);
    printf("event_age_p999_us=%.3f\n", percentile(ages, count, 0.999) / 1000.0);
    printf("event_age_max_us=%.3f\n", count ? ages[count - 1] / 1000.0 : 0.0);

    SDL_free(ages);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
