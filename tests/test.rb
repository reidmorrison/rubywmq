# Shift include path to use locally built copy of rubywmq - For testing dev builds only
#$:.unshift '../lib'

require 'rubygems'
require 'wmq/wmq'
require 'wmq/wmq_const_admin'
require 'test/unit'
class TestTest < Test::Unit::TestCase

  def setup
    puts '****** setup: start ******'
    @queue_manager = WMQ::QueueManager.new(:q_mgr_name => 'REID') #, :connection_name=>'localhost(1414)')
    @queue_manager.connect

    # Create Queue and clear any messages from the queue
    @in_queue = WMQ::Queue.new(:queue_manager=>@queue_manager,
                               :mode=>:input,
                               :dynamic_q_name=>'UNIT.TEST.*',
                               :q_name=>'SYSTEM.DEFAULT.MODEL.QUEUE',
                               :fail_if_exists=>false)
    @in_queue.open
    @in_queue.each { |message| }

    # Open same queue for Output. Queue should be created by now
    @out_queue = WMQ::Queue.new(:queue_manager=>@queue_manager,
                                :mode=>:output,
                                :q_name=>@in_queue.name)
    @out_queue.open
    puts '****** setup: end ******'
  end

  def teardown
    puts '****** teardown: start ******'
    @out_queue.close
    @in_queue.close
    @queue_manager.disconnect
    puts '****** teardown: end ******'
  end

  def test_exceptions
    puts '****** test_exceptions: start ******'
    assert_raise(TypeError) { WMQ::QueueManager.new(1) }
    assert_nothing_raised   { WMQ::QueueManager.new(:exception_on_error=>nil) }
    assert_raise(TypeError) { WMQ::QueueManager.new(:exception_on_error=>1) }
    assert_raise(TypeError) { WMQ::QueueManager.new(:q_mgr_name=>2).connect }
    assert_raise(WMQ::WMQException) { WMQ::QueueManager.new(:q_mgr_name=>'bad').connect }
    puts '****** test_exceptions: end ******'
  end

  def test_queue_manager
    puts '****** test_queue_manager ******'
    assert_equal(Object, WMQ::QueueManager.superclass)
    assert_equal(WMQ::QueueManager, @queue_manager.class)
  end

  def test_1
    puts '****** test_1 ******'
    assert_equal(@out_queue.put(:data=>'Hello World'), true)
    message = WMQ::Message.new
    assert_equal(@in_queue.get(:message=>message), true)
    assert_equal(message.data, 'Hello World')
  end

  def test_dynamic_buffer
    puts '****** test_dynamic_buffer ******'
    # Clear out queue of any messages
    @in_queue.each { |message| }

    test_sizes = [0, 1, 100, 101, 102, 500, 65534, 65535, 65536, 65537, 1*1024*1024, 4*1024*1024]
    test_sizes.each do |size|
      str = '0123456789ABCDEF' * (size/16) + '0123456789ABCDEF'[0,size%16]
      assert_equal(str.length, size)
      assert_equal(@out_queue.put(:data=>str), true)
    end

    # First test the browse mechanism
    counter = 0
    @queue_manager.open_queue(:mode=>:browse, :q_name=>@in_queue.name) do |browse_queue|
      browse_queue.each do |message|
        size = test_sizes[counter]
        assert_equal(size, message.data.length)
        str = '0123456789ABCDEF' * (size/16) + '0123456789ABCDEF'[0,size%16]
        assert_equal(str, message.data)
        counter = counter + 1
      end
    end
    assert_equal(test_sizes.size, counter)

    # Now retrieve the messages destructively
    message = WMQ::Message.new
    test_sizes.each do |size|
      assert_equal(true, @in_queue.get(:message=>message, :match=>WMQ::MQMO_NONE))
      assert_equal(size, message.data.length)
      str = '0123456789ABCDEF' * (size/16) + '0123456789ABCDEF'[0,size%16]
      assert_equal(str, message.data)
    end
  end

  def test_q_name_hash
    puts '****** test_q_name_hash ******'
    @queue_manager.open_queue(:mode=>:output,
                              :q_name=>{:q_name=>@in_queue.name,
                                        :q_mgr_name=>@queue_manager.name}
                             ) do |test_queue|
      assert_equal(true, test_queue.put(:data=>'Hello World'))
      message = WMQ::Message.new
      assert_equal(true, @in_queue.get(:message=>message))
      assert_equal('Hello World', message.data)
    end
  end

  def test_execute
    puts '****** test_execute ******'
    array = @queue_manager.inquire_q(:q_name=>@in_queue.name, :q_type=>WMQ::MQQT_LOCAL, :current_q_depth=>nil)
    assert_equal(1, array.size)
    assert_equal(@in_queue.name, array[0][:q_name])

    assert_equal(true, @queue_manager.inquire_process(:process_name=>'*').size>0)
    assert_raise(WMQ::WMQException) { @queue_manager.inquire_q(:q_name=>'BADQUEUENAME*') }
    assert_equal(1, @queue_manager.ping_q_mgr.size)
  end

  def test_mqsc
    puts '****** test_mqsc ******'
    array = @queue_manager.mqsc("dis ql(#{@in_queue.name})")
    assert_equal(1, array.size)
    assert_equal(true, array[0].include?("QUEUE(#{@in_queue.name})"))
  end

  def test_put1
    puts '****** test_put1 ******'
    data = 'Some Test Data'
    assert_equal(true,@queue_manager.put(:q_name=>@in_queue.name, :data=>data))

    message = WMQ::Message.new
    assert_equal(true, @in_queue.get(:message=>message))
    assert_equal(data, message.data)
  end

  def verify_header(header, format)
    verify_multiple_headers([header], format)
  end

  def verify_multiple_headers(headers, format)
    data = 'Some Test Data'
    message = WMQ::Message.new
    message.data = data
    message.descriptor[:format] = format
    message.headers = headers
    #assert_equal(true,@queue_manager.put(:q_name=>@in_queue.name, :message=>message))
    assert_equal(true,@out_queue.put(:message=>message))

    message = WMQ::Message.new
    assert_equal(true, @in_queue.get(:message=>message))
    assert_equal(data, message.data)
    assert_equal(headers.size, message.headers.size)
    count = 0
    headers.each do |header|
      reply_header = message.headers[count]
      header.each_pair{|key, value| assert_equal(value, reply_header[key])}
      count = count + 1
    end
  end
  
  def test_dlh
    puts '****** test_dlh ******'
    dlh = {:header_type     =>:dead_letter_header,
           :reason          => WMQ::MQRC_UNKNOWN_REMOTE_Q_MGR,
           :dest_q_name     =>'ORIGINAL_QUEUE_NAME',
           :dest_q_mgr_name =>'BAD_Q_MGR',
           :put_appl_name   =>'TestApp.exe',
           }

    verify_header(dlh, WMQ::MQFMT_DEAD_LETTER_HEADER)
    # Untested keys:
    #:put_date=>"",
    #:put_time=>"",
    #:encoding=>0,
    #:coded_char_set_id=>437,
    #:put_appl_type=>0,

    # Test again, but use QueueManager#put this time
    data = 'Some Test Data'
    message = WMQ::Message.new
    message.data = data
    message.descriptor[:format] = WMQ::MQFMT_STRING
    message.headers << dlh
    assert_equal(true,@queue_manager.put(:q_name=>@in_queue.name, :message=>message))

    message = WMQ::Message.new
    assert_equal(true, @in_queue.get(:message=>message))
    assert_equal(data, message.data)
    assert_equal(1, message.headers.size)
    reply_header = message.headers[0]

    dlh.each_pair{|key, value| assert_equal(value, reply_header[key])}
  end

  def test_cics
    puts '****** test_cics ******'
    cics = {:header_type     =>:cics,
            :reason          =>WMQ::MQRC_UNKNOWN_REMOTE_Q_MGR,
            :facility        =>'TOKEN123',
            :reply_to_format =>WMQ::MQFMT_STRING,
            }
    verify_header(cics, WMQ::MQFMT_NONE)
  end
  
  def test_ims
    puts '****** test_ims ******'
    ims = {:header_type      =>:ims,
           :l_term_override  =>'LTERM',
           :reply_to_format  =>WMQ::MQFMT_STRING,
          }
    verify_header(ims, WMQ::MQFMT_STRING)
  end
  
  def test_transmission_header
    puts '****** test_transmission_header ******'
    xqh = {:header_type     =>:xmit_q_header,
           :remote_q_name   =>'SOME_REMOTE_QUEUE',
           :remote_q_mgr_name=>'SOME_REMOTE_Q_MGR',
           :msg_type        =>WMQ::MQMT_REQUEST,
           :msg_id          =>'my message Id',
          }
    verify_header(xqh, WMQ::MQFMT_STRING)
  end
  
  def test_rf_header
    puts '****** test_rf_header ******'
    rfh = {:header_type =>:rf_header,
           :name_value  => {' name   s' => '  v a "l" u e 1  ', 
                            'n a m e 2 ' => 'v a l u e  2', 
                            '' => ['"', '""', '"""', '""""', ''],
                            'name3'=>['"value3"', '', '"',' value 43"']},
          }
    verify_header(rfh, WMQ::MQFMT_STRING)
  end
  
  def test_rf_header_2
    puts '****** test_rf_header_2 ******'
    rfh2 = {:header_type =>:rf_header_2,
            :xml => ['<hello>to the world</hello>', 
                     '<another>xml like string</another>'],
           }
    verify_header(rfh2, WMQ::MQFMT_STRING)
  end
  
  def test_multiple_headers
    puts '****** test_multiple_headers ******'
    headers = [{:header_type      => :rf_header_2,
                :xml              => ['<hello>to the world</hello>', 
                                      '<another>xml like string</another>'],},
                
               {:header_type      => :rf_header,
                :name_value       => {' name   s' => '  v a l u e 1  ', 
                                      'n a m e 2 ' => 'v a l u e  2', 
                                      'name3'=>['value3', '', 'value 43']} },
                
               {:header_type      => :ims,
                :l_term_override  => 'LTERM',
                :reply_to_format  => WMQ::MQFMT_STRING},
                
               {:header_type      => :rf_header,
                :name_value       => {' name   s' => '  v a l u e 1  ', 
                                      'n a m e 2 ' => 'v a l u e  2', 
                                      'name3'=>['value3', '', 'value 43']} },

               {:header_type      => :cics,
                :reason           => WMQ::MQRC_UNKNOWN_REMOTE_Q_MGR,
                :facility         => 'TOKEN123',
                :reply_to_format  => WMQ::MQFMT_STRING},

               {:header_type      => :rf_header_2,
                :xml              => ['<hello>to the world</hello>', '<another>xml like string</another>'],},
                
               {:header_type      => :xmit_q_header,
                :remote_q_name    => 'SOME_REMOTE_QUEUE',
                :remote_q_mgr_name=> 'SOME_REMOTE_Q_MGR',
                :msg_type         => WMQ::MQMT_REQUEST,
                :msg_id           => 'my message Id'},
              ]              
    verify_multiple_headers(headers, WMQ::MQFMT_STRING)
  end
  
  def test_xmit_multiple_headers
    puts '****** test_xmit_q_header with ims header ******'
    headers = [{:header_type      => :xmit_q_header,
                :remote_q_name    => 'SOME_REMOTE_QUEUE',
                :remote_q_mgr_name=> 'SOME_REMOTE_Q_MGR',
                :msg_type         => WMQ::MQMT_REQUEST,
                :msg_id           => 'my message Id'},
                
               {:header_type      => :ims,
                :l_term_override  => 'LTERM',
                :reply_to_format  => WMQ::MQFMT_STRING},
              ]
    verify_multiple_headers(headers, WMQ::MQFMT_STRING)
  end
  
  def test_message_grouping
    puts '****** test_message_grouping ******'
    # Clear out queue of any messages
    @in_queue.each { |message| }
    
    msg = WMQ::Message.new
    msg.data = 'First'
    msg.descriptor[:msg_flags] = WMQ::MQMF_MSG_IN_GROUP
    assert_equal(@out_queue.put(:message=>msg, :options => WMQ::MQPMO_LOGICAL_ORDER), true)
    
    msg.data = 'Second'
    assert_equal(@out_queue.put(:message=>msg, :options => WMQ::MQPMO_LOGICAL_ORDER), true)
    
    msg.data = 'Last'
    msg.descriptor[:msg_flags] = WMQ::MQMF_LAST_MSG_IN_GROUP
    assert_equal(@out_queue.put(:message=>msg, :options => WMQ::MQPMO_LOGICAL_ORDER), true)

    # Now retrieve the messages destructively
    message = WMQ::Message.new
    test_sizes.each do |size|
      assert_equal(true, @in_queue.get(:message=>message, :match=>WMQ::MQMO_NONE))
      assert_equal(size, message.data.length)
      str = '0123456789ABCDEF' * (size/16) + '0123456789ABCDEF'[0,size%16]
      assert_equal(str, message.data)
    end
  end
  
end
