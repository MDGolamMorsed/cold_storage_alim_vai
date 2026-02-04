Micro-controller: esp32-s3
communication: wifi and A7670E gsm module
sensor: dht22, Ds18b20
code comment: doxygen
code format: pin mapping and variable will be in kconfig and func name will be start with file name
task1: give modular code for gsm module
sms or mqtt will use to set humidity and temp parameter threshold. format will be same
if temp or humidity reach threshold then send notification using sms and mqtt
also give a call to 8801521475412 and also print if any ring is ongoing
if at cmd use then also print at cmd reply

#status#
#+8801521475412#
#temp:GT,30.5# (Greater Than)
#temp:LT,10.0# (Less Than)
#temp:R,20.0,25.0# (Range)
#hum:LT,30.0,79.90# 
A085E3F08548/cold_storage/data
A085E3F08548/cold_storage/data
A085E3F08548/cold_storage/alert
A085E3F08548/cold_storage/commands