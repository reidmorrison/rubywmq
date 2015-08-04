#
# Example : files_to_q : Place all files in a directory to a queue
#          Each file is written as a separate message
#
require 'find'
require 'yaml'
require 'wmq'

# Call program passing environment name as first parameter
#   The environment corresponds to an entry in the config file
env    = ARGV[0] || fail('Command line argument `environment` is required')
config = YAML::load_file('files_to_q.cfg')[env]

message            = WMQ::Message.new
message.descriptor = config['descriptor'] || {}
tstart             = Time.now
counter            = 0
WMQ::QueueManager.connect(config['qmgr_options']) do |qmgr|
  qmgr.open_queue({ mode: :output }.merge(config['output_queue'])) do |queue|
    Find.find(config['source_directory']) do |path|
      unless FileTest.directory?(path)
        printf("%5d: #{path}\n", counter = counter + 1)
        message.data = File.read(path)
        queue.put({ message: message }.merge(config['put_options']))
      end
    end
  end
end
duration = Time.now - tstart
printf "Processed #{counter} messages in %.3f seconds. Average: %.3f messages/second\n", duration, counter/duration
