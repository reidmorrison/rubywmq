#
# Sample : get() : Retrieve a single message from a queue
#          If no messages are on the queue, message.data is nil
#
require 'wmq'

WMQ::QueueManager.connect(q_mgr_name: 'REID') do |qmgr|
  qmgr.open_queue(q_name: 'TEST.QUEUE', mode: :input) do |queue|
    message = WMQ::Message.new
    if queue.get(message: message)
      puts "Data Received: #{message.data}"
    else
      puts 'No message available'
    end
  end
end

