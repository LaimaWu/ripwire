### Fixed — Windows: `--quality-delta=A..B` extracts its per-commit archive through stdin (#422, thanks @lennix1337)

- `--quality-delta=A..B` extracts its per-commit archive through stdin (`tar -x -f -`), the same Windows drive-colon fix as #402 (thanks @lennix1337).
