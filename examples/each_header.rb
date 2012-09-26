#
# Sample : each() : Retrieve all messages from a queue
#          If no messages are on the queue, the program
#          completes without waiting
#
require 'rubygems'
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
