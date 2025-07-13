# FlexDiscovery
   Uses Flex API without .net or FlexLib (i.e. uses native text-based API commands)

   Utility to test Flex API commands and to show TCP/IP traffic from the radio.
   Displays the first Flex discovery UDP packet that it finds.  Other Flex UDP traffic is not read.

   The commented out commands are what I have been experimenting with.  
   There are many many more that can be added here.  Most are commented out so that
   you can focus on just what you are interested in.

   The main goal is to provide a way to try out commands before putting them into your
   program.  To get the proper parameter name (may vary slightly from the wiki...), 
   uncomment the appropriate subscription(s), run this program, then activate the control
   in SmartSDR, or on your Maestro.  The response from the radio will be displayed on 
   the console screen.  The parameters have good instruction set reciprocity,
   so this will lead you to the correct syntax.

   A good example is VOX.  The wiki says:
   C41|transmit set vox=1

   It doesn't work...

   Run this utility with "sub tx all" uncommented and turn VOX on and off.  The radio
   responds with:
	S1F5C6BC|transmit vox_enable=1 vox_level=68 vox_delay=16
	S1F5C6BC|transmit vox_enable=0 vox_level=68 vox_delay=16

   So now you can deduce that the actual command is: 
   C41|transmit set vox_enable=1

   Note that "s" and "set" are interchangable.  In my commands, I always use CD instead of C
   so that I get the verbose debug version of the radio responses.
