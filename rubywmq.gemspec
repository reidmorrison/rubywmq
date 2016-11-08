$:.push File.expand_path('../lib', __FILE__)

# Maintain your gem's version:
require 'wmq/version'
require 'rake/file_list'

# Describe your gem and declare its dependencies:
Gem::Specification.new do |s|
  # Exclude locally compiled files since they are platform specific
  excludes      = [
    /lib.wmq.constants\.rb/,
    /lib.wmq.constants_admin\.rb/,
    /ext.wmq_structs\.c/,
    /ext.wmq_reason\.c/,
    /ext.Makefile/,
    /ext.*\.o/,
    /ext.wmq\.so/,
    /\.gem$/,
    /\.log$/,
    /nbproject/
  ]
  s.name        = 'rubywmq'
  s.version     = WMQ::VERSION
  s.platform    = Gem::Platform::RUBY
  s.authors     = ['Reid Morrison']
  s.email       = ['reidmo@gmail.com']
  s.homepage    = 'https://github.com/reidmorrison/rubywmq'
  s.summary     = 'Native Ruby interface into WebSphere MQ'
  s.description = 'RubyWMQ is a high performance native Ruby interface into WebSphere MQ.'
  s.files       = Rake::FileList['./**/*'].exclude(*excludes).map { |f| f.sub(/^\.\//, '') } + ['.document']
  s.test_files  = Dir['test/**/*']
  s.license     = 'Apache-2.0'
  s.has_rdoc    = true
  s.extensions << 'ext/extconf.rb'
  s.requirements << 'WebSphere MQ v5.3, v6 or v7 Client or Server with Development Kit'
  s.required_ruby_version = '>= 1.9'
end

