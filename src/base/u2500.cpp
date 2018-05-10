// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/color.h"
#include "app/config.h"
#include "u2500.h"

enum u2500op: unsigned char { fi, nl, np, ol, op, re, al, rr, wl = 9, wp = 10 };
typedef const unsigned char charspec[];

static charspec s_u2500 = { nl, 1, 0, 60, 120, 60, fi };
static charspec s_u2501 = { wl, 1, 0, 60, 120, 60, fi };
static charspec s_u2502 = { nl, 1, 60, 0, 60, 120, fi };
static charspec s_u2503 = { wl, 1, 60, 0, 60, 120, fi };
static charspec s_u2504 = { nl, 3, 0, 60, 20, 60, 40, 60, 60, 60, 80, 60, 100, 60, fi };
static charspec s_u2505 = { wl, 3, 0, 60, 20, 60, 40, 60, 60, 60, 80, 60, 100, 60, fi };
static charspec s_u2506 = { nl, 3, 60, 0, 60, 20, 60, 40, 60, 60, 60, 80, 60, 100, fi };
static charspec s_u2507 = { wl, 3, 60, 0, 60, 20, 60, 40, 60, 60, 60, 80, 60, 100, fi };
static charspec s_u2508 = { nl, 4, 0, 60, 15, 60, 30, 60, 45, 60, 60, 60, 75, 60, 90, 60, 105, 60, fi };
static charspec s_u2509 = { wl, 4, 0, 60, 15, 60, 30, 60, 45, 60, 60, 60, 75, 60, 90, 60, 105, 60, fi };
static charspec s_u250a = { nl, 4, 60, 0, 60, 15, 60, 30, 60, 45, 60, 60, 60, 75, 60, 90, 60, 105, fi };
static charspec s_u250b = { wl, 4, 60, 0, 60, 15, 60, 30, 60, 45, 60, 60, 60, 75, 60, 90, 60, 105, fi };
static charspec s_u250c = { np, 3, 60, 120, 60, 60, 120, 60, fi };
static charspec s_u250d = { nl, 1, 60, 60, 60, 120, wl, 1, 60, 60, 120, 60, fi };
static charspec s_u250e = { wl, 1, 60, 60, 60, 120, nl, 1, 60, 60, 120, 60, fi };
static charspec s_u250f = { wp, 3, 60, 120, 60, 60, 120, 60, fi };
static charspec s_u2510 = { np, 3, 0, 60, 60, 60, 60, 120, fi };
static charspec s_u2511 = { wl, 1, 60, 60, 0, 60, nl, 1, 60, 60, 60, 120, fi };
static charspec s_u2512 = { nl, 1, 60, 60, 0, 60, wl, 1, 60, 60, 60, 120, fi };
static charspec s_u2513 = { wp, 3, 0, 60, 60, 60, 60, 120, fi };
static charspec s_u2514 = { np, 3, 60, 0, 60, 60, 120, 60, fi };
static charspec s_u2515 = { nl, 1, 60, 60, 60, 0, wl, 1, 60, 60, 120, 60, fi };
static charspec s_u2516 = { wl, 1, 60, 60, 60, 0, nl, 1, 60, 60, 120, 60, fi };
static charspec s_u2517 = { wp, 3, 60, 0, 60, 60, 120, 60, fi };
static charspec s_u2518 = { np, 3, 0, 60, 60, 60, 60, 0, fi };
static charspec s_u2519 = { wl, 1, 60, 60, 0, 60, nl, 1, 60, 60, 60, 0, fi };
static charspec s_u251a = { nl, 1, 60, 60, 0, 60, wl, 1, 60, 60, 60, 0, fi };
static charspec s_u251b = { wp, 3, 0, 60, 60, 60, 60, 0, fi };
static charspec s_u251c = { nl, 2, 60, 0, 60, 120, 60, 60, 120, 60, fi };
static charspec s_u251d = { wl, 1, 60, 60, 120, 60, nl, 1, 60, 0, 60, 120, fi };
static charspec s_u251e = { wl, 1, 60, 60, 60, 0, nl, 2, 60, 60, 60, 120, 60, 60, 120, 60, fi };
static charspec s_u251f = { wl, 1, 60, 60, 60, 120, nl, 2, 60, 60, 60, 0, 60, 60, 120, 60, fi };
static charspec s_u2520 = { wl, 1, 60, 0, 60, 120, nl, 1, 60, 60, 120, 60, fi };
static charspec s_u2521 = { wp, 3, 60, 0, 60, 60, 120, 60, nl, 1, 60, 60, 60, 120, fi };
static charspec s_u2522 = { wp, 3, 60, 120, 60, 60, 120, 60, nl, 1, 60, 60, 60, 0, fi };
static charspec s_u2523 = { wl, 2, 60, 0, 60, 120, 60, 60, 120, 60, fi };
static charspec s_u2524 = { nl, 2, 60, 0, 60, 120, 0, 60, 60, 60, fi };
static charspec s_u2525 = { nl, 1, 60, 0, 60, 120, wl, 1, 0, 60, 60, 60, fi };
static charspec s_u2526 = { wl, 1, 60, 60, 60, 0, np, 3, 0, 60, 60, 60, 60, 120, fi };
static charspec s_u2527 = { wl, 1, 60, 60, 60, 120, np, 3, 0, 60, 60, 60, 60, 0, fi };
static charspec s_u2528 = { wl, 1, 60, 0, 60, 120, nl, 1, 0, 60, 60, 60, fi };
static charspec s_u2529 = { wp, 3, 0, 60, 60, 60, 60, 0, nl, 1, 60, 60, 60, 120, fi };
static charspec s_u252a = { wp, 3, 0, 60, 60, 60, 60, 120, nl, 1, 60, 60, 60, 0, fi };
static charspec s_u252b = { wl, 2, 60, 0, 60, 120, 0, 60, 60, 60, fi };
static charspec s_u252c = { nl, 2, 0, 60, 120, 60, 60, 60, 60, 120, fi };
static charspec s_u252d = { np, 3, 60, 120, 60, 60, 120, 60, wl, 1, 0, 60, 60, 60, fi };
static charspec s_u252e = { np, 3, 0, 60, 60, 60, 60, 120, wl, 1, 60, 60, 120, 60, fi };
static charspec s_u252f = { wl, 1, 0, 60, 120, 60, nl, 1, 60, 60, 60, 120, fi };
static charspec s_u2530 = { nl, 1, 0, 60, 120, 60, wl, 1, 60, 60, 60, 120, fi };
static charspec s_u2531 = { wp, 3, 0, 60, 60, 60, 60, 120, nl, 1, 60, 60, 120, 60, fi };
static charspec s_u2532 = { wp, 3, 60, 120, 60, 60, 120, 60, nl, 1, 0, 60, 60, 60, fi };
static charspec s_u2533 = { wl, 2, 0, 60, 120, 60, 60, 60, 60, 120, fi };
static charspec s_u2534 = { nl, 2, 0, 60, 120, 60, 60, 0, 60, 60, fi };
static charspec s_u2535 = { np, 3, 60, 0, 60, 60, 120, 60, wl, 1, 0, 60, 60, 60, fi };
static charspec s_u2536 = { np, 3, 0, 60, 60, 60, 60, 0, wl, 1, 60, 60, 120, 60, fi };
static charspec s_u2537 = { wl, 1, 0, 60, 120, 60, nl, 1, 60, 0, 60, 60, fi };
static charspec s_u2538 = { nl, 1, 0, 60, 120, 60, wl, 1, 60, 0, 60, 60, fi };
static charspec s_u2539 = { wp, 3, 0, 60, 60, 60, 60, 0, nl, 1, 60, 60, 120, 60, fi };
static charspec s_u253a = { wp, 3, 60, 0, 60, 60, 120, 60, nl, 1, 0, 60, 60, 60, fi };
static charspec s_u253b = { wl, 2, 0, 60, 120, 60, 60, 0, 60, 60, fi };
static charspec s_u253c = { nl, 2, 60, 0, 60, 120, 0, 60, 120, 60, fi };
static charspec s_u253d = { nl, 2, 60, 0, 60, 120, 60, 60, 120, 60, wl, 1, 0, 60, 60, 60, fi };
static charspec s_u253e = { nl, 2, 60, 0, 60, 120, 0, 60, 60, 60, wl, 1, 60, 60, 120, 60, fi };
static charspec s_u253f = { wl, 1, 0, 60, 120, 60, nl, 1, 60, 0, 60, 120, fi };
static charspec s_u2540 = { nl, 2, 0, 60, 120, 60, 60, 60, 60, 120, wl, 1, 60, 0, 60, 60, fi };
static charspec s_u2541 = { nl, 2, 0, 60, 120, 60, 60, 0, 60, 60, wl, 1, 60, 60, 60, 120, fi };
static charspec s_u2542 = { wl, 1, 60, 0, 60, 120, nl, 1, 0, 60, 120, 60, fi };
static charspec s_u2543 = { wp, 3, 0, 60, 60, 60, 60, 0, np, 3, 60, 120, 60, 60, 120, 60, fi };
static charspec s_u2544 = { wp, 3, 60, 0, 60, 60, 120, 60, np, 3, 0, 60, 60, 60, 60, 120, fi };
static charspec s_u2545 = { wp, 3, 0, 60, 60, 60, 60, 120, np, 3, 60, 0, 60, 60, 120, 60, fi };
static charspec s_u2546 = { wp, 3, 60, 120, 60, 60, 120, 60, np, 3, 0, 60, 60, 60, 60, 0, fi };
static charspec s_u2547 = { wl, 2, 0, 60, 120, 60, 60, 0, 60, 60, nl, 1, 60, 60, 60, 120, fi };
static charspec s_u2548 = { wl, 2, 0, 60, 120, 60, 60, 60, 60, 120, nl, 1, 60, 0, 60, 60, fi };
static charspec s_u2549 = { wl, 2, 60, 0, 60, 120, 0, 60, 60, 60, nl, 1, 60, 60, 120, 60, fi };
static charspec s_u254a = { wl, 2, 60, 0, 60, 120, 60, 60, 120, 60, nl, 1, 0, 60, 60, 60, fi };
static charspec s_u254b = { wl, 2, 60, 0, 60, 120, 0, 60, 120, 60, fi };
static charspec s_u254c = { nl, 2, 0, 60, 30, 60, 60, 60, 90, 60, fi };
static charspec s_u254d = { wl, 2, 0, 60, 30, 60, 60, 60, 90, 60, fi };
static charspec s_u254e = { nl, 2, 60, 0, 60, 30, 60, 60, 60, 90, fi };
static charspec s_u254f = { wl, 2, 60, 0, 60, 30, 60, 60, 60, 90, fi };
static charspec s_u2550 = { ol, 2, 1, 128, 0, 127, 0, 2, 128, 0, 127, 0, fi };
static charspec s_u2551 = { ol, 2, 4, 0, 128, 0, 127, 8, 0, 128, 0, 127, fi };
static charspec s_u2552 = { op, 3, 1, 0, 127, 0, 0, 127, 0, ol, 1, 2, 0, 0, 127, 0, fi };
static charspec s_u2553 = { op, 3, 4, 0, 127, 0, 0, 127, 0, ol, 1, 8, 0, 0, 0, 127, fi };
static charspec s_u2554 = { op, 3, 5, 0, 127, 0, 0, 127, 0, op, 3, 10, 0, 127, 0, 0, 127, 0, fi };
static charspec s_u2555 = { op, 3, 1, 128, 0, 0, 0, 0, 127, ol, 1, 2, 128, 0, 0, 0, fi };
static charspec s_u2556 = { op, 3, 8, 128, 0, 0, 0, 0, 127, ol, 1, 4, 0, 0, 0, 127, fi };
static charspec s_u2557 = { op, 3, 6, 128, 0, 0, 0, 0, 127, op, 3, 9, 128, 0, 0, 0, 0, 127, fi };
static charspec s_u2558 = { op, 3, 2, 0, 128, 0, 0, 127, 0, ol, 1, 1, 0, 0, 127, 0, fi };
static charspec s_u2559 = { op, 3, 4, 0, 128, 0, 0, 127, 0, ol, 1, 8, 0, 128, 0, 0, fi };
static charspec s_u255a = { op, 3, 6, 0, 128, 0, 0, 127, 0, op, 3, 9, 0, 128, 0, 0, 127, 0, fi };
static charspec s_u255b = { op, 3, 2, 128, 0, 0, 0, 0, 128, ol, 1, 1, 128, 0, 0, 0, fi };
static charspec s_u255c = { op, 3, 8, 128, 0, 0, 0, 0, 128, ol, 1, 4, 0, 128, 0, 0, fi };
static charspec s_u255d = { op, 3, 5, 128, 0, 0, 0, 0, 128, op, 3, 10, 128, 0, 0, 0, 0, 128, fi };
static charspec s_u255e = { nl, 1, 60, 0, 60, 120, ol, 2, 1, 0, 0, 127, 0, 2, 0, 0, 127, 0, fi };
static charspec s_u255f = { ol, 3, 4, 0, 128, 0, 127, 8, 0, 128, 0, 127, 8, 0, 0, 127, 0, fi };
static charspec s_u2560 = { ol, 1, 4, 0, 128, 0, 127, op, 3, 9, 0, 128, 0, 0, 127, 0, op, 3, 10, 0, 127, 0, 0, 127, 0, fi };
static charspec s_u2561 = { nl, 1, 60, 0, 60, 120, ol, 2, 1, 128, 0, 0, 0, 2, 128, 0, 0, 0, fi };
static charspec s_u2562 = { ol, 3, 4, 0, 128, 0, 127, 8, 0, 128, 0, 127, 4, 128, 0, 0, 0, fi };
static charspec s_u2563 = { ol, 1, 8, 0, 128, 0, 127, op, 3, 5, 128, 0, 0, 0, 0, 128, op, 3, 6, 128, 0, 0, 0, 0, 127, fi };
static charspec s_u2564 = { ol, 3, 1, 128, 0, 127, 0, 2, 128, 0, 127, 0, 2, 0, 0, 0, 127, fi };
static charspec s_u2565 = { nl, 1, 0, 60, 120, 60, ol, 2, 4, 0, 0, 0, 127, 8, 0, 0, 0, 127, fi };
static charspec s_u2566 = { ol, 1, 1, 128, 0, 127, 0, op, 3, 6, 128, 0, 0, 0, 0, 127, op, 3, 10, 0, 127, 0, 0, 127, 0, fi };
static charspec s_u2567 = { ol, 3, 2, 128, 0, 127, 0, 1, 128, 0, 127, 0, 1, 0, 128, 0, 0, fi };
static charspec s_u2568 = { nl, 1, 0, 60, 120, 60, ol, 2, 4, 0, 128, 0, 0, 8, 0, 128, 0, 0, fi };
static charspec s_u2569 = { ol, 1, 2, 128, 0, 127, 0, op, 3, 5, 128, 0, 0, 0, 0, 128, op, 3, 9, 0, 128, 0, 0, 127, 0, fi };
static charspec s_u256a = { nl, 1, 60, 0, 60, 120, ol, 2, 1, 128, 0, 127, 0, 2, 128, 0, 127, 0, fi };
static charspec s_u256b = { nl, 1, 0, 60, 120, 60, ol, 2, 4, 0, 128, 0, 127, 8, 0, 128, 0, 127, fi };
static charspec s_u256c = { op, 3, 5, 128, 0, 0, 0, 0, 128, op, 3, 9, 0, 128, 0, 0, 127, 0, op, 3, 6, 128, 0, 0, 0, 0, 127, op, 3, 10, 0, 127, 0, 0, 127, 0, fi };
static charspec s_u256d = { rr, 0, fi };
static charspec s_u256e = { rr, 2, fi };
static charspec s_u256f = { rr, 3, fi };
static charspec s_u2570 = { rr, 1, fi };
static charspec s_u2571 = { al, 1, 0, 120, 120, 0, fi };
static charspec s_u2572 = { al, 1, 0, 0, 120, 120, fi };
static charspec s_u2573 = { al, 2, 0, 120, 120, 0, 0, 0, 120, 120, fi };
static charspec s_u2574 = { nl, 1, 0, 60, 60, 60, fi };
static charspec s_u2575 = { nl, 1, 60, 0, 60, 60, fi };
static charspec s_u2576 = { nl, 1, 60, 60, 120, 60, fi };
static charspec s_u2577 = { nl, 1, 60, 60, 60, 120, fi };
static charspec s_u2578 = { wl, 1, 0, 60, 60, 60, fi };
static charspec s_u2579 = { wl, 1, 60, 0, 60, 60, fi };
static charspec s_u257a = { wl, 1, 60, 60, 120, 60, fi };
static charspec s_u257b = { wl, 1, 60, 60, 60, 120, fi };
static charspec s_u257c = { nl, 1, 0, 60, 60, 60, wl, 1, 60, 60, 120, 60, fi };
static charspec s_u257d = { nl, 1, 60, 0, 60, 60, wl, 1, 60, 60, 60, 120, fi };
static charspec s_u257e = { wl, 1, 0, 60, 60, 60, nl, 1, 60, 60, 120, 60, fi };
static charspec s_u257f = { wl, 1, 60, 0, 60, 60, nl, 1, 60, 60, 60, 120, fi };
static charspec s_u2580 = { re, 1, 0, 128, 128, 255, 188, fi };
static charspec s_u2581 = { re, 1, 0, 128, 105, 255, 255, fi };
static charspec s_u2582 = { re, 1, 0, 128, 90, 255, 255, fi };
static charspec s_u2583 = { re, 1, 0, 128, 75, 255, 255, fi };
static charspec s_u2584 = { re, 1, 0, 128, 60, 255, 255, fi };
static charspec s_u2585 = { re, 1, 0, 128, 45, 255, 255, fi };
static charspec s_u2586 = { re, 1, 0, 128, 30, 255, 255, fi };
static charspec s_u2587 = { re, 1, 0, 128, 15, 255, 255, fi };
static charspec s_u2588 = { re, 1, 0, 128, 128, 255, 255, fi };
static charspec s_u2589 = { re, 1, 0, 128, 128, 233, 255, fi };
static charspec s_u258a = { re, 1, 0, 128, 128, 218, 255, fi };
static charspec s_u258b = { re, 1, 0, 128, 128, 203, 255, fi };
static charspec s_u258c = { re, 1, 0, 128, 128, 188, 255, fi };
static charspec s_u258d = { re, 1, 0, 128, 128, 173, 255, fi };
static charspec s_u258e = { re, 1, 0, 128, 128, 158, 255, fi };
static charspec s_u258f = { re, 1, 0, 128, 128, 143, 255, fi };
static charspec s_u2590 = { re, 1, 0, 60, 128, 255, 255, fi };
static charspec s_u2591 = { re, 1, 15, 128, 128, 255, 255, fi };
static charspec s_u2592 = { re, 1, 50, 128, 128, 255, 255, fi };
static charspec s_u2593 = { re, 1, 130, 128, 128, 255, 255, fi };
static charspec s_u2594 = { re, 1, 0, 128, 128, 255, 143, fi };
static charspec s_u2595 = { re, 1, 0, 105, 128, 255, 255, fi };
static charspec s_u2596 = { re, 1, 0, 128, 60, 188, 255, fi };
static charspec s_u2597 = { re, 1, 0, 60, 60, 255, 255, fi };
static charspec s_u2598 = { re, 1, 0, 128, 128, 188, 188, fi };
static charspec s_u2599 = { re, 2, 0, 128, 128, 188, 255, 0, 128, 60, 255, 255, fi };
static charspec s_u259a = { re, 2, 0, 128, 128, 188, 188, 0, 60, 60, 255, 255, fi };
static charspec s_u259b = { re, 2, 0, 128, 128, 255, 188, 0, 128, 128, 188, 255, fi };
static charspec s_u259c = { re, 2, 0, 128, 128, 255, 188, 0, 60, 128, 255, 255, fi };
static charspec s_u259d = { re, 1, 0, 60, 128, 255, 188, fi };
static charspec s_u259e = { re, 2, 0, 128, 60, 188, 255, 0, 60, 128, 255, 188, fi };
static charspec s_u259f = { re, 2, 0, 128, 60, 255, 255, 0, 60, 128, 255, 255, fi };

