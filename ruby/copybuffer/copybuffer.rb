# frozen_string_literal: true

require 'optparse'

options = { platform: 0, device: 0 }
OptionParser.new do |parser|
  parser.banner = "Usage: ruby #{File.basename($PROGRAM_NAME)} [options]"
  parser.on('-p', '--platform INDEX', Integer, 'Index of the platform to use (default 0)')
  parser.on('-d', '--device INDEX', Integer, 'Index of the device to use (default 0)')
end.parse!(into: options)

require 'opencl_ruby_ffi'

buffer_size = 1024 * 1024

p = OpenCL.platforms[options[:platform]]
raise "Invalid platform index #{options[:platform]}" unless p

d = p.devices[options[:device]]
raise "Invalid device index #{options[:device]}" unless d

puts "Running on platform: #{p.name}"
puts "Running on device: #{d.name}"

c = OpenCL.create_context(d)
q = c.create_command_queue(d)

device_src = c.create_buffer(buffer_size * OpenCL::UInt.size, flags: OpenCL::Mem::ALLOC_HOST_PTR)
device_dst = c.create_buffer(buffer_size * OpenCL::UInt.size, flags: OpenCL::Mem::ALLOC_HOST_PTR)

_, host_ptr = q.enqueue_map_buffer(device_src, OpenCL::MapFlags::WRITE_INVALIDATE_REGION, blocking: true)
host_ptr.write_array_of_uint(buffer_size.times.to_a)
q.enqueue_unmap_mem_object(device_src, host_ptr)

q.enqueue_copy_buffer(device_src, device_dst)

_, host_ptr = q.enqueue_map_buffer(device_dst, OpenCL::MapFlags::READ, blocking: true)
result = host_ptr.read_array_of_uint(buffer_size)
buffer_size.times do |i|
  raise "invalid copy: wanted #{i}, got #{result[i]}" unless result[i] == i
end
q.enqueue_unmap_mem_object(device_dst, host_ptr)

puts 'Success.'
