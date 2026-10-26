### Fixed — Django and unittest hooks no longer read as dead code (#437, thanks @llvm-x86)

Before: `--quality-delta` reported a Python hook as dead when no call in the tree named it. unittest and Django call
`setUp`, `tearDown`, `setUpClass` and `setUpTestData` themselves. The runner calls each `test*` method. A closure inside a
method that called `self.hook()` also lost the call. Each such method appeared as dead code.

Now: the tool treats these names as called. It covers `setUpModule`, `tearDownModule`, `load_tests`, `runTest`, `asyncSetUp` and `asyncTearDown` too. The pytest xunit hooks `setup_module`, `teardown_module`,
`setup_function`, `teardown_function`, `setup_class`, `teardown_class`, `setup_method` and `teardown_method` count as called too. A `test*`
method (any name that starts with `test`) and an xunit hook count as called only in a test module: a path `isTestPath` accepts, or a Django `tests.py`. A closure
that calls `self.` or `cls.` keeps the overrides of the method it calls live. A closure that binds `self` or `cls` as its own
parameter does not.

The HEAD snapshot scheme moves from 22 to 24. Old cached snapshots are not used.

Not covered, still reported: pytest's conftest hooks such as `pytest_configure`. (`@pytest.fixture` functions are not reported as dead code here.)

The hook names live in one table per framework (`kUnittestHooks`, `kXunitHooks`, `kUnittestModuleHooks` in `quality.h`), shared with `--dead-code` and `--safe-delete`, which now also skip `setUpTestData` and `runTest` in a TestCase subclass.

Limit: the name check ignores types and scope. A method named `setUp`, `tearDown` or `load_tests` in application code is also treated as called. In a test module, any def named `test*` counts as called, nested defs and helper functions included. The tool errs toward live here.

A closure that binds `self` or `cls` as its own parameter does not count, even for `def cb(self=self)`.