static const unsigned char *s_specs[] = {
    s_u2500, s_u2501, s_u2502, s_u2503, s_u2504, s_u2505, s_u2506, s_u2507,
    s_u2508, s_u2509, s_u250a, s_u250b, s_u250c, s_u250d, s_u250e, s_u250f,
    s_u2510, s_u2511, s_u2512, s_u2513, s_u2514, s_u2515, s_u2516, s_u2517,
    s_u2518, s_u2519, s_u251a, s_u251b, s_u251c, s_u251d, s_u251e, s_u251f,
    s_u2520, s_u2521, s_u2522, s_u2523, s_u2524, s_u2525, s_u2526, s_u2527,
    s_u2528, s_u2529, s_u252a, s_u252b, s_u252c, s_u252d, s_u252e, s_u252f,
    s_u2530, s_u2531, s_u2532, s_u2533, s_u2534, s_u2535, s_u2536, s_u2537,
    s_u2538, s_u2539, s_u253a, s_u253b, s_u253c, s_u253d, s_u253e, s_u253f,
    s_u2540, s_u2541, s_u2542, s_u2543, s_u2544, s_u2545, s_u2546, s_u2547,
    s_u2548, s_u2549, s_u254a, s_u254b, s_u254c, s_u254d, s_u254e, s_u254f,
    s_u2550, s_u2551, s_u2552, s_u2553, s_u2554, s_u2555, s_u2556, s_u2557,
    s_u2558, s_u2559, s_u255a, s_u255b, s_u255c, s_u255d, s_u255e, s_u255f,
    s_u2560, s_u2561, s_u2562, s_u2563, s_u2564, s_u2565, s_u2566, s_u2567,
    s_u2568, s_u2569, s_u256a, s_u256b, s_u256c, s_u256d, s_u256e, s_u256f,
    s_u2570, s_u2571, s_u2572, s_u2573, s_u2574, s_u2575, s_u2576, s_u2577,
    s_u2578, s_u2579, s_u257a, s_u257b, s_u257c, s_u257d, s_u257e, s_u257f,
    s_u2580, s_u2581, s_u2582, s_u2583, s_u2584, s_u2585, s_u2586, s_u2587,
    s_u2588, s_u2589, s_u258a, s_u258b, s_u258c, s_u258d, s_u258e, s_u258f,
    s_u2590, s_u2591, s_u2592, s_u2593, s_u2594, s_u2595, s_u2596, s_u2597,
    s_u2598, s_u2599, s_u259a, s_u259b, s_u259c, s_u259d, s_u259e, s_u259f,
};

