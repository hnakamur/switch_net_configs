#!/usr/bin/env ruby

require 'erb'
require 'open3'

def get_ipv4_addr(net_if = "en0")
  Open3.popen3("ifconfig #{net_if}") do |stdin, stdout, stderr, thread|
    line = stdout.readlines.find do |line|
      line =~ /\binet\b/
    end
    line ? line.split(' ')[1] : nil
  end
end

def is_home
  get_ipv4_addr.start_with? '192.168.11.'
end

class ConfigWriter
  def write_config_files
    @is_home = is_home
    [
      ['hosts.erb', '/etc/hosts']
    ].each {|template, dest| render_template_to_file template, dest}
  end

  def render_template_to_file(template_filename, output_path)
    write_file(output_path, render_template(template_filename))
  end

  def render_template(template_filename)
    path = "#{File.dirname(__FILE__)}/templates/#{template_filename}"
    ERB.new(File.read(path)).result(binding)
  end

  def write_file(path, content)
    File.open(path, 'w') do |f|
      f.write(content)
    end
  end
end

ConfigWriter.new.write_config_files
