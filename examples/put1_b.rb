#
# Sample : put1() : Put a single message to a queue
#
#          Set the correlation id to a text string
#
require 'rubygems'
require 'wmq'

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|

  message = WMQ::Message.new
  message.data = 'Hello World'
  message.descriptor[:correl_id] = 'My first message'

  qmgr.put(:q_name=>'TEST.QUEUE', :message=>message)
end

