Force allocation regression checks
==================================

Run from the repository root:

```
cmake -S tests/force-allocation -B build/force-allocation-check
cmake --build build/force-allocation-check --config Release
ctest --test-dir build/force-allocation-check -C Release --output-on-failure
```

The harness compiles the current production cost table, validator, allocation
reader, template loader, menu entry point, refresh function, and command forwarding functions.
Engine imports and menu rendering are mocked. The menu-paint mock invokes the
same force validation used by the show-all-force ownerdraw.

Checks cover:

- 20,000 light/dark allocations with paid and free sabers, including fully spent
  builds: UI totals agree with validation and repeated validation preserves ranks.
- Console free-saber changes recalculate totals and preserve affordable ranks;
  turning freebies off still reduces a genuinely over-budget build.
- A simultaneous free-saber/rank change applies the new budget before menu
  validation can trim and save the build. This failed before the ordering fix.
- Server-triggered player/force menu opening processes pending rules even when
  the next refresh has not run yet.
- Template/custom switching and repeated saved-allocation reads preserve a
  fully spent build under unchanged rules.
- Both `forcechanged` forwarding paths send pending userinfo first, preserve
  unrelated flags, and avoid duplicate userinfo. This failed before the fix.

These are deterministic function-level checks, not a live game/network test.
Startup cvar registration, server event delivery, reconnect timing, and third-party
server rules still require in-game verification. The tests do not promise that
every rule transition preserves ranks: a lower budget or paid saber costs can
make the old allocation invalid.
