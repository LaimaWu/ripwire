module Rake
  module TaskManager
    def define_task(task_class, *args, &block)
      task_class.new(args.first)
    end

    def synthesize_file_task(task_name)
      define_task(Rake::FileTask, task_name)
    end
  end
end
