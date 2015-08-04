# QueueManager ruby methods
module WMQ
  class QueueManager
    # Execute any MQSC command against the queue manager
    #
    # Example
    #   require 'wmq/wmq'
    #   require 'wmq/wmq_const_admin'
    #   WMQ::QueueManager.connect(q_mgr_name: 'REID', connection_name: 'localhost(1414)') do |qmgr|
    #     qmgr.mqsc('dis ql(*)').each {|item| p item }
    #   end
    def mqsc(mqsc_text)
      execute(command: :escape, escape_type: WMQ::MQET_MQSC, escape_text: mqsc_text).collect { |item| item[:escape_text] }
    end

    # Put a reply message back to the sender
    #
    #   The :message is sent to the queue and queue manager specified in the
    #   :reply_to_q and :reply_to_q_mgr propoerties of the :request_message.
    #
    #   The following rules are followed before sending the reply:
    #   - Only send replies to Request messages. No reply for Datagrams
    #   - Set the message type to Reply when replying to a request message
    #   - Reply with:
    #     - Remaining Expiry (Ideally deduct any processing time since get)
    #     - Same priority as received message
    #     - Same persistence as received message
    #   - Adhere to the Report options supplied for message and correlation id's
    #       in reply message
    #   - All headers must be returned on reply messages
    #     - This allows the calling application to store state information
    #       in these headers
    #     - Unless of course if the relevant header is input only and used
    #       for completing the request
    #       - In this case any remaining headers should be returned
    #         to the caller
    #
    # Parameters:
    #   * :request_message The message originally received
    #   * All the other parameters are the same as QueueManager#put
    #
    def put_to_reply_q(parms)
      # Send replies only if message type is request
      if parms[:request_message].descriptor[:msg_type] == WMQ::MQMT_REQUEST
        request = parms.delete(:request_message)

        reply                         = parms[:message] ||= Message.new(data: parms[:data])
        reply.descriptor[:msg_type]   = WMQ::MQMT_REPLY
        reply.descriptor[:expiry]     = request.descriptor[:expiry]
        reply.descriptor[:priority]   = request.descriptor[:priority]
        reply.descriptor[:persistence]= request.descriptor[:persistence]
        reply.descriptor[:format]     = request.descriptor[:format]

        # Set Correlation Id based on report options supplied
        reply.descriptor[:correl_id]  =
          if request.descriptor[:report] & WMQ::MQRO_PASS_CORREL_ID != 0
            request.descriptor[:correl_id]
          else
            request.descriptor[:msg_id]
          end

        # Set Message Id based on report options supplied
        if request.descriptor[:report] & WMQ::MQRO_PASS_MSG_ID != 0
          reply.descriptor[:msg_id] = request.descriptor[:msg_id]
        end

        parms[:q_name]    = request.descriptor[:reply_to_q]
        parms[:q_mgr_name]= request.descriptor[:reply_to_q_mgr]
        return put(parms)
      else
        return false
      end
    end

    # Put a message to the Dead Letter Queue
    #
    #   If an error occurs when processing a datagram message
    #   it is necessary to move the message to the dead letter queue.
    #   I.e. An error message cannot be sent back to the sender because
    #        the original message was not a request message.
    #          I.e. msg_type != WMQ::MQMT_REQUEST
    #
    #   All existing message data, message descriptor and message headers
    #   are retained.
    #
    def put_to_dead_letter_q(parms)
      message = parms[:message] ||= Message.new(data: parms[:data])
      dlh     = {
        header_type:     :dead_letter_header,
        reason:          parms.delete(:reason),
        dest_q_name:     parms.delete(:q_name),
        dest_q_mgr_name: name
      }

      message.headers.unshift(dlh)
      parms[:q_name] = 'SYSTEM.DEAD.LETTER.QUEUE' #TODO Should be obtained from QMGR config

      put(parms)
    end

    # Expose Commands directly as Queue Manager methods
    def method_missing(name, *args)
      if args.size == 1
        execute({ command: name }.merge(args[0]))
      elsif args.size == 0
        execute(command: name)
      else
        raise("Invalid arguments supplied to QueueManager#:#{name}, args:#{args}")
      end
    end

  end
end
