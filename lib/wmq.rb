require 'wmq/version'
require 'wmq/constants'
require 'wmq/constants_admin'
require 'wmq/queue_manager'
require 'wmq/message'

# Load wmq using the auto-load library.
#
# If it fails, then it is most likely since this platform is not supported
# by the auto load facility in Ruby WMQ, so try to load client linked library
# For Example AIX does not support Autoload whereas Windows and Linux are supported
begin
  require 'wmq/wmq'
rescue LoadError
  require 'wmq/wmq_client'
end
