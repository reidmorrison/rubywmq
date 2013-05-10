lib = File.expand_path('../lib/', __FILE__)
$:.unshift lib unless $:.include?(lib)

require 'rubygems'
require 'rake/clean'
require 'rake/testtask'
require 'date'
require 'wmq/version'

desc "Build Ruby Source gem"
task :gem  do |t|
  excludes = [
    'lib/wmq/constants.rb',
    'lib/wmq/constants_admin.rb',
    'ext/wmq_structs.c',
    'ext/wmq_reason.c',
    'ext/Makefile',
    'ext/*.o',
    'ext/wmq.so',
    '*.gem',
    'nbproject'
  ]

  gemspec = Gem::Specification.new do |spec|
    spec.name              = 'ekaranto-rubywmq'
    spec.version           = WMQ::VERSION
    spec.platform          = Gem::Platform::RUBY
    spec.authors           = ['Reid Morrison', 'Edwin Fine']
    spec.email             = ['reidmo@gmail.com']
    spec.homepage          = 'https://github.com/reidmorrison/rubywmq'
    spec.date              = Date.today.to_s
    spec.summary           = "Native Ruby interface into WebSphere MQ"
    spec.description       = "RubyWMQ is a high performance native Ruby interface into WebSphere MQ."
    spec.files             = FileList["./**/*"].exclude(*excludes).map{|f| f.sub(/^\.\//, '')} +
                             ['.document']
    spec.extensions        << 'ext/extconf.rb'
    spec.rubyforge_project = 'rubywmq'
    spec.test_file         = 'tests/test.rb'
    spec.has_rdoc          = true
    spec.required_ruby_version = '>= 1.8.4'
    spec.add_development_dependency 'shoulda'
    spec.requirements << 'WebSphere MQ v5.3, v6 or v7 Client or Server with Development Kit'
  end
  Gem::Builder.new(gemspec).build
end

desc "Build a binary gem including pre-compiled binary files for re-distribution"
task :binary  do |t|
  # Move compiled files into locations for repackaging as a binary gem
  FileUtils.mkdir_p('lib/wmq')
  Dir['ext/lib/*.rb'].each{|file| FileUtils.copy(file, File.join('lib/wmq', File.basename(file)))}
  FileUtils.copy('ext/wmq.so', 'lib/wmq/wmq.so')

  gemspec = Gem::Specification.new do |spec|
    spec.name              = 'ekaranto-rubywmq'
    spec.version           = WMQ::VERSION
    spec.platform          = Gem::Platform::CURRENT
    spec.authors           = ['Reid Morrison', 'Edwin Fine']
    spec.email             = ['reidmo@gmail.com']
    spec.homepage          = 'https://github.com/reidmorrison/rubywmq'
    spec.date              = Date.today.to_s
    spec.summary           = "Native Ruby interface into WebSphere MQ"
    spec.description       = "RubyWMQ is a high performance native Ruby interface into WebSphere MQ."
    spec.files             = Dir['examples/**/*.rb'] +
                             Dir['examples/**/*.cfg'] +
                             Dir['doc/**/*.*'] +
                             Dir['lib/**/*.rb'] +
                             ['lib/wmq/wmq.so', 'tests/test.rb', 'README', 'LICENSE']
    spec.rubyforge_project = 'rubywmq'
    spec.test_file         = 'tests/test.rb'
    spec.has_rdoc          = false
    spec.required_ruby_version = '>= 1.8.4'
    spec.add_development_dependency 'shoulda'
    spec.requirements << 'WebSphere MQ v5.3, v6 or v7 Client or Server with Development Kit'
  end
  Gem::Builder.new(gemspec).build
end

desc "Run Test Suite"
task :test do
  Rake::TestTask.new(:functional) do |t|
    t.test_files = FileList['test/*_test.rb']
    t.verbose    = true
  end

  Rake::Task['functional'].invoke
end
