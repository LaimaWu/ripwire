### Changed — tests: legend byte pins are stair-step targets under a runaway guard

- The five legend size gates keep every pinned number as a target; a registered present-only clause may pass it (printed `OVER-TARGET`), a hard runaway guard fails as a bug signal, and `RIPWIRE_LEGEND_PINS=strict` keeps the old hard pins (tests only; no output change).
