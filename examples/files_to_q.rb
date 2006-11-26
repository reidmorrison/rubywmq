################################################################################
#  Copyright 2006 J. Reid Morrison. Dimension Solutions, Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
################################################################################

#
# Sample : files_to_q : Place all files in a directory to a queue
#          Each file is written as a separate message
#
require 'find'
require 'yaml'

# Call program passing environment name as first parameter
#   The environment corresponds to an entry in the config file
#   Defaults to dev
env = ARGV[0] || raise("Command line argument 'environment' is required")
config = YAML::load_file('files_to_q.cfg')[env]
qmgr_options = config['qmgr_options']
if qmgr_options[:connection_name]
  require 'wmq/wmq_client'   # Force Client Connection (library)
else
  require 'wmq'
end

message = WMQ::Message.new
message.descriptor = config['descriptor'] || {}
tstart = Time.now
counter = 0
WMQ::QueueManager.connect(qmgr_options) do |qmgr|
  qmgr.open_queue({:mode=>:output}.merge(config['output_queue'])) do |queue|
    Find.find(config['source_directory']) do |path|
      unless FileTest.directory?(path)
        printf("%5d: #{path}\n",counter = counter + 1)
        message.data = File.read(path)
        queue.put({:message => message}.merge(config['put_options']))
      end
    end
  end
end
duration = Time.now - tstart
printf "Processed #{counter} messages in %.3f seconds. Average: %.3f messages/second\n", duration, counter/duration
