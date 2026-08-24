// ipcalc.go
package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"strconv"
	"strings"
)

func ipToInt(ip string) uint32 {
	parts := strings.Split(ip, ".")
	return (uint32(parseInt(parts[0])) << 24) |
		(uint32(parseInt(parts[1])) << 16) |
		(uint32(parseInt(parts[2])) << 8) |
		uint32(parseInt(parts[3]))
}

func intToIP(n uint32) string {
	return fmt.Sprintf("%d.%d.%d.%d", (n>>24)&0xff, (n>>16)&0xff, (n>>8)&0xff, n&0xff)
}

func parseInt(s string) int {
	v, _ := strconv.Atoi(s)
	return v
}

func prefixToMask(prefix int) uint32 {
	return ^uint32(0) << (32 - prefix)
}

func maskToPrefix(mask uint32) int {
	return bitsOn(mask)
}

func bitsOn(n uint32) int {
	count := 0
	for n != 0 {
		count++
		n &= n - 1
	}
	return count
}

func ipBinary(n uint32) string {
	return fmt.Sprintf("%08b.%08b.%08b.%08b", (n>>24)&0xff, (n>>16)&0xff, (n>>8)&0xff, n&0xff)
}

type IPCalc struct {
	ipInt      uint32
	maskInt    uint32
	prefix     int
	networkInt uint32
	broadcast  uint32
	wildcard   uint32
	first      uint32
	last       uint32
	total      uint32
	usable     uint32
}

func NewIPCalc(input, maskStr string) (*IPCalc, error) {
	var ipInt, maskInt uint32
	var prefix int
	if strings.Contains(input, "/") {
		parts := strings.Split(input, "/")
		ipInt = ipToInt(parts[0])
		prefix, _ = strconv.Atoi(parts[1])
		maskInt = prefixToMask(prefix)
	} else if maskStr != "" {
		ipInt = ipToInt(input)
		if strings.Contains(maskStr, ".") {
			maskInt = ipToInt(maskStr)
			prefix = maskToPrefix(maskInt)
		} else {
			prefix, _ = strconv.Atoi(maskStr)
			maskInt = prefixToMask(prefix)
		}
	} else {
		return nil, fmt.Errorf("invalid input")
	}
	c := &IPCalc{
		ipInt:   ipInt,
		maskInt: maskInt,
		prefix:  prefix,
	}
	c.networkInt = ipInt & maskInt
	c.broadcast = c.networkInt | (^maskInt & 0xffffffff)
	c.wildcard = ^maskInt & 0xffffffff
	if c.networkInt == c.broadcast {
		c.first = 0
		c.last = 0
	} else {
		c.first = c.networkInt + 1
		c.last = c.broadcast - 1
	}
	c.total = 1 << (32 - prefix)
	c.usable = c.total
	if c.total > 2 {
		c.usable = c.total - 2
	}
	return c, nil
}

func (c *IPCalc) toMap() map[string]interface{} {
	m := map[string]interface{}{
		"ip":           intToIP(c.ipInt),
		"mask":         intToIP(c.maskInt),
		"prefix":       c.prefix,
		"network":      intToIP(c.networkInt),
		"broadcast":    intToIP(c.broadcast),
		"wildcard":     intToIP(c.wildcard),
		"total_hosts":  c.total,
		"usable_hosts": c.usable,
	}
	if c.first != 0 {
		m["first_usable"] = intToIP(c.first)
		m["last_usable"] = intToIP(c.last)
	} else {
		m["first_usable"] = nil
		m["last_usable"] = nil
	}
	return m
}

func (c *IPCalc) subnets(newPrefix int) ([]map[string]string, error) {
	if newPrefix <= c.prefix {
		return nil, fmt.Errorf("new prefix must be larger than original")
	}
	num := 1 << (newPrefix - c.prefix)
	step := uint32(1 << (32 - newPrefix))
	var subnets []map[string]string
	for i := 0; i < num; i++ {
		net := c.networkInt + uint32(i)*step
		subCalc, _ := NewIPCalc(intToIP(net), strconv.Itoa(newPrefix))
		first := "none"
		last := "none"
		if subCalc.first != 0 {
			first = intToIP(subCalc.first)
			last = intToIP(subCalc.last)
		}
		subnets = append(subnets, map[string]string{
			"network":      intToIP(subCalc.networkInt),
			"prefix":       strconv.Itoa(subCalc.prefix),
			"first_usable": first,
			"last_usable":  last,
		})
	}
	return subnets, nil
}

func main() {
	var (
		input   = flag.String("input", "", "IP address (CIDR or with mask)")
		mask    = flag.String("mask", "", "Subnet mask if not in CIDR")
		subnet  = flag.Int("subnet", 0, "Create subnets of this prefix length")
		binary  = flag.Bool("bin", false, "Show binary")
		jsonOut = flag.Bool("json", false, "Output JSON")
	)
	flag.Parse()
	if *input == "" && len(flag.Args()) > 0 {
		*input = flag.Args()[0]
	}
	if len(flag.Args()) > 1 && *mask == "" {
		*mask = flag.Args()[1]
	}
	if *input == "" {
		fmt.Println("Usage: ipcalc <ip>[/prefix] [mask] [options]")
		return
	}
	calc, err := NewIPCalc(*input, *mask)
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		return
	}
	res := calc.toMap()
	if *binary {
		res["binary_ip"] = ipBinary(calc.ipInt)
		res["binary_mask"] = ipBinary(calc.maskInt)
		res["binary_network"] = ipBinary(calc.networkInt)
		res["binary_broadcast"] = ipBinary(calc.broadcast)
	}
	var subnets []map[string]string
	if *subnet > 0 {
		subnets, err = calc.subnets(*subnet)
		if err != nil {
			fmt.Printf("Subnet error: %v\n", err)
			return
		}
	}
	if *jsonOut {
		out := res
		if len(subnets) > 0 {
			out["subnets"] = subnets
		}
		b, _ := json.MarshalIndent(out, "", "  ")
		fmt.Println(string(b))
		return
	}
	fmt.Println("\nIP Calculator")
	fmt.Printf("IP Address:     %s\n", res["ip"])
	fmt.Printf("Subnet Mask:    %s (/%d)\n", res["mask"], res["prefix"])
	fmt.Printf("Network:        %s\n", res["network"])
	fmt.Printf("Broadcast:      %s\n", res["broadcast"])
	fmt.Printf("Wildcard Mask:  %s\n", res["wildcard"])
	if res["first_usable"] != nil {
		fmt.Printf("First Usable:   %s\n", res["first_usable"])
		fmt.Printf("Last Usable:    %s\n", res["last_usable"])
	} else {
		fmt.Println("First Usable:   (none)")
		fmt.Println("Last Usable:    (none)")
	}
	fmt.Printf("Total Hosts:    %d\n", res["total_hosts"])
	fmt.Printf("Usable Hosts:   %d\n", res["usable_hosts"])
	if *binary {
		fmt.Println("\nBinary:")
		fmt.Printf("IP:      %s\n", res["binary_ip"])
		fmt.Printf("Mask:    %s\n", res["binary_mask"])
		fmt.Printf("Network: %s\n", res["binary_network"])
	}
	if len(subnets) > 0 {
		fmt.Printf("\nSubnets (/%d):\n", *subnet)
		for _, s := range subnets {
			fmt.Printf("  %s/%s   (%s - %s)\n", s["network"], s["prefix"], s["first_usable"], s["last_usable"])
		}
	}
}
