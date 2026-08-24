// IPCalc.cs
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

class IPCalc
{
    static uint IpToInt(string ip)
    {
        var parts = ip.Split('.');
        return (uint.Parse(parts[0]) << 24) |
               (uint.Parse(parts[1]) << 16) |
               (uint.Parse(parts[2]) << 8) |
               uint.Parse(parts[3]);
    }

    static string IntToIp(uint num)
    {
        return $"{((num >> 24) & 0xff)}.{((num >> 16) & 0xff)}.{((num >> 8) & 0xff)}.{num & 0xff}";
    }

    static uint PrefixToMask(int prefix) => (~0u << (32 - prefix)) & 0xffffffff;

    static int MaskToPrefix(uint mask) => System.Numerics.BitOperations.PopCount(mask);

    static string IpBinary(uint num)
    {
        return $"{Convert.ToString((num >> 24) & 0xff, 2).PadLeft(8, '0')}." +
               $"{Convert.ToString((num >> 16) & 0xff, 2).PadLeft(8, '0')}." +
               $"{Convert.ToString((num >> 8) & 0xff, 2).PadLeft(8, '0')}." +
               $"{Convert.ToString(num & 0xff, 2).PadLeft(8, '0')}";
    }

    class Result
    {
        public string ip { get; set; }
        public string mask { get; set; }
        public int prefix { get; set; }
        public string network { get; set; }
        public string broadcast { get; set; }
        public string wildcard { get; set; }
        public string first_usable { get; set; }
        public string last_usable { get; set; }
        public int total_hosts { get; set; }
        public int usable_hosts { get; set; }
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public string binary_ip { get; set; }
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public string binary_mask { get; set; }
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public string binary_network { get; set; }
        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
        public List<Subnet> subnets { get; set; }
    }

    class Subnet
    {
        public string network { get; set; }
        public string prefix { get; set; }
        public string first_usable { get; set; }
        public string last_usable { get; set; }
    }

    static Result Calculate(string input, string mask)
    {
        uint ipInt, maskInt;
        int prefix;
        if (input.Contains("/"))
        {
            var parts = input.Split('/');
            ipInt = IpToInt(parts[0]);
            prefix = int.Parse(parts[1]);
            maskInt = PrefixToMask(prefix);
        }
        else if (mask != null)
        {
            ipInt = IpToInt(input);
            if (mask.Contains("."))
            {
                maskInt = IpToInt(mask);
                prefix = MaskToPrefix(maskInt);
            }
            else
            {
                prefix = int.Parse(mask);
                maskInt = PrefixToMask(prefix);
            }
        }
        else throw new Exception("Invalid input");
        uint networkInt = ipInt & maskInt;
        uint broadcast = networkInt | (~maskInt & 0xffffffff);
        uint wildcard = ~maskInt & 0xffffffff;
        int total = 1 << (32 - prefix);
        int usable = Math.Max(0, total - 2);
        string first = networkInt == broadcast ? null : IntToIp(networkInt + 1);
        string last = networkInt == broadcast ? null : IntToIp(broadcast - 1);
        return new Result
        {
            ip = IntToIp(ipInt),
            mask = IntToIp(maskInt),
            prefix = prefix,
            network = IntToIp(networkInt),
            broadcast = IntToIp(broadcast),
            wildcard = IntToIp(wildcard),
            first_usable = first,
            last_usable = last,
            total_hosts = total,
            usable_hosts = usable
        };
    }

    static List<Subnet> GetSubnets(string input, string mask, int newPrefix)
    {
        var baseRes = Calculate(input, mask);
        if (newPrefix <= baseRes.prefix) throw new Exception("New prefix must be larger than original");
        uint netInt = IpToInt(baseRes.network);
        int num = 1 << (newPrefix - baseRes.prefix);
        uint step = 1u << (32 - newPrefix);
        var list = new List<Subnet>();
        for (int i = 0; i < num; i++)
        {
            uint n = netInt + (uint)i * step;
            var sub = Calculate(IntToIp(n), newPrefix.ToString());
            list.Add(new Subnet
            {
                network = sub.network,
                prefix = sub.prefix.ToString(),
                first_usable = sub.first_usable ?? "none",
                last_usable = sub.last_usable ?? "none"
            });
        }
        return list;
    }

    static void Main(string[] args)
    {
        if (args.Length < 1)
        {
            Console.WriteLine("Usage: IPCalc <ip>[/prefix] [mask] [--subnet PREFIX] [--bin] [--json]");
            return;
        }
        string input = args[0];
        string mask = null;
        bool bin = false, json = false;
        int subnetPrefix = -1;
        for (int i = 1; i < args.Length; i++)
        {
            if (args[i] == "--subnet" && i + 1 < args.Length)
                subnetPrefix = int.Parse(args[++i]);
            else if (args[i] == "--bin") bin = true;
            else if (args[i] == "--json") json = true;
            else if (!args[i].StartsWith("--") && mask == null)
                mask = args[i];
        }
        var res = Calculate(input, mask);
        if (bin)
        {
            res.binary_ip = IpBinary(IpToInt(res.ip));
            res.binary_mask = IpBinary(IpToInt(res.mask));
            res.binary_network = IpBinary(IpToInt(res.network));
        }
        if (subnetPrefix != -1)
        {
            res.subnets = GetSubnets(input, mask, subnetPrefix);
        }
        if (json)
        {
            var opt = new JsonSerializerOptions { WriteIndented = true };
            Console.WriteLine(JsonSerializer.Serialize(res, opt));
            return;
        }
        Console.WriteLine("\nIP Calculator");
        Console.WriteLine($"IP Address:     {res.ip}");
        Console.WriteLine($"Subnet Mask:    {res.mask} (/{res.prefix})");
        Console.WriteLine($"Network:        {res.network}");
        Console.WriteLine($"Broadcast:      {res.broadcast}");
        Console.WriteLine($"Wildcard Mask:  {res.wildcard}");
        if (res.first_usable != null)
        {
            Console.WriteLine($"First Usable:   {res.first_usable}");
            Console.WriteLine($"Last Usable:    {res.last_usable}");
        }
        else
        {
            Console.WriteLine("First Usable:   (none)");
            Console.WriteLine("Last Usable:    (none)");
        }
        Console.WriteLine($"Total Hosts:    {res.total_hosts}");
        Console.WriteLine($"Usable Hosts:   {res.usable_hosts}");
        if (bin)
        {
            Console.WriteLine("\nBinary:");
            Console.WriteLine($"IP:      {res.binary_ip}");
            Console.WriteLine($"Mask:    {res.binary_mask}");
            Console.WriteLine($"Network: {res.binary_network}");
        }
        if (res.subnets != null && res.subnets.Count > 0)
        {
            Console.WriteLine($"\nSubnets (/{subnetPrefix}):");
            foreach (var s in res.subnets)
                Console.WriteLine($"  {s.network}/{s.prefix}   ({s.first_usable} - {s.last_usable})");
        }
    }
}
