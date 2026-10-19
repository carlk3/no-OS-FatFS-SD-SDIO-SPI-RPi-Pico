
Date 2026.10.18
What's new:
SD card SDIO mode is tested with a Waveshare RP2350-PiZero board.
Since Waveshare RP2350-PiZero only has one SD card socket, only tested one SDIO port.
The command_line project in examples/commoand_line folder is used for benchmark.

The benchmark results: (command_line project "bench" test)
----------------------------------------
SPI mode: (Waveshare RP2350-PiZero board, spi mode)
FILE_SIZE_MB = 5                                                                
BUF_SIZE = 65536 
write speed and latency                                                         
speed,   max,  min  ,avg                                                               
KB/Sec, usec, usec, usec                                                           
1255.2,52506,51990,52159                                                        
1255.8,52501,51989,52149                                                                                                    
                                                                                
read speed and latency                                                          
speed,  max,  min,   avg                                                               
KB/Sec, usec, usec, usec                                                           
1266.4,51754,51715,51744                                                        
1266.4,51752,51717,51738  

----------------------------------------
SDIO mode: (Waveshare RP2350-PiZero board, SDIO mode D0=GPIO40)
FILE_SIZE_MB = 5                                                                
BUF_SIZE = 65536 
write speed and latency                                                         
speed,  max, min,  avg                                                               
KB/Sec, usec,usec,usec                                                           
7620.5,11101,8145,8544                                                          
7755.7, 8656,8144,8409                                                           
                                                                                                                                                                                                  
read speed and latency                                                          
speed,  max, min, avg                                                               
KB/Sec,usec,usec,usec                                                           
8282.6,7914,7898,7909                                                           
8295.7,7914,7888,7906   




