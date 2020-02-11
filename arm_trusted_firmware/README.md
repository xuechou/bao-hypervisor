Implementation of an Arm Trusted Firmware Service that allows Bao to configure and use FIQs.

---
### Zynqmp ZCU10X example:
1. `cd` into the ATF directory;
2. Apply the patch:
```bash
git am ../0001-SPD-baod-implement-SMC-calls-to-control-FIQs.patch
```
3. Compile the ATF with the newly added service:
 ```bash
make CROSS_COMPILE=[..] PLAT=zynqmp SPD=baod
```
4. [Download](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18842316/Zynq+Releases) latest pre-built zynqmp firmware and bootloader 20xx.x-zcu10x-release.tar.xz, and replace the bl31.elf with the one you just compiled here: `build/zynqmp/release/bl31/bl31.elf`
5.  Follow [this guide](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18841722/ZCU102+Image+creation+in+OSL+flow#ZCU102ImagecreationinOSLflow-CreateSDimage) to generate the `BOOT.BIN`.
