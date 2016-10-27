Written by Matthew Ng-Wai Shing 19/09/2016

This folder contains all the files required to flash an ETRX357 with the 
Aurora dimmer firmware written by Telegesis for the AONE control manufactured 
by Aurora China.

To do this:
1. Connect the ETRX357 to an ISA3
2. Connect the ISA3 to the PC via a USB port
3. Execute "downloadErase.bat"

Notes:
 - ISA3 should be set to DCHP
 - You can modify the first two digits in "Aurora_tokens.txt" to set the mode
	RGBW = 00FFFFFFFFFFFFFF
	CX = 01FFFFFFFFFFFFFF
	DIM = 02FFFFFFFFFFFFFF
	PLC = 03FFFFFFFFFFFFFF
