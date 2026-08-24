# ipcalc.rb
#!/usr/bin/env ruby
require 'optparse'
require 'json'

def ip_to_int(ip)
  parts = ip.split('.')
  (parts[0].to_i << 24) | (parts[1].to_i << 16) | (parts[2].to_i << 8) | parts[3].to_i
end

def int_to_ip(num)
  [(num >> 24) & 0xff, (num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff].join('.')
end

def prefix_to_mask(prefix)
  (~0 << (32 - prefix)) & 0xffffffff
end

def mask_to_prefix(mask)
  mask.to_s(2).count('1')
end

def ip_binary(num)
  [(num >> 24) & 0xff, (num >> 16) & 0xff, (num >> 8) & 0xff, num & 0xff]
    .map { |b| b.to_s(2).rjust(8, '0') }.join('.')
end

class IPCalc
  attr_reader :ip_int, :mask_int, :prefix, :network_int, :broadcast, :wildcard,
              :total_hosts, :usable_hosts, :first_usable, :last_usable

  def initialize(input, mask = nil)
    if input.include?('/')
      ip_part, prefix_str = input.split('/')
      @ip_int = ip_to_int(ip_part)
      @prefix = prefix_str.to_i
      @mask_int = prefix_to_mask(@prefix)
    elsif mask
      @ip_int = ip_to_int(input)
      if mask.include?('.')
        @mask_int = ip_to_int(mask)
        @prefix = mask_to_prefix(@mask_int)
      else
        @prefix = mask.to_i
        @mask_int = prefix_to_mask(@prefix)
      end
    else
      raise ArgumentError, "Invalid input"
    end
    @network_int = @ip_int & @mask_int
    @broadcast = @network_int | (~@mask_int & 0xffffffff)
    @wildcard = ~@mask_int & 0xffffffff
    @total_hosts = 1 << (32 - @prefix)
    @usable_hosts = [0, @total_hosts - 2].max
    if @network_int == @broadcast
      @first_usable = nil
      @last_usable = nil
    else
      @first_usable = @network_int + 1
      @last_usable = @broadcast - 1
    end
  end

  def to_hash
    {
      ip: int_to_ip(@ip_int),
      mask: int_to_ip(@mask_int),
      prefix: @prefix,
      network: int_to_ip(@network_int),
      broadcast: int_to_ip(@broadcast),
      wildcard: int_to_ip(@wildcard),
      first_usable: @first_usable ? int_to_ip(@first_usable) : nil,
      last_usable: @last_usable ? int_to_ip(@last_usable) : nil,
      total_hosts: @total_hosts,
      usable_hosts: @usable_hosts
    }
  end

  def subnets(new_prefix)
    raise "New prefix must be larger than original" if new_prefix <= @prefix
    num = 1 << (new_prefix - @prefix)
    step = 1 << (32 - new_prefix)
    results = []
    num.times do |i|
      net = @network_int + i * step
      sub = IPCalc.new(int_to_ip(net), new_prefix.to_s)
      results << {
        network: int_to_ip(sub.network_int),
        prefix: sub.prefix,
        first_usable: sub.first_usable ? int_to_ip(sub.first_usable) : 'none',
        last_usable: sub.last_usable ? int_to_ip(sub.last_usable) : 'none'
      }
    end
    results
  end
end

options = {}
OptionParser.new do |opts|
  opts.banner = "Usage: ipcalc.rb <ip>[/prefix] [mask] [options]"
  opts.on("--subnet PREFIX", Integer, "Create subnets of this prefix length") { |v| options[:subnet] = v }
  opts.on("--bin", "Show binary representation") { options[:bin] = true }
  opts.on("--json", "Output JSON") { options[:json] = true }
end.parse!

input = ARGV[0]
mask = ARGV[1]
unless input
  puts "Usage: ipcalc.rb <ip>[/prefix] [mask] [options]"
  exit 1
end

begin
  calc = IPCalc.new(input, mask)
rescue => e
  warn "Error: #{e.message}"
  exit 1
end

result = calc.to_hash
if options[:bin]
  result[:binary_ip] = ip_binary(calc.ip_int)
  result[:binary_mask] = ip_binary(calc.mask_int)
  result[:binary_network] = ip_binary(calc.network_int)
end

subnets = []
if options[:subnet]
  begin
    subnets = calc.subnets(options[:subnet])
  rescue => e
    warn "Subnet error: #{e.message}"
    exit 1
  end
end

if options[:json]
  result[:subnets] = subnets if subnets.any?
  puts JSON.pretty_generate(result)
  exit
end

puts "\nIP Calculator"
puts "IP Address:     #{result[:ip]}"
puts "Subnet Mask:    #{result[:mask]} (/#{result[:prefix]})"
puts "Network:        #{result[:network]}"
puts "Broadcast:      #{result[:broadcast]}"
puts "Wildcard Mask:  #{result[:wildcard]}"
if result[:first_usable]
  puts "First Usable:   #{result[:first_usable]}"
  puts "Last Usable:    #{result[:last_usable]}"
else
  puts "First Usable:   (none)"
  puts "Last Usable:    (none)"
end
puts "Total Hosts:    #{result[:total_hosts]}"
puts "Usable Hosts:   #{result[:usable_hosts]}"

if options[:bin]
  puts "\nBinary:"
  puts "IP:      #{result[:binary_ip]}"
  puts "Mask:    #{result[:binary_mask]}"
  puts "Network: #{result[:binary_network]}"
end

if subnets.any?
  puts "\nSubnets (/#{options[:subnet]}):"
  subnets.each do |s|
    puts "  #{s[:network]}/#{s[:prefix]}   (#{s[:first_usable]} - #{s[:last_usable]})"
  end
end
