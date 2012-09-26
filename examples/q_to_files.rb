#
# Example: q_to_files:  Copy all messages in a queue to separate files in a directory
#
require 'rubygems'
require 'find'
require 'yaml'
require 'wmq'
require 'fileutils'

# Call program passing environment name as first parameter
#   The environment corresponds to an entry in the config file
env = ARGV[0] || raise("Command line argument 'environment' is required")
config = YAML::load_file('q_to_files.cfg')[env]

# Create supplied path if it does not exist
path = config['target_directory']
FileUtils.mkdir_p(path)

message = WMQ::Message.new
message.descriptor = config['descriptor'] || {}
tstart = Time.now
counter = 0
WMQ::QueueManager.connect(config['qmgr_options']) do |qmgr|
  qmgr.open_queue(config['input_queue']) do |queue|
    queue.each do |message|
      counter = counter + 1
      File.open(File.join(path, "message_%03d" % counter), 'w') {|file| file.write(message.data) }
    end
  end
end
duration = Time.now - tstart
printf "Processed #{counter} messages in %.3f seconds. Average: %.3f messages/second\n", duration, counter/duration
