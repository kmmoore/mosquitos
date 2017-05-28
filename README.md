MosquitOS is a tiny x86_64 UEFI-based operating system built from scratch.

**Goals:**

-   64-bit kernel
-   UEFI bootloader
-   Pre-emptive multitasking
-   Multiprocessor support
-   PCI support
-   USB support
-   Basic filesystem
-   Virtual memory
-   Bootable on a real computer

**Compilation:**

*Note:* I do all development on Ubuntu 16.04. Other Linux environments should work just fine. I cannot get the code to compile on Mac OS X, and I have not tried Windows.

  1. Make sure `binutils` and `gcc` are installed.
  
  2. Install `gnu-efi`. On Ubuntu this can be done with:

    `sudo apt-get install gnu-efi`
  
  3. Install [`tup`](http://gittup.org/tup/) build manager.
    
  4. Run `tup` from the base directory to start the compilation.
  
  5. This will produce an EFI bootloader file (`build/bootloader`) and a kernel file (`build/kernel`) that can be used to boot a virtual machine.


**Running in VirtualBox:**

  1. Install [VirtualBox](https://www.virtualbox.org/).
  
  2. Create a new virtual machine with type 'Other' and version 'Other/Unknown (64-bit)'.
  
  3. Highlight the new virtual machine. Go to Settings->System and check 'Enable EFI' under Extended Features.
  
  4. Create a new FAT-16 formatted CD-image (on Mac OS X this can be done in Disk Utility) and copy `build/bootloader` that you compiled earlier into a file called `/EFI/BOOT/BOOTx64.EFI` on the disk image. Additionally, copy `kernel` into the root directory of the disk image.
  
  5. Attach the disk image as a virtual CD to the VM with a SATA controller.
  
  6. Add a virtual HD under the same SATA controller. You can delete the existing IDE controller.

  6. Boot the VM, it should boot off of the CD image and into MosquitOS.
  
