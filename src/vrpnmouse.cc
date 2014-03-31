/***
 * vrpnmouse.cc
 ***
 * Author: Johannes Zarl <johannes.zarl@jku.at>
 * Institution: Johannes Kepler University, Altenbergerstraße 69, 4040 Linz, Austria
 ***/
#include <iostream>
#include <cstdlib>
// for sscanf:
#include <cstdio>
#include <getopt.h>
#include <X11/Xlib.h> 
#include <X11/extensions/XTest.h> 
#include <vrpn_Connection.h>
#include <vrpn_Tracker.h>
#include <vrpn_Button.h>
#include <quat.h>

#define VERSION "1.1-20140115"

/**
 * ScreenPlane is a description of the screen plane in the tracker coordinate system.
 */
struct ScreenPlane {
	public:
	ScreenPlane()
	: z(0.0), xmin(0.0), xmax(0.0),ymin(0.0),ymax(0.0) {}
	ScreenPlane(double _z, double _xmin, double _xmax, double _ymin, double _ymax)
	: z(_z), xmin(_xmin), xmax(_xmax),ymin(_ymin),ymax(_ymax) {}
	/// The plane is always on a z-plane.
	double z;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
};

struct MouseCoord {
	public:
		MouseCoord()
			: x(0), y(0) {}
		MouseCoord(int _x, int _y)
			: x(_x), y(_y) {}
		int x;
		int y;
};

/**
 * Definition of short and long commandline-parameters.
 * NB: keep short_options and long options in sync! 
 */
#define short_options "hvc"
static struct option long_options[] = {
	{"help", 0, 0, 'h'},
	{"version", 0, 0, 0},
	{"verbose", 0, 0, 'v'},
	{"quitbutton", 1, 0, 0},
	{"movebutton", 1, 0, 0},
	{"lmb", 1, 0, 0},
	{"mmb", 1, 0, 0},
	{"rmb", 1, 0, 0},
	{"calibrate",0,0,0},
	{"calibration-data",1,0,'c'},
	{"screen",1,0,0},
	{0, 0, 0, 0}
};
#define OPTINDEX_VERSION 1
#define OPTINDEX_QUITBUTTON 3
#define OPTINDEX_MOVEBUTTON 4
#define OPTINDEX_LMB 5
#define OPTINDEX_MMB 6
#define OPTINDEX_RMB 7
#define OPTINDEX_CALIBRATE 8
#define OPTINDEX_SCREEN 10

// if your vrpn input device uses more buttons, adjust here:
#define MAX_BUTTONS 16
enum ButtonAction { ButtonNone = 0, ButtonQuit, ButtonMove, ButtonLeft, ButtonMiddle, ButtonRight };
ButtonAction mouseactions[MAX_BUTTONS];
// index of the tracker to use
int trackerId = -1;
// X11 display:
Display *display = 0;
int screenId = 0;
// dimensions of the current screen:
int screenDimX = 0;
int screenDimY = 0;

// whether to calibrate the coordinate system before starting
bool calibrate = false;
q_type calibration_data_orientation = Q_ID_QUAT;
// JKU CurvedScreen middle screen coordinates:
ScreenPlane projectionScreen (-2.641, -1.750,1.750, 0.0,2.750);

// whether to exit the mainloop
bool do_quit = false;
// be verbose?
unsigned int verbose = 0;
// whether the mouse should be moved by the current tracker movement
// initialised to true, so that mouse always moves if no movebutton was set
bool do_move_mouse = true;

/**
 * Print usage information to stdout.
 */
void printHelp()
{
	std::cout << "vrpnmouse OPTIONS... TRACKERURI ID [BUTTONURI]" << std::endl;
	std::cout << "vrpnmouse -h | --help" << std::endl;
	std::cout << "vrpnmouse --version" << std::endl;
	std::cout << std::endl;
	std::cout << "vrpnmouse connects to a vrpn device and creates mouse events" << std::endl;
	std::cout << std::endl;
	std::cout << "[--calibrate]      Run calibration on start." << std::endl;
	std::cout << "[--calibration-data ARG| -c ARG], ARG = [double,double,double,double]" << std::endl;
	std::cout << "                   Set calibration quaternion [X,Y,Z,W]." << std::endl;
	std::cout << "                   (default: [" << calibration_data_orientation[Q_X]
		<< "," << calibration_data_orientation[Q_Y]
		<< "," << calibration_data_orientation[Q_Z]
		<< "," << calibration_data_orientation[Q_W]
		<< "])" << std::endl;
	std::cout << "[--screen ARG], ARG = [double,double,double,double,double]" << std::endl;
	std::cout << "                   Set the screen dimensions, in tracker units [z,xmin,xmax,ymin,ymax]." << std::endl;
	std::cout << "                   (default: [" << projectionScreen.z
		<< "," << projectionScreen.xmin
		<< "," << projectionScreen.xmax
		<< "," << projectionScreen.ymin
		<< "," << projectionScreen.ymax
		<< "])" << std::endl;
	std::cout << "[--lmb ID] [--mmb ID] [--rmb ID]" << std::endl;
	std::cout << "                   Send a left/middle/right mouse button event when button with index ID is pressed." << std::endl;
	std::cout << "[--movebutton ID]  Only move the mouse when the button with index ID is pressed." << std::endl;
	std::cout << "[--quitbutton ID]  Quit when the button with index ID is pressed." << std::endl;
	std::cout << "[--verbose | -v]   Print events and be generally verbose during operations." << std::endl;
	std::cout << "                   Apply several times for greater verbosity:" << std::endl;
	std::cout << "                   1 ... print actions" << std::endl;
	std::cout << "                   2 ... mouse movement" << std::endl;
	std::cout << "                   3 ... coordinate mapping and debug info" << std::endl;
	std::cout << "TRACKERURI         The address of the vrpn tracker device." << std::endl;
	std::cout << "BUTTONURI          The address of the vrpn button device." << std::endl;
	std::cout << std::endl;
}

