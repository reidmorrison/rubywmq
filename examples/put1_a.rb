#
# Sample : put1() : Put a single message to a queue
#
require 'rubygems'
require 'wmq'

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.put(:q_name=>'TEST.QUEUE', :data => 'Hello World')
end
