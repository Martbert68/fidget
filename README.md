# fidget
This was an exploration of the PiPico box with 10 lamps and 10 switches, just to see what is possible.

The code uses both cores of the PiPico, one to bitsmash the UART to PWM the LEDs and produce some crude sounds. The other code does the game play 
and checks the switches.


Youtube description is here
https://youtu.be/IM0ZgJtdikM

If you want to build one of these then GPIO pins 2-11 are LEDS 12-21 are switches 22 is sound... although the sound component of the code is currently in a state
state of flux.

