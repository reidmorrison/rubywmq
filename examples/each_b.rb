#
# Sample : each() : Retrieve all messages from a queue that
#          have the same correlation id
#
require 'rubygems'
require 'wmq'

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:input) do |queue|
    message = WMQ::Message.new

    # Retrieve First message
    if queue.get(:message=>message)
      puts "Data Received: #{message.data}"

      #Retrieve all subsequent messages that have the same correlation id
      queue.each(:message=>message, :match=>WMQ::MQMO_MATCH_CORREL_ID) do |msg|
        puts "Matching Data Received: #{msg.data}"
      end
    else
      puts 'No message available'
    end
  end
  puts 'Completed.'
end
