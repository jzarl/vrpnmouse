VRPN mouse
==========

VRPN mouse allows you to use any VRPN input device to emulate a standard
desktop mouse. In other words, it is a mouse emulation for X11 that maps a 3D,
6-degrees-of-freedom (6DOF) tracking device onto the 2D screen.


Motivation
----------

The motivation behind VRPN mouse is that people sometimes want to use a desktop
program on a powerwall display. While it is possible to just connect a desktop
mouse to the display directly, often this is impracticable (e.g. no suitable
surface for using the mouse) and a 6DOF tracking device is already available
without additional setup.


Prerequisites
-------------

 - X11 server with XTest extension enabled
 - VRPN


How to use
----------

VRPN mouse needs one input tracking device, plus one button device (both can be
the same). A typical setup might look like this:

	vrpnmouse --movebutton 4 --lmb 1 --mmb 2 --rmb 3 powerwalltracker@mytrackinghost 4 wiimote@mytrackinghost

This means that the tracker with index 4 on the VRPN device
```powerwalltracker@mytrackinghost``` is used, together with the button input
from ```wiimote@mytrackinghost```.

In the example shown, mouse movement is only enabled when button 4 is pressed
(```--movebutton 4```). If you want the movement to be always enabled, omit the
```--movebutton``` argument.


Calibration
-----------

In case your tracker device is not perfectly aligned with the "perceived
orientation" of the device, you can calibrate the orientation by adding the
```--calibrate``` argument.  At startup, the program will then prompt you to
point the device towards the screen and press any button.

As part of the calibration step, the calibration data is displayed as follows:
	Calibration data: [0.585161,0.355244,0.359502,0.634164]

To avoid the manual prompt in the future, you can directly set the calibration
data on startup:

	vrpnmouse --movebutton 4 --lmb 1 --mmb 2 --rmb 3 powerwalltracker@mytrackinghost 4 wiimote@mytrackinghost -c [0.585161,0.355244,0.359502,0.634164]

Please note that no whitespace is allowed within the calibration data.


Physical Screen setup
---------------------

While the program comes with a default setting for the physical screen setup,
you will most certainly want to adopt this to your needs.

Assuming that you use a powerwall setup which is 5 meters wide, and 2 meters
high from the floor. If the coordinate-system origin is situated 1.5 meters in
front of the screen-middle on the floor, the following screen definition would
suit your needs:

	vrpnmouse --movebutton 4 --lmb 1 --mmb 2 --rmb 3 powerwalltracker@mytrackinghost 4 wiimote@mytrackinghost --screen [-1.5,-2.5,2.5,0.0,2.0]

Please note that no whitespace is allowed within the screen definition.

BUGS
====

 - vrpnmouse uses sscanf for parsing of the --calibration-data and --screen arguments
 - jitter in the tracking data is not smoothed out
