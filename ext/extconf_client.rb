#
# NOTE:  Do NOT use this file unless the platform being used is not supported
#        directly by Ruby WMQ. Ruby WMQ already supports automatic dynamic loading on
#        Windows, Solaris and Linux
#
require 'mkmf'
require_relative '../../generate/generate_reason'
require_relative '../../generate/generate_const'
require_relative '../../generate/generate_structs'

include_path = ''
unless (RUBY_PLATFORM =~ /win/i) || (RUBY_PLATFORM =~ /solaris/i) || (RUBY_PLATFORM =~ /linux/i)
  include_path = '/opt/mqm/inc'
  dir_config('mqm', include_path, '/opt/mqm/lib')
  have_library('mqic')

  # Generate Source Files # Could check if not already present
  GenerateReason.generate(include_path+'/')
  GenerateConst.generate(include_path+'/', '../../lib/wmq')
  GenerateStructs.new(include_path+'/', '../../generate').generate

  have_header('cmqc.h')
  create_makefile('wmq_client')
end
