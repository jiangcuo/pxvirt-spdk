# SPDK for PXVIRT

An SPDK component project optimized for Proxmox VE and PXVIRT environments. By adopting AIO bdev type, it achieves over 15% performance improvement compared to traditional virtio-scsi and 10% latency reduction.

## Project Overview

This project aims to integrate SPDK components into Proxmox VE or PXVIRT environments. Currently, AIO bdev type is adopted as the main implementation approach, which is a well-considered technical choice:

- **AIO Solution Advantages**: Provides the optimal balance of performance and manageability
- **User-space NVMe**: Although NVMe bdev via vfio-pci offers the best performance and advanced features like RAID5F, storage management complexity is higher, and it's planned for future versions
- **Flexibility**: Users can independently use user-space NVMe solutions through parameters

## Build and Installation

### Install Dependencies

```bash
apt update
apt install devscripts build-essential git -y
```

### Compile and Build

```bash
git submodule update --init --recursive
mk-build-deps --install --remove
make deb
```

## Configuration and Usage

### Configuration File

Edit the configuration file `/etc/spdk/vhost.conf`:

```bash
cores=0x1       # CPU mask for core binding
hugemem=2048    # Hugepage memory size in MB
vhost=/var/tmp  # vhost socket path (do not modify)
```

**Note**: Service restart is required after configuration changes

```bash
systemctl restart pxvirt-spdk
```

⚠️ **Important Warning**: Restarting the service will stop VM SPDK threads, virtual machines using SPDK may crash!

### CPU Mask Configuration

Use `cpumask_tool` to convert between core lists and hexadecimal masks:

```bash
root@pvedevel:~/spdk# cpumask_tool -c 4-9
Hex mask:   0x3f0
Core list:  4-9
Core count: 6
```

### Hugepage Configuration

Create GRUB configuration file `/etc/default/grub.d/spdk.cfg`:

```bash
cat /etc/default/grub.d/spdk.cfg
GRUB_CMDLINE_LINUX="hugepagesz=2M hugepages=2048 intel_iommu=on iommu=pt amd_iommu=on"
```

After modifying the `hugepages=2048` parameter value, run `update-grub` to apply the new hugepage memory configuration.

**ZFS Users Note**: If using ZFS, please modify the `/etc/kernel/cmdline` file and run `proxmox-boot-tool refresh`

## PXVIRT Integration

Enable SPDK support for virtual machines:

```bash
qm set $vmid --spdk local:60
```

## Roadmap

- [ ] RBD bdev support
- [ ] Ceph SPDK OSD integration  
- [ ] NVMe user-space management