/**
 * Open a connection to the X11 display and check for necessary extensions.
 * @return a Display on success, or NULL on error.
 */
Display *initDisplay()
{
	Display *dpy = XOpenDisplay(0);

	if ( ! dpy )
	{
		std::cerr << "Couldn't open display!" << std::endl;
		return NULL;
	}

	// do we need this?
	//XFlush(dpy);

	// Check for XTest extension:
	int event_base, error_base;
	int XTest_version_major, XTest_version_minor;
	if ( XTestQueryExtension(dpy, &event_base,&error_base, &XTest_version_major,&XTest_version_minor))
	{
		if (verbose)
			std::cerr << "XTest extension version " << XTest_version_major <<"."<< XTest_version_minor << " found." << std::endl;
	} else {
		std::cerr << "XTest extension not supported!" << std::endl;
		XCloseDisplay(dpy);
		return NULL;
	}

	screenId = DefaultScreen( dpy );
	// query screen dimensions:
	screenDimX = WidthOfScreen( DefaultScreenOfDisplay( dpy ) );
	screenDimY = HeightOfScreen( DefaultScreenOfDisplay( dpy ) );
	if ( verbose )
		std::cerr << "Screen resolution: " << screenDimX << "x" << screenDimY << std::endl;

	return dpy;
}

/**
 * Cleanup X11-related stuff.
 */
void closeDisplay(Display *dpy)
{
	XFlush(dpy);
	XCloseDisplay(dpy);
}

/**
 * Callback function to get the buttondata.
 * This is also responsible for sending mouse button events.
 */
void VRPN_CALLBACK handle_button (void *userdata, const vrpn_BUTTONCB b)
{
	if (verbose >= 3)
		std::cout << "Button " << b.button << ( b.state?" pressed":" released") << std::endl;
	if ( b.button < MAX_BUTTONS && b.button >= 0 )
	{
		if (calibrate)
		{
			calibrate = false;
			// since we just captured the current rotation, we need to invert it before we can use it:
			q_invert(calibration_data_orientation,calibration_data_orientation);
			std::cout << "Calibration data: ["
				<< calibration_data_orientation[Q_X] << ","
				<< calibration_data_orientation[Q_Y] << ","
				<< calibration_data_orientation[Q_Z] << ","
				<< calibration_data_orientation[Q_W] << "]"
				<< std::endl;
		} else {
			switch (mouseactions[b.button])
			{
				case ButtonNone:
					break;
				case ButtonQuit:
					if (verbose)
						std::cout << "[ButtonQuit]" << std::endl;
					do_quit = true;
					break;
				case ButtonMove:
					if (verbose)
						std::cout << "[ButtonMove]" << std::endl;
					do_move_mouse = ( b.state == 1 );
					break;
				case ButtonLeft:
					if (verbose)
						std::cout << "[ButtonLeft]" << std::endl;
					XTestFakeButtonEvent(display, Button1, b.state, 0);
					XFlush(display);
					break;
				case ButtonMiddle:
					if (verbose)
						std::cout << "[ButtonMiddle]" << std::endl;
					XTestFakeButtonEvent(display, Button2, b.state, 0);
					XFlush(display);
					break;
				case ButtonRight:
					if (verbose)
						std::cout << "[ButtonRight]" << std::endl;
					XTestFakeButtonEvent(display, Button3, b.state, 0);
					XFlush(display);
					break;
			}
		}
	} else {
		std::cerr << "Button ID outside range: " << b.button << " is not < " << MAX_BUTTONS << std::endl;
	}
}

/**
 * Make sure the value is bounded to the given bounds.
 */
