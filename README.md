
# Interface Tracker Kernel Module

This kernel module tracks network interfaces and logs their details to `/proc/interface_tracker`.

## Requirements

### Ubuntu
- `build-essential`
- `linux-headers-$(uname -r)`

### Red Hat (RHEL/CentOS)
- `Development Tools`
- `kernel-devel-$(uname -r)`
- `kernel-headers-$(uname -r)`

## Installation

### Ubuntu

1. **Install Prerequisites:**
   ```bash
   sudo apt update
   sudo apt install build-essential linux-headers-$(uname -r)
   ```

2. **Clone the Repository:**
   ```bash
   git clone https://github.com/yourusername/interface_tracker.git
   cd interface_tracker
   ```

3. **Compile the Module:**
   ```bash
   make
   ```

4. **Insert the Module:**
   ```bash
   sudo insmod interface_tracker.ko
   ```

5. **Check if the Module is Loaded:**
   ```bash
   lsmod | grep interface_tracker
   ```

6. **Make the Module Load at Boot:**
   ```bash
   sudo cp interface_tracker.ko /lib/modules/$(uname -r)/kernel/
   echo "interface_tracker" | sudo tee -a /etc/modules
   sudo depmod
   ```

### Red Hat (RHEL/CentOS)

1. **Install Prerequisites:**
   ```bash
   sudo yum groupinstall "Development Tools"
   sudo yum install kernel-devel-$(uname -r) kernel-headers-$(uname -r)
   ```

2. **Clone the Repository:**
   ```bash
   git clone https://github.com/yourusername/interface_tracker.git
   cd interface_tracker
   ```

3. **Compile the Module:**
   ```bash
   make
   ```

4. **Insert the Module:**
   ```bash
   sudo insmod interface_tracker.ko
   ```

5. **Check if the Module is Loaded:**
   ```bash
   lsmod | grep interface_tracker
   ```

6. **Make the Module Load at Boot:**
   ```bash
   sudo cp interface_tracker.ko /lib/modules/$(uname -r)/kernel/
   echo "interface_tracker" | sudo tee -a /etc/modules-load.d/interface_tracker.conf
   sudo depmod
   ```

## Operation

### View Tracked Interfaces

To view the tracked interfaces, read the `/proc/interface_tracker` file:

```bash
cat /proc/interface_tracker

6 lxcbr0 00:16:3e:00:00:00 Physical 0 1500 -1 UP None
1 lo 00:00:00:00:00:00 Logical 0 65536 0 UP None
2 ens18 bc:24:11:dd:11:ae Virtual 0 1500 -1 UP None

```

This will display the details of all network interfaces being tracked by the module.
