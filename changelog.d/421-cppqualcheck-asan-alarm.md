### Fixed — test infrastructure: `cppqualcheck` gives a Linux ASan binary the sanitizer alarm (#421)

The gate scales its per-call alarm (30 s plain, 90 s sanitizer) from the binary's flavour, which it read only off the
binary's undefined symbols. Linux clang links the sanitizer runtime statically, so every ubuntu ASan run was read as
plain and its one cold whole-repo call (22-30 s under ASan) timed out twice in nightly CI. The flavour now also comes
from the runtime's symbol names inside the binary or from the runtime answering `ASAN_OPTIONS=help=1`; the alarms
themselves, every arm and every literal are unchanged. Test-only; no output change.
