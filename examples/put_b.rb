#
# Sample : put() : Put two Messages to a queue
#          Open the queue so that multiple puts can be performed
#          Ensure that all messages have the same correlation id
#
require 'rubygems'
require 'wmq'

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:output) do |queue|

    message = WMQ::Message.new

    # First message
    #   Results in a WMQ generated msg_id and empty correl_id
    message.data = 'Hello World'
    queue.put(:message => message)

    # Second message
    #   new_msg_id will cause the second message to have a new message id
    #   otherwise both messages will have the same message and correlation id's
    #   This message will have the same correlation id as the first message (empty)
    message.data = 'Hello Again'
    p message.descriptor
    queue.put(:message => message, :new_msg_id => true)
  end
end
