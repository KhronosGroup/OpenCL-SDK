# frozen_string_literal: true

require 'opencl_ruby_ffi'
require 'yaml'

puts YAML.dump(
  { 'platforms' => OpenCL.platforms.collect do |p|
    { 'name' => p.name,
      'vendor' => p.vendor,
      'version' => p.version,
      'devices' => p.devices.collect do |d|
        { 'name' => d.name,
          'type' => d.type.to_s,
          'vendor' => d.vendor,
          'version' => d.version,
          'profile' => d.profile,
          'driver_version' => d.driver_version }
      end }
  end }
)