static inline int
normalWidth(const QSizeF &cellSize, bool bold)
{
    return (bold + 1) * (1 + (int)(cellSize.height() / U2500_WIDTH_INCREMENT));
}

static inline int
wideWidth(const QSizeF &cellSize, bool bold)
{
    return (bold + 3) * (1 + (int)(cellSize.height() / U2500_WIDTH_INCREMENT));
}

static void
setPen(QPainter &painter, const QPen &savedPen, int width)
{
    QPen pen(savedPen);
    pen.setWidth(width);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::MiterJoin);
    painter.setPen(pen);
}

static int
drawLines(QPainter &painter, const QSizeF &cellSize, const unsigned char *spec)
{
    int n = spec[1];
    spec += 2;

    QVector<QLineF> lines;

    for (int i = 0; i < n; ++i)
    {
        lines.push_back(QLineF(spec[0] * cellSize.width() / 120,
                               spec[1] * cellSize.height() / 120,
                               spec[2] * cellSize.width() / 120,
                               spec[3] * cellSize.height() / 120));
        spec += 4;
    }

    painter.drawLines(lines);
    return 2 + n * 4;
}

static int
drawPolyline(QPainter &painter, const QSizeF &cellSize, const unsigned char *spec)
{
    int n = spec[1];
    spec += 2;

    QVector<QPointF> points;

    for (int i = 0; i < n; ++i)
    {
        points.push_back(QPointF(spec[0] * cellSize.width() / 120,
                                 spec[1] * cellSize.height() / 120));
        spec += 2;
    }

    painter.drawPolyline(points.constData(), n);
    return 2 + n * 2;
}