inline int boundValue(int v, int low, int high)
{
	if (v <= low)
		return low;
	if (v >= high)
		return high;
	return v;
}

/**
 * Map 3D tracker data to a 2D coordinate space.
 */
struct MouseCoord trackerToCoords( const q_vec_type pos, const q_type quat)
{
	q_vec_type wand;
	double scale;
	// a vector pointing to the screen
	q_vec_set( wand, 0.0, 0.0, -1.0 );
	// apply orientation correction:
	q_xform( wand, calibration_data_orientation, wand);
	// apply orientation:
	q_xform( wand, quat, wand);

	// compute scale
	scale = projectionScreen.z - pos[Q_Z];
	scale /= wand[Q_Z];
	q_vec_scale( wand, scale, wand);

	// project wand orientation onto screen:
	q_vec_add( wand, wand, pos);
	if (verbose>=3)
		if ( ((projectionScreen.z - wand[Q_Z]) >= 0.0001 )
				|| ((projectionScreen.z - wand[Q_Z]) <= -0.0001 ) )
			std::cerr << "Projection onto z_plane seems off: " << wand[Q_X] <<", "<< wand[Q_Y] <<", "<< wand[Q_Z] <<"; z plane: " << projectionScreen.z << std::endl;

	// physical coordinates to screen coordinates (pixels)
	double x,y;
	x = (wand[Q_X] - projectionScreen.xmin) / (projectionScreen.xmax - projectionScreen.xmin) * screenDimX;
	// screen (0,0) is upper left, physical z-plange (0,0) is lower left -> substract from screemDimY:
	y = screenDimY - (wand[Q_Y] - projectionScreen.ymin) / (projectionScreen.ymax - projectionScreen.ymin) * screenDimY;
#ifdef DEBUG_TRACKERMAPPING
	std::cerr << "( " << x << " , " << y << " )" << std::endl;
#endif
	struct MouseCoord result(
			boundValue( (int)x, 0, screenDimX)
			, boundValue( (int)y, 0, screenDimY)
			);
	return result;
}

/**
 * Callback function to get the tracker data.
 * Depending on the do_move_mouse state, the mouse is moved according to the tracker position/orientation.
 */
void VRPN_CALLBACK handle_tracker (void *userdata, const vrpn_TRACKERCB trackerData)
{
	static bool did_move_mouse=false;
	if ( trackerData.sensor != trackerId)
		return;
	if (calibrate)
	{
		// in calibration mode, we just take notice of the orientation:
		q_copy(calibration_data_orientation, trackerData.quat);
		return;
	}
	if (do_move_mouse)
	{
		if ( verbose>=2 && ! did_move_mouse )
			std::cout << "Mouse move start." << std::endl;
		struct MouseCoord mc = trackerToCoords( trackerData.pos, trackerData.quat );
		if (verbose >= 4)
			std::cout << "[" << trackerData.pos[Q_X] << ", " 
				<< trackerData.pos[Q_Y] << ", "
				<< trackerData.pos[Q_Z] << "], [( "
				<< trackerData.quat[Q_X] << ", "
				<< trackerData.quat[Q_Y] << ", "
				<< trackerData.quat[Q_Z] << "), "
				<< trackerData.quat[Q_W] << "] --> ";
		if (verbose >= 2)
			std::cout << "[ " << mc.x << " , " << mc.y << " ]" << std::endl;
		XTestFakeMotionEvent(display, screenId, mc.x, mc.y, 0);
		XFlush(display);
	} else {
		if ( verbose>=2 && did_move_mouse )
			std::cout << "Mouse move end." << std::endl;
	}
	did_move_mouse = do_move_mouse;
}

void printMouseActions()
{
	for (int i = 0; i < MAX_BUTTONS ; i++ )
	{
		std::cout << "Button " << i << ": ";
		switch (mouseactions[i])
		{
			case ButtonNone:
				//std::cout << "ButtonNone";
				break;
			case ButtonQuit:
				std::cout << "ButtonQuit";
				break;
			case ButtonMove:
				std::cout << "ButtonMove";
				break;
			case ButtonLeft:
				std::cout << "ButtonLeft";
				break;
			case ButtonMiddle:
				std::cout << "ButtonMiddle";
				break;
			case ButtonRight:
				std::cout << "ButtonRight";
				break;
		}
		std::cout << std::endl;
	}
}

bool setMouseAction(int index, ButtonAction a)
{
	if ( index < 0 || index >= MAX_BUTTONS )
	{
		std::cerr << "Invalid button index: " << index << std::endl;
		return false;
	}
	mouseactions[index] = a;
	return true;
}

