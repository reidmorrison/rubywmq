#
# Sample : put() : Put a message to a queue with a Refernce header
#          Open the queue so that multiple puts can be performed
#
require 'rubygems'
require 'wmq'

# The Rules Format header2 (MQRFH2) allows a an XML-like string to be passed as a header
# to the data.
#

WMQ::QueueManager.connect(:q_mgr_name=>'REID') do |qmgr|
  qmgr.open_queue(:q_name=>'TEST.QUEUE', :mode=>:output) do |queue|
    message = WMQ::Message.new
    message.data = 'Hello World'

    message.headers = [
     {:header_type =>:rf_header_2,
      :xml => '<hello>to the world</hello>'
     }]

    message.descriptor[:format] = WMQ::MQFMT_STRING

    queue.put(:message=>message)
  end
end

