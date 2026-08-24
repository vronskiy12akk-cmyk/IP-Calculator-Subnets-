// IPCalc.java
import java.io.*;
import java.util.*;
import com.google.gson.*;

public class IPCalc {
    public static long ipToInt(String ip) {
        String[] parts = ip.split("\\.");
        return (Long.parseLong(parts[0]) << 24) |
               (Long.parseLong(parts[1]) << 16) |
               (Long.parseLong(parts[2]) << 8) |
               Long.parseLong(parts[3]);
    }

    public static String intToIP(long num) {
        return ((num >> 24) & 0xff) + "." +
               ((num >> 16) & 0xff) + "." +
               ((num >> 8) & 0xff) + "." +
               (num & 0xff);
    }

    public static long prefixToMask(int prefix) {
        return (~0L << (32 - prefix)) & 0xffffffffL;
    }

    public static int maskToPrefix(long mask) {
        return Long.bitCount(mask);
    }

    public static String ipBinary(long num) {
        return String.format("%08d.%08d.%08d.%08d",
            Long.parseLong(Long.toBinaryString((num >> 24) & 0xff)),
            Long.parseLong(Long.toBinaryString((num >> 16) & 0xff)),
            Long.parseLong(Long.toBinaryString((num >> 8) & 0xff)),
            Long.parseLong(Long.toBinaryString(num & 0xff)));
    }

    static class CalcResult {
        String ip, mask, network, broadcast, wildcard, firstUsable, lastUsable;
        int prefix, totalHosts, usableHosts;
    }

    public static CalcResult calculate(String input, String maskStr) throws Exception {
        long ipInt, maskInt;
        int prefix;
        if (input.contains("/")) {
            String[] parts = input.split("/");
            ipInt = ipToInt(parts[0]);
            prefix = Integer.parseInt(parts[1]);
            maskInt = prefixToMask(prefix);
        } else if (maskStr != null) {
            ipInt = ipToInt(input);
            if (maskStr.contains(".")) {
                maskInt = ipToInt(maskStr);
                prefix = maskToPrefix(maskInt);
            } else {
                prefix = Integer.parseInt(maskStr);
                maskInt = prefixToMask(prefix);
            }
        } else {
            throw new Exception("Invalid input");
        }
        long networkInt = ipInt & maskInt;
        long broadcast = networkInt | (~maskInt & 0xffffffffL);
        long wildcard = ~maskInt & 0xffffffffL;
        int total = 1 << (32 - prefix);
        int usable = Math.max(0, total - 2);
        long firstUsable = networkInt == broadcast ? -1 : networkInt + 1;
        long lastUsable = networkInt == broadcast ? -1 : broadcast - 1;

        CalcResult res = new CalcResult();
        res.ip = intToIP(ipInt);
        res.mask = intToIP(maskInt);
        res.prefix = prefix;
        res.network = intToIP(networkInt);
        res.broadcast = intToIP(broadcast);
        res.wildcard = intToIP(wildcard);
        res.firstUsable = firstUsable != -1 ? intToIP(firstUsable) : null;
        res.lastUsable = lastUsable != -1 ? intToIP(lastUsable) : null;
        res.totalHosts = total;
        res.usableHosts = usable;
        return res;
    }

    public static List<Map<String, String>> subnets(String input, String maskStr, int newPrefix) throws Exception {
        CalcResult base = calculate(input, maskStr);
        if (newPrefix <= base.prefix) throw new Exception("New prefix must be larger than original");
        long netInt = ipToInt(base.network);
        int num = 1 << (newPrefix - base.prefix);
        long step = 1L << (32 - newPrefix);
        List<Map<String, String>> result = new ArrayList<>();
        for (int i = 0; i < num; i++) {
            long n = netInt + i * step;
            CalcResult sub = calculate(intToIP(n), String.valueOf(newPrefix));
            Map<String, String> map = new HashMap<>();
            map.put("network", sub.network);
            map.put("prefix", String.valueOf(sub.prefix));
            map.put("first_usable", sub.firstUsable != null ? sub.firstUsable : "none");
            map.put("last_usable", sub.lastUsable != null ? sub.lastUsable : "none");
            result.add(map);
        }
        return result;
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.out.println("Usage: IPCalc <ip>[/prefix] [mask] [--subnet PREFIX] [--bin] [--json]");
            return;
        }
        String input = args[0];
        String mask = args.length > 1 && !args[1].startsWith("--") ? args[1] : null;
        boolean bin = false, json = false;
        int subnetPrefix = -1;
        for (int i = 1; i < args.length; i++) {
            if (args[i].equals("--subnet") && i+1 < args.length) {
                subnetPrefix = Integer.parseInt(args[++i]);
            } else if (args[i].equals("--bin")) {
                bin = true;
            } else if (args[i].equals("--json")) {
                json = true;
            }
        }
        CalcResult res = calculate(input, mask);
        Map<String, Object> resultMap = new LinkedHashMap<>();
        resultMap.put("ip", res.ip);
        resultMap.put("mask", res.mask);
        resultMap.put("prefix", res.prefix);
        resultMap.put("network", res.network);
        resultMap.put("broadcast", res.broadcast);
        resultMap.put("wildcard", res.wildcard);
        resultMap.put("first_usable", res.firstUsable);
        resultMap.put("last_usable", res.lastUsable);
        resultMap.put("total_hosts", res.totalHosts);
        resultMap.put("usable_hosts", res.usableHosts);
        if (bin) {
            resultMap.put("binary_ip", ipBinary(ipToInt(res.ip)));
            resultMap.put("binary_mask", ipBinary(ipToInt(res.mask)));
            resultMap.put("binary_network", ipBinary(ipToInt(res.network)));
        }
        List<Map<String, String>> subnets = null;
        if (subnetPrefix != -1) {
            subnets = subnets(input, mask, subnetPrefix);
            resultMap.put("subnets", subnets);
        }

        if (json) {
            Gson gson = new GsonBuilder().setPrettyPrinting().create();
            System.out.println(gson.toJson(resultMap));
            return;
        }
        System.out.println("\nIP Calculator");
        System.out.printf("IP Address:     %s%n", res.ip);
        System.out.printf("Subnet Mask:    %s (/%d)%n", res.mask, res.prefix);
        System.out.printf("Network:        %s%n", res.network);
        System.out.printf("Broadcast:      %s%n", res.broadcast);
        System.out.printf("Wildcard Mask:  %s%n", res.wildcard);
        if (res.firstUsable != null) {
            System.out.printf("First Usable:   %s%n", res.firstUsable);
            System.out.printf("Last Usable:    %s%n", res.lastUsable);
        } else {
            System.out.println("First Usable:   (none)");
            System.out.println("Last Usable:    (none)");
        }
        System.out.printf("Total Hosts:    %d%n", res.totalHosts);
        System.out.printf("Usable Hosts:   %d%n", res.usableHosts);
        if (bin) {
            System.out.println("\nBinary:");
            System.out.printf("IP:      %s%n", resultMap.get("binary_ip"));
            System.out.printf("Mask:    %s%n", resultMap.get("binary_mask"));
            System.out.printf("Network: %s%n", resultMap.get("binary_network"));
        }
        if (subnets != null && !subnets.isEmpty()) {
            System.out.printf("\nSubnets (/%d):%n", subnetPrefix);
            for (Map<String, String> s : subnets) {
                System.out.printf("  %s/%s   (%s - %s)%n", s.get("network"), s.get("prefix"), s.get("first_usable"), s.get("last_usable"));
            }
        }
    }
}
