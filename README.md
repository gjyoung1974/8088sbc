# 8088 Single Board Computer

What is this?
- This is a fork of the Excellent work of Elijah Miller of [homebrew8088](https://www.homebrew8088.com/home/raspberry-pi)

Why?
- I wanted a physical hardware device to run simple ia16 assembly language on
- I have a small collection of 8088 hardware and wanted to play with them. 
- Getting back to my roots! PCXT, 8088 + Dos
- Having fun getting the "Embedded Linux Kernel Subset": [ELKS](https://github.com/jbruchon/elks) to run on real hardware again  
  (without big, heavy, noisy, power sucking, and sometimes stinky old PCXT hardware)

What have I done so far?
- Added a cmake project (CMakeLists.txt)
- Added a script that builds the project 
- Explicit casts of some characters to resolve a compiler narrowing issue: [Wnarrowing](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#index-Wnarrowing)

Todo:
- Can we replace the WiringPI Library with some other SPI library and run on Desktop vs Raspberry PI Hardware?   
  Like one of the several USB to SPI interfaces, ch341, ft232, etc?
- can we port the UI to SDL vs nCurses similar to the V2 version of the project? [raspberry-pi-second-projec](https://www.homebrew8088.com/home/raspberry-pi-second-project)  
- Profiling/optimization, can we otimize the responsiveness similar to: [elijah-millers-nec-v30-on-a-pi-hat/](https://virtuallyfun.com/wordpress/2021/06/04/elijah-millers-nec-v30-on-a-pi-hat/)    

[comment]: <> (https://virtuallyfun.com/wordpress/2021/06/04/elijah-millers-nec-v30-on-a-pi-hat/)

---
2021 - gjyoung1974@gmail.com