static int
drawOffsetLines(QPainter &painter, const QSizeF &cellSize,
                int lineWidth, const unsigned char *spec)
{
    int n = spec[1];
    spec += 2;

    QVector<QLineF> lines;

    for (int i = 0; i < n; ++i)
    {
        int xoff = 0, yoff = 0;
        if (spec[0] & 1)
            yoff = -lineWidth;
        else if (spec[0] & 2)
            yoff = lineWidth;

        if (spec[0] & 4)
            xoff = -lineWidth;
        else if (spec[0] & 8)
            xoff = lineWidth;

        lines.push_back(QLineF(((char)spec[1] + 60) * cellSize.width() / 120 + xoff,
                               ((char)spec[2] + 60) * cellSize.height() / 120 + yoff,
                               ((char)spec[3] + 60) * cellSize.width() / 120 + xoff,
                               ((char)spec[4] + 60) * cellSize.height() / 120 + yoff));
        spec += 5;
    }

    painter.setClipRect(QRectF(0, 0, cellSize.width(), cellSize.height()));
    painter.drawLines(lines);
    painter.setClipping(false);
    return 2 + n * 5;
}

static int
drawOffsetPolyline(QPainter &painter, const QSizeF &cellSize,
                int lineWidth, const unsigned char *spec)
{
    int xoff = 0, yoff = 0;
    if (spec[2] & 1)
        yoff = -lineWidth;
    else if (spec[2] & 2)
        yoff = lineWidth;

    if (spec[2] & 4)
        xoff = -lineWidth;
    else if (spec[2] & 8)
        xoff = lineWidth;

    int n = spec[1];
    spec += 3;

    QVector<QPointF> points;

    for (int i = 0; i < n; ++i)
    {
        points.push_back(QPointF(((char)spec[0] + 60) * cellSize.width() / 120 + xoff,
                                 ((char)spec[1] + 60) * cellSize.height() / 120 + yoff));
        spec += 2;
    }

    painter.setClipRect(QRectF(0, 0, cellSize.width(), cellSize.height()));
    painter.drawPolyline(points.constData(), n);
    painter.setClipping(false);
    return 3 + n * 2;
}

