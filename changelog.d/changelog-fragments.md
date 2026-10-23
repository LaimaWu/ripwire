### Changed — contributors: a changelog entry is one new file under `changelog.d/`, not an edit to this file

Every merge to main used to leave every open pull request conflicting on this file's `[Unreleased]` section, because
each one appended to the same lines. A change now adds one file, `changelog.d/<pr-number-or-slug>.md`, holding its
entry exactly as it will read here (format: `changelog.d/README.md`); `scripts/changelog_assemble.py` folds the files
under the new version's heading when a release is cut, and the release workflow refuses a tag while any are left.
`test/changelogfragcheck.sh` validates the fragments (known section, one change per file, no private paths, credit
form), refuses an entry written under `[Unreleased]` with a pointer to the fragment to write instead, and pins the
fold against a golden. The entries already under `[Unreleased]` moved into fragments byte for byte. No output change.
