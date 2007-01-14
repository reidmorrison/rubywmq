################################################################################
#  Copyright 2006 J. Reid Morrison. Dimension Solutions, Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
################################################################################

#
# NOTE:  Do NOT use this file unless the platform being requested is not supported
#        directly by Ruby WMQ. Ruby WMQ already supports automatic dynamic loading on
#        Windows, Solaris and Linux
#
require 'mkmf'
require '../../generate/generate_reason'
require '../../generate/generate_const'
require '../../generate/generate_structs'

include_path = ''
unless (RUBY_PLATFORM =~ /win/i) || (RUBY_PLATFORM =~ /solaris/i) || (RUBY_PLATFORM =~ /linux/i)
  include_path = '/opt/mqm/inc'
  dir_config('mqm', include_path, '/opt/mqm/lib')
  have_library('mqic')
  
  # Generate Source Files # Could check if not already present
  GenerateReason.generate(include_path+'/')
  GenerateConst.generate(include_path+'/')
  GenerateStructs.new(include_path+'/', '../../generate').generate
  
  have_header('cmqc.h')
  create_makefile('wmq_client')
end