static int
drawRoundedRect(QPainter &painter, const QSizeF &cellSize, const unsigned char *spec)
{
    qreal x = cellSize.width() / 2, y = cellSize.height() / 2;
    qreal radius = qMin(x, y);
    if (spec[1] & 1)
        y -= cellSize.height();
    if (spec[1] & 2)
        x -= cellSize.width();

    painter.setClipRect(QRectF(0, 0, cellSize.width(), cellSize.height()));
    QRectF rect(x, y, cellSize.width(), cellSize.height());
    painter.drawRoundedRect(rect, radius, radius);
    painter.setClipping(false);
    return 2;
}

static int
fillRects(QPainter &painter, const QSizeF &cellSize, const unsigned char *spec, QRgb bgval)
{
    int n = spec[1];
    spec += 2;

    painter.setClipRect(QRectF(0, 0, cellSize.width(), cellSize.height()));

    for (int i = 0; i < n; ++i)
    {
        QColor color = painter.pen().color();
        if (spec[0]) {
            color = Colors::blend0a(bgval, color.rgb(), spec[0]);
        }

        painter.fillRect(((char)spec[1] * cellSize.width()) / 120,
                         ((char)spec[2] * cellSize.height()) / 120,
                         (spec[3] * cellSize.width()) / 120,
                         (spec[4] * cellSize.height()) / 120,
                         color);
        spec += 5;
    }

    painter.setClipping(false);
    return 2 + n * 5;
}

