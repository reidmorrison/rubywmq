#
# Sample : put() : Put a message to a queue with a Refernce header
#          Open the queue so that multiple puts can be performed
#
require 'wmq'

# The Rules Format header (MQRFH) allows a list of name value pairs to be sent along
# with a WMQ message. These name value pairs are represented as follows on the "wire":
#   Name1 Value1 Name2 Value2 Name3 Value3
#
# Ruby WMQ converts the above string of data into a Ruby hash by
# using the name as the key, as follows:
# data = {
#    'Name1' => 'Value1',
#    'Name2' => 'Value2',
#    'Name3' => 'Value3'
# }
#
# Since a name can consist of any character except null, it is stored as a String
#
# Note: It is possible to send or receive the same Name with multiple values using an array.
#   E.g.   Name1 Value1 Name2 Value2 Name1 Value3
#   Becomes:
# data = {
#    'Name1' => ['Value1', 'Value3'],
#    'Name2' => 'Value2'
# }
#
# Note: Since a Hash does not preserve order, reading a Rules Format Header and then writing
#       it out immediately again could result in re-ordering of the name value pairs.
#

WMQ::QueueManager.connect(q_mgr_name: 'REID') do |qmgr|
  qmgr.open_queue(q_name: 'TEST.QUEUE', mode: :output) do |queue|
    message      = WMQ::Message.new
    message.data = 'Hello World'

    message.headers = [
      {
        header_type: :rf_header,
        name_value:  {
          'name1' => 'value1',
          'name2' => 'value2',
          'name3' => ['value 3a', 'value 3b']
        }
      }
    ]

    message.descriptor[:format] = WMQ::MQFMT_STRING

    queue.put(message: message)
  end
end
