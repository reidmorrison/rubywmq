require_relative 'test_helper'

# Unit Test for RocketJob::Job
class WMQTest < Minitest::Test
  context WMQ do
    setup do
      @queue_manager = WMQ::QueueManager.new(q_mgr_name: 'TEST') #, connection_name: 'localhost(1414)')
      @queue_manager.connect

      # Create Queue and clear any messages from the queue
      @in_queue = WMQ::Queue.new(
        queue_manager:  @queue_manager,
        mode:           :input,
        dynamic_q_name: 'UNIT.TEST.*',
        q_name:         'SYSTEM.DEFAULT.MODEL.QUEUE',
        fail_if_exists: false
      )
      @in_queue.open
      @in_queue.each { |message|}

      # Open same queue for Output. Queue should be created by now
      @out_queue = WMQ::Queue.new(
        queue_manager: @queue_manager,
        mode:          :output,
        q_name:        @in_queue.name
      )
      @out_queue.open
    end

    teardown do
      @out_queue.close if @out_queue
      @in_queue.close if @in_queue
      @queue_manager.disconnect if @queue_manager
    end

    context 'exceptions' do
      should 'raise exceptions' do
        assert_raises(TypeError) do
          WMQ::QueueManager.new(1)
        end
        WMQ::QueueManager.new(exception_on_error: nil)
        assert_raises(TypeError) do
          WMQ::QueueManager.new(exception_on_error: 1)
        end
        assert_raises(TypeError) do
          WMQ::QueueManager.new(q_mgr_name: 2).connect
        end
        assert_raises(WMQ::WMQException) do
          WMQ::QueueManager.new(q_mgr_name: 'bad').connect
        end
      end
    end

    context 'queue manager' do
      should 'exist' do
        assert_equal Object, WMQ::QueueManager.superclass
        assert_equal WMQ::QueueManager, @queue_manager.class
      end

      should 'open_queue' do
        @queue_manager.open_queue(
          mode:   :output,
          q_name: {
            q_name:     @in_queue.name,
            q_mgr_name: @queue_manager.name
          }
        ) do |test_queue|
          assert_equal(true, test_queue.put(data: 'Hello World'))
          message = WMQ::Message.new
          assert_equal(true, @in_queue.get(message: message))
          assert_equal('Hello World', message.data)
        end
      end

      should 'execute' do
        array = @queue_manager.inquire_q(
          q_name:          @in_queue.name,
          q_type:          WMQ::MQQT_LOCAL,
          current_q_depth: nil
        )
        assert_equal 1, array.size
        assert_equal @in_queue.name, array[0][:q_name]

        assert_equal true, @queue_manager.inquire_process(process_name: '*').size > 0
        assert_raises WMQ::WMQException do
          @queue_manager.inquire_q(q_name: 'BADQUEUENAME*')
        end
        assert_equal 1, @queue_manager.ping_q_mgr.size
      end

      should 'mqsc' do
        array = @queue_manager.mqsc("dis ql(#{@in_queue.name})")
        assert_equal 1, array.size
        assert_equal true, array[0].include?("QUEUE(#{@in_queue.name})")
      end

      should 'put1' do
        data = 'Some Test Data'
        assert_equal true, @queue_manager.put(q_name: @in_queue.name, data: data)

        message = WMQ::Message.new
        assert_equal true, @in_queue.get(message: message)
        assert_equal data, message.data
      end

    end

    context 'Queue' do
      should 'send and receive message' do
        assert_equal @out_queue.put(data: 'Hello World'), true
        message = WMQ::Message.new
        assert_equal @in_queue.get(message: message), true
        assert_equal message.data, 'Hello World'
      end

      should 'group messages' do
        # Clear out queue of any messages
        @in_queue.each { |message|}

        msg                        = WMQ::Message.new
        msg.data                   = 'First'
        msg.descriptor[:msg_flags] = WMQ::MQMF_MSG_IN_GROUP
        assert_equal(@out_queue.put(message: msg, options: WMQ::MQPMO_LOGICAL_ORDER), true)

        msg.data = 'Second'
        assert_equal(@out_queue.put(message: msg, options: WMQ::MQPMO_LOGICAL_ORDER), true)

        msg.data                   = 'Last'
        msg.descriptor[:msg_flags] = WMQ::MQMF_LAST_MSG_IN_GROUP
        assert_equal(@out_queue.put(message: msg, options: WMQ::MQPMO_LOGICAL_ORDER), true)
      end
    end

    context 'Message' do
      should 'dynamic_buffer' do
        test_sizes = [0, 1, 100, 101, 102, 500, 65534, 65535, 65536, 65537, 1*1024*1024, 4*1024*1024]
        test_sizes.each do |size|
          str = '0123456789ABCDEF' * (size/16) + '0123456789ABCDEF'[0, size%16]
          assert_equal str.length, size
          assert_equal @out_queue.put(data: str), true
        end

        # First test the browse mechanism
        counter = 0
        @queue_manager.open_queue(mode: :browse, q_name: @in_queue.name) do |browse_queue|
          browse_queue.each do |message|
            size = test_sizes[counter]
            assert_equal(size, message.data.length)
            str = '0123456789ABCDEF' * (size/16) + '0123456789ABCDEF'[0, size%16]
            assert_equal(str, message.data)
            counter = counter + 1
          end
        end
        assert_equal(test_sizes.size, counter)

        # Now retrieve the messages destructively
        message = WMQ::Message.new
        test_sizes.each do |size|
          assert_equal(true, @in_queue.get(message: message, match: WMQ::MQMO_NONE))
          assert_equal(size, message.data.length)
          str = '0123456789ABCDEF' * (size/16) + '0123456789ABCDEF'[0, size%16]
          assert_equal(str, message.data)
        end
      end

      should 'dead letter header' do
        # TODO: Something has changed with DLH since MQ V6
        skip
        dlh = {
          header_type:     :dead_letter_header,
          reason:          WMQ::MQRC_UNKNOWN_REMOTE_Q_MGR,
          dest_q_name:     'ORIGINAL_QUEUE_NAME',
          dest_q_mgr_name: 'BAD_Q_MGR',
          put_appl_name:   'TestApp.exe',
        }

        verify_header(dlh, WMQ::MQFMT_DEAD_LETTER_HEADER)
      end

      should 'cics' do
        cics = {
          header_type:     :cics,
          reason:          WMQ::MQRC_UNKNOWN_REMOTE_Q_MGR,
          facility:        'TOKEN123',
          reply_to_format: WMQ::MQFMT_STRING
        }
        verify_header(cics, WMQ::MQFMT_NONE)
      end

      should 'ims' do
        ims = {
          header_type:     :ims,
          l_term_override: 'LTERM',
          reply_to_format: WMQ::MQFMT_STRING
        }
        verify_header(ims, WMQ::MQFMT_STRING)
      end

      should 'transmission_header' do
        xqh = {
          header_type:       :xmit_q_header,
          remote_q_name:     'SOME_REMOTE_QUEUE',
          remote_q_mgr_name: 'SOME_REMOTE_Q_MGR',
          msg_type:          WMQ::MQMT_REQUEST,
          msg_id:            'my message Id'
        }
        verify_header(xqh, WMQ::MQFMT_STRING)
      end

      should 'rf_header' do
        rfh = {
          header_type: :rf_header,
          name_value:  {
            ' name   s'  => '  v a "l" u e 1  ',
            'n a m e 2 ' => 'v a l u e  2',
            ''           => ['"', '""', '"""', '""""', ''],
            'name3'      => ['"value3"', '', '"', ' value 43"']
          }
        }
        verify_header(rfh, WMQ::MQFMT_STRING)
      end

      should 'rf_header_2' do
        rfh2 = {
          header_type: :rf_header_2,
          xml:         [
                         '<hello>to the world</hello>',
                         '<another>xml like string</another>'
                       ],
        }
        verify_header(rfh2, WMQ::MQFMT_STRING)
      end

      should 'multiple_headers' do
        headers = [
          {header_type: :rf_header_2,
            xml:        [
                          '<hello>to the world</hello>',
                          '<another>xml like string</another>'],
          },

          {
            header_type: :rf_header,
            name_value:  {
              ' name   s'  => '  v a l u e 1  ',
              'n a m e 2 ' => 'v a l u e  2',
              'name3'      => ['value3', '', 'value 43']
            }
          },

          {
            header_type:     :ims,
            l_term_override: 'LTERM',
            reply_to_format: WMQ::MQFMT_STRING
          },

          {
            header_type: :rf_header,
            name_value:  {
              ' name   s'  => '  v a l u e 1  ',
              'n a m e 2 ' => 'v a l u e  2',
              'name3'      => ['value3', '', 'value 43']
            }
          },

          {
            header_type:     :cics,
            reason:          WMQ::MQRC_UNKNOWN_REMOTE_Q_MGR,
            facility:        'TOKEN123',
            reply_to_format: WMQ::MQFMT_STRING
          },

          {
            header_type: :rf_header_2,
            xml:         ['<hello>to the world</hello>', '<another>xml like string</another>'],
          },

          {
            header_type:       :xmit_q_header,
            remote_q_name:     'SOME_REMOTE_QUEUE',
            remote_q_mgr_name: 'SOME_REMOTE_Q_MGR',
            msg_type:          WMQ::MQMT_REQUEST,
            msg_id:            'my message Id'
          },
        ]
        verify_multiple_headers(headers, WMQ::MQFMT_STRING)
      end

      should 'xmit_q_header' do
        headers = [
          {
            header_type:       :xmit_q_header,
            remote_q_name:     'SOME_REMOTE_QUEUE',
            remote_q_mgr_name: 'SOME_REMOTE_Q_MGR',
            msg_type:          WMQ::MQMT_REQUEST,
            msg_id:            'my message Id'},

          {
            header_type:     :ims,
            l_term_override: 'LTERM',
            reply_to_format: WMQ::MQFMT_STRING
          },
        ]
        verify_multiple_headers(headers, WMQ::MQFMT_STRING)
      end
    end
  end

  def verify_header(header, format)
    verify_multiple_headers([header], format)
  end

  def verify_multiple_headers(headers, format)
    data                        = 'Some Test Data'
    message                     = WMQ::Message.new
    message.data                = data
    message.descriptor[:format] = format
    message.headers             = headers
    #assert_equal(true,@queue_manager.put(q_name: @in_queue.name, message: message))
    assert_equal(true, @out_queue.put(message: message))

    message = WMQ::Message.new
    assert_equal true, @in_queue.get(message: message)
    assert_equal data, message.data
    assert_equal headers.size, message.headers.size
    count = 0
    headers.each do |header|
      reply_header = message.headers[count]
      header.each_pair { |key, value| assert_equal(value, reply_header[key]) }
      count = count + 1
    end
  end

end
