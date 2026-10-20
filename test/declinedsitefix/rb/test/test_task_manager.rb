require "minitest/autorun"

class TestTaskManager < Minitest::Test
  def setup
    @tm = make_manager
  end

  def test_create_task_manager
    @tm.define_task(Rake::Task, :t)
  end

  def test_define_two
    @tm.define_task(Rake::Task, :a)
    @tm.define_task(Rake::Task, :b)
  end
end
