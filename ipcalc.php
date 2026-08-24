# ipcalc.php
#!/usr/bin/env php
<?php

function ipToInt($ip) {
    $parts = explode('.', $ip);
    return ($parts[0] << 24) | ($parts[1] << 16) | ($parts[2] << 8) | $parts[3];
}

function intToIP($num) {
    return (($num >> 24) & 0xff) . '.' . (($num >> 16) & 0xff) . '.' . (($num >> 8) & 0xff) . '.' . ($num & 0xff);
}

function prefixToMask($prefix) {
    return (~0 << (32 - $prefix)) & 0xffffffff;
}

function maskToPrefix($mask) {
    return substr_count(decbin($mask), '1');
}

function ipBinary($num) {
    return implode('.', array_map(function($b) { return str_pad(decbin($b), 8, '0', STR_PAD_LEFT); },
        [($num >> 24) & 0xff, ($num >> 16) & 0xff, ($num >> 8) & 0xff, $num & 0xff]));
}

class IPCalc {
    public $ipInt, $maskInt, $prefix, $networkInt, $broadcast, $wildcard;
    public $totalHosts, $usableHosts, $firstUsable, $lastUsable;

    public function __construct($input, $mask = null) {
        if (strpos($input, '/') !== false) {
            list($ipPart, $prefixStr) = explode('/', $input);
            $this->ipInt = ipToInt($ipPart);
            $this->prefix = (int)$prefixStr;
            $this->maskInt = prefixToMask($this->prefix);
        } elseif ($mask !== null) {
            $this->ipInt = ipToInt($input);
            if (strpos($mask, '.') !== false) {
                $this->maskInt = ipToInt($mask);
                $this->prefix = maskToPrefix($this->maskInt);
            } else {
                $this->prefix = (int)$mask;
                $this->maskInt = prefixToMask($this->prefix);
            }
        } else {
            throw new Exception("Invalid input");
        }
        $this->networkInt = $this->ipInt & $this->maskInt;
        $this->broadcast = $this->networkInt | (~$this->maskInt & 0xffffffff);
        $this->wildcard = ~$this->maskInt & 0xffffffff;
        $this->totalHosts = 1 << (32 - $this->prefix);
        $this->usableHosts = max(0, $this->totalHosts - 2);
        if ($this->networkInt == $this->broadcast) {
            $this->firstUsable = null;
            $this->lastUsable = null;
        } else {
            $this->firstUsable = $this->networkInt + 1;
            $this->lastUsable = $this->broadcast - 1;
        }
    }

    public function toArray() {
        return [
            'ip' => intToIP($this->ipInt),
            'mask' => intToIP($this->maskInt),
            'prefix' => $this->prefix,
            'network' => intToIP($this->networkInt),
            'broadcast' => intToIP($this->broadcast),
            'wildcard' => intToIP($this->wildcard),
            'first_usable' => $this->firstUsable !== null ? intToIP($this->firstUsable) : null,
            'last_usable' => $this->lastUsable !== null ? intToIP($this->lastUsable) : null,
            'total_hosts' => $this->totalHosts,
            'usable_hosts' => $this->usableHosts,
        ];
    }

    public function subnets($newPrefix) {
        if ($newPrefix <= $this->prefix) throw new Exception("New prefix must be larger than original");
        $num = 1 << ($newPrefix - $this->prefix);
        $step = 1 << (32 - $newPrefix);
        $results = [];
        for ($i = 0; $i < $num; $i++) {
            $net = $this->networkInt + $i * $step;
            $sub = new self(intToIP($net), (string)$newPrefix);
            $results[] = [
                'network' => intToIP($sub->networkInt),
                'prefix' => $sub->prefix,
                'first_usable' => $sub->firstUsable !== null ? intToIP($sub->firstUsable) : 'none',
                'last_usable' => $sub->lastUsable !== null ? intToIP($sub->lastUsable) : 'none',
            ];
        }
        return $results;
    }
}

$opts = getopt("", ["subnet:", "bin", "json", "help"]);
if (isset($opts['help'])) {
    echo "Usage: php ipcalc.php <ip>[/prefix] [mask] [--subnet PREFIX] [--bin] [--json]\n";
    exit(0);
}
$input = $argv[1] ?? null;
$mask = $argv[2] ?? null;
if (!$input) {
    fwrite(STDERR, "Usage: php ipcalc.php <ip>[/prefix] [mask] [options]\n");
    exit(1);
}

try {
    $calc = new IPCalc($input, $mask);
} catch (Exception $e) {
    fwrite(STDERR, "Error: " . $e->getMessage() . "\n");
    exit(1);
}

$result = $calc->toArray();
if (isset($opts['bin'])) {
    $result['binary_ip'] = ipBinary($calc->ipInt);
    $result['binary_mask'] = ipBinary($calc->maskInt);
    $result['binary_network'] = ipBinary($calc->networkInt);
}
$subnets = [];
if (isset($opts['subnet'])) {
    try {
        $subnets = $calc->subnets((int)$opts['subnet']);
    } catch (Exception $e) {
        fwrite(STDERR, "Subnet error: " . $e->getMessage() . "\n");
        exit(1);
    }
}

if (isset($opts['json'])) {
    if (!empty($subnets)) $result['subnets'] = $subnets;
    echo json_encode($result, JSON_PRETTY_PRINT) . "\n";
    exit(0);
}

echo "\nIP Calculator\n";
echo "IP Address:     {$result['ip']}\n";
echo "Subnet Mask:    {$result['mask']} (/{$result['prefix']})\n";
echo "Network:        {$result['network']}\n";
echo "Broadcast:      {$result['broadcast']}\n";
echo "Wildcard Mask:  {$result['wildcard']}\n";
if ($result['first_usable']) {
    echo "First Usable:   {$result['first_usable']}\n";
    echo "Last Usable:    {$result['last_usable']}\n";
} else {
    echo "First Usable:   (none)\n";
    echo "Last Usable:    (none)\n";
}
echo "Total Hosts:    {$result['total_hosts']}\n";
echo "Usable Hosts:   {$result['usable_hosts']}\n";

if (isset($opts['bin'])) {
    echo "\nBinary:\n";
    echo "IP:      {$result['binary_ip']}\n";
    echo "Mask:    {$result['binary_mask']}\n";
    echo "Network: {$result['binary_network']}\n";
}
if (!empty($subnets)) {
    echo "\nSubnets (/" . $opts['subnet'] . "):\n";
    foreach ($subnets as $s) {
        echo "  {$s['network']}/{$s['prefix']}   ({$s['first_usable']} - {$s['last_usable']})\n";
    }
}
?>
