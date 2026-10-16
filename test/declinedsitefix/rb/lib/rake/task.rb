module Rake
  class Task
    def initialize(name)
      @name = name
    end

    class << self
      def define_task(*args, &block)
        Rake.application.define_task(self, *args, &block)
      end
    end
  end

  class FileTask < Task
  end
end
