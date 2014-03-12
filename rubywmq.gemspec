$:.push File.expand_path("../lib", __FILE__)

# Maintain your gem's version:
require 'rubywmq/version'

# Describe your gem and declare its dependencies:
Gem::Specification.new do |spec|
  # Exclude locally compiled files since they are platform specific
  excludes = [
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

  spec.name        = 'rubywmq'
  spec.version     = WMQ::VERSION
  spec.platform    = Gem::Platform::RUBY
  spec.authors     = ['Reid Morrison', 'Edwin Fine']
  spec.email       = ['reidmo@gmail.com']
  spec.homepage    = 'https://github.com/reidmorrison/rubywmq'
  spec.summary     = "Native Ruby interface into WebSphere MQ"
  spec.description = "RubyWMQ is a high performance native Ruby interface into WebSphere MQ."
  spec.files       = Dir["lib/**/*", "LICENSE.txt", "Rakefile", "README.md"]
  spec.files       = FileList["./**/*"].exclude(*excludes).map{|f| f.sub(/^\.\//, '')} + ['.document']
  spec.extensions  << 'ext/extconf.rb'
  spec.test_files  = Dir["test/**/*"]
  spec.license     = "Apache License V2.0"
  spec.has_rdoc    = true
  spec.required_ruby_version = '>= 1.8.4'
  spec.add_development_dependency 'shoulda'
  spec.requirements << 'WebSphere MQ v5.3, v6 or v7 Client or Server with Development Kit'
end

