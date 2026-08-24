🌐 IP Calculator (Subnets) — Multi‑Language Subnet Calculator
8 languages, one powerful IP subnet calculator – analyze any IPv4 address, compute network details, and perform subnet division – right from your terminal.

✨ Features
🔢 CIDR or mask input – accept both 192.168.1.0/24 and 192.168.1.0 255.255.255.0

📐 Network details – network address, broadcast address, wildcard mask

👥 Host counts – total hosts, usable hosts

📋 First / last usable IP – quickly see the range

🔗 Binary representation – show IP and mask in binary (optional)

📊 Subnet division – split a network into smaller subnets (e.g., /24 into /26 subnets)

💾 JSON output – export results in a structured format

🖥️ Cross‑platform – works on Windows, macOS, Linux

🧰 Supported Languages & Files
Language	File	Dependencies
Python	ipcalc.py	none (stdlib)
Go	ipcalc.go	none (stdlib)
JavaScript (Node)	ipcalc.js	commander (optional)
Ruby	ipcalc.rb	optparse (stdlib)
PHP	ipcalc.php	none (extensions)
Java	IPCalc.java	Java 8+
C#	IPCalc.cs	.NET Core 3.1+
C++	ipcalc.cpp	nlohmann/json (optional)
🚀 Quick Start
All implementations follow the same CLI pattern:

bash
# Basic calculation
<command> 192.168.1.0/24

# With separate mask
<command> 192.168.1.10 255.255.255.0

# Show binary details
<command> 10.0.0.0/16 --bin

# Subnet division
<command> 192.168.1.0/24 --subnet 26

# JSON output
<command> 172.16.0.0/20 --json
Arguments:

<ip>[/prefix] or <ip> <mask> – IP and mask (CIDR or dotted decimal)

--subnet <prefix> – create subnets of given prefix length

--bin – show binary representation

--json – output in JSON format

--help – show usage

📸 Example Output
text
IP Calculator
IP Address:     192.168.1.0
Subnet Mask:    255.255.255.0 (/24)
Network:        192.168.1.0
Broadcast:      192.168.1.255
Wildcard Mask:  0.0.0.255
First Usable:   192.168.1.1
Last Usable:    192.168.1.254
Total Hosts:    256
Usable Hosts:   254

Binary:
IP:      11000000.10101000.00000001.00000000
Mask:    11111111.11111111.11111111.00000000
Network: 11000000.10101000.00000001.00000000

Subnets (/26):
  192.168.1.0/26   (192.168.1.1 - 192.168.1.62)
  192.168.1.64/26  (192.168.1.65 - 192.168.1.126)
  192.168.1.128/26 (192.168.1.129 - 192.168.1.190)
  192.168.1.192/26 (192.168.1.193 - 192.168.1.254)
📁 Repository Structure
text
.
├── README.md
├── python/
│   └── ipcalc.py
├── go/
│   └── ipcalc.go
├── javascript/
│   └── ipcalc.js
├── ruby/
│   └── ipcalc.rb
├── php/
│   └── ipcalc.php
├── java/
│   └── IPCalc.java
├── csharp/
│   └── IPCalc.cs
└── cpp/
    └── ipcalc.cpp
