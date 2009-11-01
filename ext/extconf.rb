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

require 'mkmf'
require 'generate/generate_reason'
require 'generate/generate_const'
require 'generate/generate_structs'

include_path = ''
if RUBY_PLATFORM =~ /mswin32/
  include_path = 'C:\Program Files\IBM\WebSphere MQ\Tools\c\include'
  dir_config('mqm', include_path, '.')
else
  include_path = '/opt/mqm/inc'
  #dir_config('mqm', include_path, '/opt/mqm/lib')
end

have_header('cmqc.h')

# Check for WebSphere MQ Server library
unless (RUBY_PLATFORM =~ /win/i) || (RUBY_PLATFORM =~ /solaris/i) || (RUBY_PLATFORM =~ /linux/i)
  have_library('mqm')
end

# Generate Source Files
GenerateReason.generate(include_path+'/')
GenerateConst.generate(include_path+'/', 'lib')
GenerateStructs.new(include_path+'/', 'generate').generate

# Generate Makefile
create_makefile('wmq/wmq')
