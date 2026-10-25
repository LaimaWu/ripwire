### Fixed — Django and unittest hooks no longer read as dead code

Before: `--quality-delta` reported a Python hook as dead when no call in the tree named it. unittest and Django call
`setUp`, `tearDown`, `setUpClass` and `setUpTestData` themselves. The runner calls each `test*` method. A closure inside a
method that called `self.hook()` also lost the call. Each such method appeared as dead code.

Now: the tool treats these names as called. It covers `setUpModule`, `tearDownModule`, `load_tests`, `runTest`, `asyncSetUp` and `asyncTearDown` too. A `test*`
method (any name that starts with `test`) counts as called only in a test module: a path `isTestPath` accepts, or a Django `tests.py`. A closure
that calls `self.` or `cls.` keeps the overrides of the method it calls live. A closure that binds `self` or `cls` as its own
parameter does not.

The HEAD snapshot scheme moves from 22 to 23. Old cached snapshots are not used.

Limit: the name check ignores types and scope. A method named `setUp`, `tearDown` or `load_tests` in application code is also treated as called. In a test module, any def named `test*` counts as called, nested defs and helper functions included. The tool errs toward live here.

A closure that binds `self` or `cls` as its own parameter does not count, even for `def cb(self=self)`.
