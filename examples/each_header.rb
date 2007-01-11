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
# Sample : each() : Retrieve all messages from a queue
#          If no messages are on the queue, the program
#          completes without waiting
#
require 'wmq'

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.open_queue(:q_name=>'TEST.DEAD', :mode=>:browse) do |queue|
    queue.each do |message|
      puts "Data Received: #{message.data}"
      
      puts "Message Descriptor:"
      p message.descriptor
      
      puts "Headers Received:"
      message.headers.each {|header| p header}
    end
  end
  puts 'Completed.'
end
