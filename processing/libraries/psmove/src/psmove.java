package psmovelib;

import processing.core.*;
import io.thp.psmove.*;

// ===============================================

// ===============================================

// ===============================================


public class psmove
{
	PApplet parent;
	public psmove(PApplet parent)
	{
		this.parent = parent;
		parent.registerDispose(this);
	}

	public static int getNumConnected()
	{
		return psmoveapi.count_connected();
	} 

/*
	public static psmovecontroller getController()
	{
		return getController(0);
	}

	public static PSMove getController(int id)
	{
		return new psmovecontroller(new PSMove(id));
	}
*/

	public void pre()
	{
		
	}

	public void dispose() 
	{

	}
}

/*


// ===============================================

public class psmovecontroller
{
	private PSMove move;


	private int updateInterval;

	public psmovecontroller()
	{
		updateInterval = 30;
	}

	public psmovecontroller(int _updateInterval)
	{
		updateInterval = _updateInterval;
	}

	public void update()
	{
		move.poll();
		ProcessMoveData();
			//lastTime = thisTime;
			
			//prevButtons = currentButtons;
			
			//uint rawButtons = 0;
			
			// NOTE! There is potentially data waiting in queue. 
			// We need to poll *all* of it by calling psmove_poll() until the queue is empty. Otherwise, data might begin to build up.
			//while (psmove_poll(handle) > 0) 
			//{
				// We are interested in every button press between the last update and this one:
			//	rawButtons = rawButtons | psmove_get_buttons(handle);
			//}
			//currentButtons = rawButtons;
			
			// For acceleration, gyroscope, and magnetometer values, we look at only the last value in the queue.
			// We could in theory average all the acceleration (and other) values in the queue for a "smoothing" effect, but we've chosen not to.
			//ProcessMoveData();
		
			//ComputeAverageFilter();
			//ComputeIMUFilter(dt);
			//ComputeKalmanFilter();
			
			// Send a report to the controller to update the LEDs and rumble.
			//if (psmove_update_leds(handle) == 0)
			//{
				// If it returns zero, the controller must have disconnected (i.e. out of battery or out of range),
				// so we should fire off any events and disconnect it.
			//	OnControllerDisconnected(this, new EventArgs());
			//	Disconnect();
			//}		

	}

	public Vector3 RawAccel	= new Vector3();
	public Vector3 Accel	= new Vector3();
	public Vector3 RawGyro	= new Vector3();
	public Vector3 Gyro		= new Vector3();
	public Vector3 Magnet	= new Vector3();

	public Button Buttons;
	public float Trigger;
	public int Battery;
	public int Temperature;

	// Constant for converting raw accelerometer values to 9.8m/s^2, i.e. rawAccel/gConst=accel (in G's)
	final float gConst = 4326f; 
		
	// Constant for converting raw gyro values to radians/s, i.e. rawGyro/rConst=gyro (in rad/s)
	final float rConst = 7509.8f;

	public void ProcessMoveData()
	{
		int x=0, y=0, z=0;
			
		//prevTrigger = trigger;
		Trigger = move.get_trigger/255;
			
		// Get Raw Acceleration Values
		//psmove_get_accelerometer(handle, ref x, ref y, ref z);			
		RawAccel.x = move.getAx();
		RawAccel.y = move.getAy();
		RawAccel.z = move.getAz();

		// Convert accelerometer values in radians/s
		// Right now the division is a rough approximation to achieve a range between -6g and 6g
		Accel.x = RawAccel.x/gConst;
		Accel.y = RawAccel.y/gConst;
		Accel.z = RawAccel.z/gConst;

		// TODO: Should these values be converted into a more human-understandable range?
		//psmove_get_gyroscope(handle, ref x, ref y, ref z );
		RawGyro.x = move.getGx();
		RawGyro.y = move.getGy();
		RawGyro.z = move.getGz();
			
		// Compute gyrometer values in radians/s
		Gyro.x = RawGyro.x/rConst;
		Gyro.y = RawGyro.y/rConst;
		Gyro.z = RawGyro.z/rConst;
			
		// TODO: Should these values be converted into a more human-understandable range?
		//psmove_get_magnetometer(handle, ref x, ref y, ref z );
		Magnet.x = move.getMx();
		Magnet.y = move.getMy();
		Magnet.z = move.getMz();
			
		// TODO: Add hook to get battery state (when it is implemented in Thomas Perl's C library).		
		Battery = move.get_battery();
			
		// TODO: Add hook to get temperature state (when it is implemented in Thomas Perl's C library).
		Temperature = move.get_temperature();
		
	//	SendButtonEvents();
	}


	public psmovecontroller(PSMove themove)
	{		
		move = themove;
	}

}

*/
