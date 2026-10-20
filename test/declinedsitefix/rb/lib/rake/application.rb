module Rake
  class Application
    include TaskManager
  end

  def self.application
    @application ||= Application.new
  end
end
