MosquitOS is a tiny x86_64 UEFI-based operating system built from scratch.

**Goals:**

  - UEFI Bootloader
  - Pre-emptive multitasking
  - Thread support
  - Multiprocessor support
  - Basic filesystem
  - Virtual memory


**Compilation:**

*Note:* I do all development on Ubuntu 14. Other linux environments should work just fine. I cannot get the code to compile on Mac OS X, and I have not tried Windows.

  1. Install `gnu-efi`. On Ubuntu this can be done with:

    `sudo apt-get install gnu-efi`
    
  2. Run `make` from the base directory.
  
  3. This will produce an EFI bootloader file (`loader.efi`) that can be run from an EFI Shell (see below).


**Running in VirtualBox:**

  1. Install [VirtualBox](https://www.virtualbox.org/).
  
  2. Create a new virtual machine with type 'Other' and version 'Other/Unknown (64-bit)'. You do not need to make a hard drive for this VM.
  
  3. Highlight the new virtual machine. Go to Settings->System and check 'Enable EFI' under Extended Features.
  
  4. Create a new CD-image (on Mac OS X this can be done in Disk Utility) and copy `loader.efi` that you compiled earlier into the base directory of the image.
  
  5. Start the VM and attach the disk image as a virtual CD drive.
  
  6. The VM should boot the the UEFI Shell. Type `fs0:` and press enter to mount the first available filesystem (the CD image we created in step 4 and attached in step 5).
  
  7. Type the name of any `.efi` file to execute that file (e.g., `loader.efi`).
  