int main(int argc, char **argv)
{
	// parse commandline:
	int option_index = 0;
	int c;
	bool use_buttons = false;
	while ( -1 != (c = getopt_long (argc, argv, short_options,
				long_options, &option_index)) )
	{
		switch (c)
		{
			case 0: /* long options */
				switch ( option_index )
				{
					case OPTINDEX_QUITBUTTON:
						if (! setMouseAction(atoi(optarg), ButtonQuit) )
							return 1;
						use_buttons = true;
						break;
					case OPTINDEX_MOVEBUTTON:
						if (! setMouseAction(atoi(optarg), ButtonMove) )
							return 1;
						// only begin movement when move button is pressed:
						do_move_mouse = false;
						use_buttons = true;
						break;
					case OPTINDEX_LMB:
						if (! setMouseAction(atoi(optarg), ButtonLeft) )
							return 1;
						use_buttons = true;
						break;
					case OPTINDEX_MMB:
						if (! setMouseAction(atoi(optarg), ButtonMiddle) )
							return 1;
						use_buttons = true;
						break;
					case OPTINDEX_RMB:
						if (! setMouseAction(atoi(optarg), ButtonRight) )
							return 1;
						use_buttons = true;
						break;
					case OPTINDEX_VERSION:
						std::cout << "vrpnmouse version " << VERSION << std::endl;
						return 0;
					case OPTINDEX_CALIBRATE:
						calibrate = true;
						use_buttons = true;
						break;
					case OPTINDEX_SCREEN:
						{
							double z,xmin,xmax,ymin,ymax;
							int numparsed = sscanf(optarg, "[%lf,%lf,%lf,%lf,%lf]"
								, &z, &xmin, &xmax, &ymin, &ymax);
							if ( numparsed != 5 )
							{
								perror(optarg);
								std::cerr << "Parsing of screen definition failed!" << std::endl;
								std::cerr << "Number of assigned elements: "<< numparsed << std::endl;
								return 1;
							}
							projectionScreen = ScreenPlane(z,xmin,xmax,ymin,ymax);
							break;
						}
					default:
						std::cerr << "Unknown option index: " << option_index << std::endl;
						std::cerr << "Please contact the program author!" << std::endl;
						return 1;

				}
				break;
				/* short options: */
			case 'v':
				verbose++;
				break;
			case 'c':
				{
					int numparsed = sscanf(optarg,"[%lf,%lf,%lf,%lf]"
								, &calibration_data_orientation[Q_X]
								, &calibration_data_orientation[Q_Y]
								, &calibration_data_orientation[Q_Z]
								, &calibration_data_orientation[Q_W]
								);
					if ( numparsed != 4 )
					{
						perror(optarg);
						std::cerr << "Parsing of calibration-data failed!" << std::endl;
						std::cerr << "Number of assigned elements: "<< numparsed << std::endl;
						return 1;
					}
				}
				break;
			case 'h':
			default:
				printHelp();
				return 0;
		}
	}
	// remaining (non-)option is the vrpn-uri :
	if ( optind >= argc )
	{
		std::cerr << "No tracker URI supplied!" << std::endl;
		return 1;
	}
	// we have our tracker URI
	// -> connect to server:
	vrpn_Tracker_Remote *trackerdev = new vrpn_Tracker_Remote( argv[optind++] );
	if ( optind >= argc )
	{
		std::cerr << "No tracker ID supplied!" << std::endl;
		return 1;
	}
	trackerId = atoi( argv[optind++] );

	vrpn_Button_Remote *buttondev = NULL;
	if ( optind < argc )
	{
		buttondev = new vrpn_Button_Remote( argv[optind++] );
	} else {
		// no buttons available -> check if buttons are requested
		if (use_buttons)
		{
			std::cerr << "Mouse button actions requested, but no button URI given!" << std::endl;
			std::cerr << "Please set the button device!" << std::endl;
			return 1;
		}
	}
	// anything left on the commandline is ignored:
	if ( optind < argc )
	{
		std::cerr << "Warning: ignoring some command line options!" << std::endl;
	}
	if (verbose)
		std::cerr << "Verbosity: " << verbose << std::endl;

	if (use_buttons && verbose>=3)
		printMouseActions();

	// initialise Display:
	display = initDisplay();
	if (! display)
		return 1;

	// register callbacks:
	if (verbose)
		std::cerr << "Registering VRPN callbacks." << std::endl;
	trackerdev->register_change_handler(NULL, handle_tracker);
	if (buttondev)
		buttondev->register_change_handler(NULL, handle_button);
	
	if (verbose)
		std::cerr << "Entering main loop." << std::endl;
	if (calibrate)
		std::cout << "Point the wand towards the screen and press a button..." << std::endl;
	while (!do_quit)
	{
		trackerdev->mainloop();
		if (buttondev)
			buttondev->mainloop();
	}
	if (verbose)
		std::cerr << "Main loop done." << std::endl;
	
	closeDisplay(display);
	delete buttondev;
	delete trackerdev;
	return 0;
}