namespace drawing
{

void
paintU2500(QPainter &painter, const DisplayCell &i, QSizeF cellSize, QRgb bgval)
{
    const QChar *raw = i.text.constData();
    int n = i.text.size();

    if (i.flags & Tsq::DblWidthChar)
        cellSize.rwidth() *= 2;

    const QPen savedPen = painter.pen();
    painter.save();
    painter.translate(i.rect.topLeft());

    for (int j = 0; j < n; ++j)
    {
        int val = raw[j].unicode();

        if (val >= 0x2500 && val <= 0x259f) {
            const unsigned char *spec = s_specs[val - 0x2500];

            while (1) {
                int lineWidth = (*spec & 8) ?
                    wideWidth(cellSize, i.flags & Tsq::Bold) :
                    normalWidth(cellSize, i.flags & Tsq::Bold);

                setPen(painter, savedPen, lineWidth);

                switch (*spec & 7) {
                default:
                    goto done;
                case nl:
                    spec += drawLines(painter, cellSize, spec);
                    break;
                case np:
                    spec += drawPolyline(painter, cellSize, spec);
                    break;
                case ol:
                    spec += drawOffsetLines(painter, cellSize, lineWidth, spec);
                    break;
                case op:
                    spec += drawOffsetPolyline(painter, cellSize, lineWidth, spec);
                    break;
                case re:
                    spec += fillRects(painter, cellSize, spec, bgval);
                    break;
                case al:
                    painter.setRenderHint(QPainter::Antialiasing, true);
                    spec += drawLines(painter, cellSize, spec);
                    painter.setRenderHint(QPainter::Antialiasing, false);
                    break;
                case rr:
                    painter.setRenderHint(QPainter::Antialiasing, true);
                    spec += drawRoundedRect(painter, cellSize, spec);
                    painter.setRenderHint(QPainter::Antialiasing, false);
                    break;
                }
            }
        }
    done:
        painter.translate(cellSize.width(), 0);
    }

    painter.restore();
}

}
