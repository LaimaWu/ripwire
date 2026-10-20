require "minitest/autorun"

class TestNameSpace < Minitest::Test
  def test_namespace_lookup
    mgr = fresh_manager
    mgr.define_task(Rake::Task, "n:t")
  end
end
