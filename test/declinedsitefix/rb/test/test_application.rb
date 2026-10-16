require "minitest/autorun"

class TestApplication < Minitest::Test
  def test_load_rakefile
    @app.define_task(Rake::Task, :default)
  end

  def test_top_level
    @app.define_task(Rake::FileTask, "a.o")
  end
end
