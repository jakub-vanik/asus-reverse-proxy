# asus-reverse-proxy
*asus-reverse-proxy* is a simple reverse proxy meant to be run inside ASUS router. It enables to having more domains hosted on different machines behind single IP address using just default ports. It is capable of routing HTTP traffic using host header and HTTPS or other SSL traffic using SNI extension. This proxy works nicely with multiple machines with Let's Encrypt certificates because it satisfies the requirement of using only default ports 80 and 443 for all servers. The main drawback is absence of X-Forwarded-For header support. It was tested on **RT-N14U** and **RT-AC1200G+**.

## Build
Cross compiler for each router model is downloadable from ASUS website. Find product page of our router, select **Support**, **Driver & Tools** and select OS **Other**. Now you should see **Source Code** to download. There is cross compiler somewhere in the archive. Each model has different archive structure. **RT-AC1200G+** has toolchain in **asuswrt/release/src-rt-9.x/src/toolchains/hndtools-arm-linux-2.6.36-uclibc-4.5.3**. **RT-N14U** has toolchain in **asuswrt/tools/brcm/K26/hndtools-mipsel-linux-uclibc-4.2.3**. Set **TOOLCHAIN_BASE** to extracted toolchain and run make. In case of compilation for older MIPS based models as **RT-N14U** compiler name has to be changed to **mipsel-linux-uclibc-g++** and **-static-libstdc++** flag has to be removed in Makefile.

## Usage
Create **hosts** file in same format as **/etc/hosts** and load this file and compiled **proxy** into same directory on your router using SCP or put it on a flash drive and connect it to the router. Connect using telnet or SSH and launch **proxy**.

## Autostarting
Autostart is possible only from flash drive because filesystem of router is stored in ramdisk and is not persistent. Autostart can be achieved by placing **autostart.sh**, **proxy** and **hosts** to root of flash drive. Also **script_usbmount** property has to be set to **cd $1 && sh autorun.sh** using nvram. This can be done by running **nvram set 'script_usbmount=cd $1 && sh autorun.sh' && nvram commit**. This is tested on **RT-AC1200G+** only.
