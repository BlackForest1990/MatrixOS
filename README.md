# MatrixOS step by step

## first step

### quick install

sudo apt-get install build-essential nasm genisoimage bochs bochs-sdl

### environment
windows Ubuntu LTS 22.04.5

### booting
![image](https://github.com/user-attachments/assets/e3cfe5df-2ecf-4223-b9ab-408851ca6899)
when the pc is turned on, the computer will start a small program that adheres to the BIOS and transfer control to a program called a bootloader.
the bootloader task is to transfer control to us, the operation system developers, and our code. we use GRUB to do this. Afterall, the bootloader transfer
control to the OS.


## reference

<https://littleosbook.github.io/>
