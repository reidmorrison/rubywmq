require 'rake/clean'
require 'rake/testtask'

require_relative 'lib/wmq/version'

task :gem do
  system 'gem build rubywmq.gemspec'
end

task publish: :gem do
  system "git tag -a v#{WMQ::VERSION} -m 'Tagging #{WMQ::VERSION}'"
  system 'git push --tags'
  system "gem push rubywmq-#{WMQ::VERSION}.gem"
  system "rm rubywmq-#{WMQ::VERSION}.gem"
end

desc 'Run Test Suite'
task :test do
  Rake::TestTask.new(:functional) do |t|
    t.test_files = FileList['test/**/*_test.rb']
    t.verbose    = true
  end

  Rake::Task['functional'].invoke
end

task default: :test
