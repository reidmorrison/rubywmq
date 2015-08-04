#
# Sample : put() : Put a single message to a queue
#          Open the queue so that multiple puts can be performed
#
require 'wmq'

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:output) do |queue|

    # First message
    queue.put(:data => 'Hello World')

    # Second message
    #   This message will have new message and correlation id's
    queue.put(:data => 'Hello Again')
  end
end
