 /**
 * UniMove API - A Unity plugin for the PlayStation Move motion controller
 * Copyright (C) 2012, 2013, Copenhagen Game Collective (http://www.cphgc.org)
 *                           Patrick Jarnfelt
 *                           Douglas Wilson (http://www.doougle.net)
 *
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 **/

 /**
 * IMPORTANT NOTES!
 *
 * -- This API has been compiled for Mac OSX (10.7 and later) specifically.
 *
 * -- This API assumes that the controller has already been paired and connected by Bluetooth beforehand.
 *    To pair a controller(s), use the Pairing Utility provided by the PS Move API http://thp.io/2010/psmove/.
 *    To connect a controller by Bluetooth, just press the PS button after pairing it.
 *    You can also use the controllers by USB, but with limited functionality (see below).
 *
 * -- Features include:
 *
 *  - Setting the RGB LED color and rumble intensity (USB and Bluetooth)
 *  - Read the status of the digital buttons (Bluetooth only)
 *  - Read the status of the analog trigger (Bluetooth only)
 *  - Read values for the internal sensors (Bluetooth only):
 *     - accelorometer
 *     - gyroscope
 *     - magnetometer
 *     - temperature
 *     - battery level
 *
 * Please see the README for more information!
 **/

using System;
using UnityEngine;
using System.Runtime.InteropServices;

#region enums and structs

/// <summary>
/// The Move controller can be connected by USB and/or Bluetooth.
/// </summary>
public enum PSMoveConnectionType
{
    Bluetooth,
    USB,
    Unknown,
};

// Not entirely sure why some of these buttons (R3/L3) are exposed...
public enum PSMoveButton
{
    L2 = 1 << 0x00,
    R2 = 1 << 0x01,
    L1 = 1 << 0x02,
    R1 = 1 << 0x03,
    Triangle = 1 << 0x04,
    Circle = 1 << 0x05,
    Cross = 1 << 0x06,
    Square = 1 << 0x07,
    Select = 1 << 0x08,
    L3 = 1 << 0x09,
    R3 = 1 << 0x0A,
    Start = 1 << 0x0B,
    Up = 1 << 0x0C,
    Right = 1 << 0x0D,
    Down = 1 << 0x0E,
    Left = 1 << 0x0F,
    PS = 1 << 0x10,
    Move = 1 << 0x13,
    Trigger = 1 << 0x14,    /* We can use this value with IsButtonDown() (or the events) to get
                             * a binary yes/no answer about if the trigger button is down at all.
                             * For the full integer/analog value of the trigger, see the corresponding property below.
                             */
};

// Used by psmove_get_battery().
public enum PSMove_Battery_Level {
    Batt_MIN = 0x00, /*!< Battery is almost empty (< 20%) */
    Batt_20Percent = 0x01, /*!< Battery has at least 20% remaining */
    Batt_40Percent = 0x02, /*!< Battery has at least 40% remaining */
    Batt_60Percent = 0x03, /*!< Battery has at least 60% remaining */
    Batt_80Percent = 0x04, /*!< Battery has at least 80% remaining */
    Batt_MAX = 0x05, /*!< Battery is fully charged (not on charger) */
    Batt_CHARGING = 0xEE, /*!< Battery is currently being charged */
    Batt_CHARGING_DONE = 0xEF, /*!< Battery is fully charged (on charger) */
};

public enum PSMove_Frame {
    Frame_FirstHalf = 0, /*!< The older frame */
    Frame_SecondHalf, /*!< The most recent frame */
};

public class UniMoveButtonEventArgs : EventArgs
{
    public readonly PSMoveButton button;

    public UniMoveButtonEventArgs(PSMoveButton button)
    {
        this.button = button;
    }
}

#endregion

public class UniMoveController : MonoBehaviour
{
    #region private instance variables

    /// <summary>
    /// The handle for this controller. This pointer is what the psmove library uses for reading data via the hid library.
    /// </summary>
    private IntPtr handle;
    private bool disconnected = false;

    private float timeElapsed = 0.0f;
    private float updateRate = 0.05f;   // The default update rate is 50 milliseconds

    private static float MIN_UPDATE_RATE = 0.02f; // You probably don't want to update the controller more frequently than every 20 milliseconds

    private float trigger = 0f;
    private uint currentButtons = 0;
    private uint prevButtons = 0;

    private Vector3 rawAccel = Vector3.down;
    private Vector3 accel = Vector3.down;
    private Vector3 magnet = Vector3.zero;
    private Vector3 rawGyro = Vector3.zero;
    private Vector3 gyro = Vector3.zero;

    //Orientation
    private Quaternion orientation;

    // TODO: These values still need to be implemented, so we don't expose them publicly
    private PSMove_Battery_Level battery = PSMove_Battery_Level.Batt_20Percent;
    private float temperature = 0f;

    /// <summary>
    /// Event fired when the controller disconnects unexpectedly (i.e. on going out of range).
    /// </summary>
    public event EventHandler OnControllerDisconnected;

    #endregion
    /// <summary>
    /// Returns whether the connecting succeeded or not.
    ///
    /// NOTE! This function does NOT pair the controller by Bluetooth.
    /// If the controller is not already paired, it can only be connected by USB.
    /// See README for more information.
    /// </summary>
    public bool Init(int index)
    {
        handle = psmove_connect_by_id(index);

        // Error check the result!
        if (handle == IntPtr.Zero) return false;

        // Make sure the connection is actually sending data. If not, this is probably a controller
        // you need to remove manually from the OSX Bluetooth Control Panel, then re-connect.
        return (psmove_update_leds(handle) != 0);
    }

    //Orientation
    //Don't forget to calibrate the magnetometer beforehand with the API's tool
    //Call this and ResetOrientation before you use access orientation, or it won't work
    public void InitOrientation(){
        if(!HasOrientation()){
            if(!HasCalibration()){
                Debug.Log("Move is not calibrated, cannot use orientation");
            }
            else{
                Debug.Log("Move does not have orientation set up, enabling...");
                EnableOrientation();
            }
        }
        }

    /// <summary>
    /// Static function that returns the number of *all* controller connections.
    /// This count will tally both USB and Bluetooth connections.
    /// Note that one physical controller, then, might register multiple connections.
    /// To discern between different connection types, see the ConnectionType property below.
    /// </summary>
    public static int GetNumConnected()
    {
        return psmove_count_connected();
    }

    /// <summary>
    /// The amount of time, in seconds, between update calls.
    /// The faster this rate, the more responsive the controllers will be.
    /// However, update too fast and your computer won't be able to keep up (see below).
    /// You almost certainly don't want to make this faster than 20 milliseconds (0.02f).
    ///
    /// NOTE! We find that slower/older computers can have trouble keeping up with a fast update rate,
    /// especially the more controllers that are connected. See the README for more information.
    /// </summary>
    public float UpdateRate
    {
        get { return this.updateRate; }
        set { updateRate = Math.Max(value, MIN_UPDATE_RATE); }  // Clamp negative values up to 0
    }

    void Update()
    {
        if (disconnected) return;

        // we want to update the previous buttons outside the update restriction so,
        // we only get one button event pr. unity update frame
        prevButtons = currentButtons;

        timeElapsed += Time.deltaTime;


        // Here we manually enforce updates only every updateRate amount of time
        // The reason we don't just do this in FixedUpdate is so the main program's FixedUpdate rate
        // can be set independently of the controllers' update rate.

        if (timeElapsed < updateRate) return;
        else timeElapsed = 0.0f;

        uint buttons = 0;

        // NOTE! There is potentially data waiting in queue.
        // We need to poll *all* of it by calling psmove_poll() until the queue is empty. Otherwise, data might begin to build up.
        while (psmove_poll(handle) > 0)
        {
            // We are interested in every button press between the last update and this one:
            buttons = buttons | psmove_get_buttons(handle);

            // The events are not really working from the PS Move Api. So we do our own with the prevButtons
            //psmove_get_button_events(handle, ref pressed, ref released);
            currentButtons = buttons;
        }


        // For acceleration, gyroscope, and magnetometer values, we look at only the last value in the queue.
        // We could in theory average all the acceleration (and other) values in the queue for a "smoothing" effect, but we've chosen not to.
        ProcessData();

        // Send a report to the controller to update the LEDs and rumble.
        if (psmove_update_leds(handle) == 0)
        {
            // If it returns zero, the controller must have disconnected (i.e. out of battery or out of range),
            // so we should fire off any events and disconnect it.
            OnControllerDisconnected(this, new EventArgs());
            Disconnect();
        }
    }

    void OnApplicationQuit()
    {
        Disconnect();
    }

    /// <summary>
    /// Returns true if "button" is currently down.
    /// </summary>
    public bool GetButton(PSMoveButton b)
    {
        if (disconnected) return false;

        return ((currentButtons & (uint)b) != 0);
    }

    /// <summary>
    /// Returns true if "button" is pressed down this instant.
    /// </summary>
    public bool GetButtonDown(PSMoveButton b)
    {
        if (disconnected) return false;
        return ((prevButtons & (uint)b) == 0) && ((currentButtons & (uint)b) != 0);
    }

    /// <summary>
    /// Returns true if "button" is released this instant.
    /// </summary>
    public bool GetButtonUp(PSMoveButton b)
    {
        if (disconnected) return false;

        return ((prevButtons & (uint)b) != 0) &&  ((currentButtons & (uint)b) == 0);
    }

    /// <summary>
    /// Disconnect the controller
    /// </summary>
    public void Disconnect()
    {
        disconnected = true;
        SetLED(0,0,0);
        SetRumble(0);
        psmove_disconnect(handle);
    }

    /// <summary>
    /// Whether or not the controller has been disconnected
    /// </summary>
    public bool Disconnected
    {
        get { return disconnected; }
    }

    /// <summary>
    /// Sets the amount of rumble
    /// </summary>
    /// <param name="rumble">the rumble amount (0-1)</param>
    public void SetRumble(float rumble)
    {
        if (disconnected) return;

        // Clamp value between 0 and 1:
        byte rumbleByte = (byte) (Math.Min(Math.Max(rumble, 0f), 1f) * 255);

        psmove_set_rumble(handle, (char)rumbleByte);
    }

    /// <summary>
    /// Sets the LED color
    /// </summary>
    /// <param name="color">Unity's Color type</param>
    public void SetLED(Color color)
    {
        if (disconnected) return;

        psmove_set_leds(handle, (char)(color.r * 255), (char)(color.g * 255), (char)(color.b * 255));
    }

    /// <summary>
    /// Sets the LED color
    /// </summary>
    /// <param name="r">Red value of the LED color (0-255)</param>
    /// <param name="g">Green value of the LED color (0-255)</param>
    /// <param name="b">Blue value of the LED color (0-255)</param>
    public void SetLED(byte r, byte g, byte b)
    {
        if (disconnected) return;

        psmove_set_leds(handle, (char)r, (char)g, (char)b);
    }

    /// <summary>
    /// Value of the analog trigger button (between 0 and 1)
    /// </summary>
    public float Trigger
    {
        get { return trigger; }
    }

    /// <summary>
    /// The 3-axis acceleration values.
    /// </summary>
    public Vector3 RawAcceleration
    {
        get { return rawAccel; }
    }

    /// <summary>
    /// The 3-axis acceleration values, roughly scaled between -3g to 3g (where 1g is Earth's gravity).
    /// </summary>
    public Vector3 Acceleration
    {
        get { return accel; }
    }

    /// <summary>
    /// The raw values of the 3-axis gyroscope.
    /// </summary>
    public Vector3 RawGyroscope
    {
        get { return rawGyro; }
    }

    /// <summary>
    /// The raw values of the 3-axis gyroscope.
    /// </summary>
    public Vector3 Gyro
    {
        get { return gyro; }
    }

    /// <summary>
    /// The raw values of the 3-axis magnetometer.
    /// To be honest, we don't fully understand what the magnetometer does.
    /// The C API on which this code is based warns that this isn't fully tested.
    /// </summary>
    public Vector3 Magnetometer
    {
        get { return magnet; }
    }

    //Orientation

    //Returns a quaternion (rotation)
    public Quaternion Orientation{
        get{return orientation;}
    }
    //Returns true if Move is calibrated
    public bool HasCalibration(){
        return psmove_has_calibration(handle)==1;
    }
    //Returns true if Move can be used for orientation
    public bool HasOrientation(){
        return psmove_has_orientation(handle)==1;
    }
    //Enables orientation
    public void EnableOrientation(){
        psmove_enable_orientation(handle,1);
    }
    //Resets orientation to identity quaternion, must be called at least once before orientation will work
    public void ResetOrientation(){
        psmove_reset_orientation(handle);
    }

    /// <summary>
    /// The battery level
    /// </summary>
    public PSMove_Battery_Level Battery
    {
        get { return battery; }
    }

    /// <summary>
    /// The temperature in Celcius
    /// </summary>
    public float Temperature
    {
        get { return temperature; }
    }

    /// <summary>
    /// The serial number of the controller (Bluetooth address)
    /// </summary>
    public string Serial
    {
        get { return psmove_get_serial(handle); }
    }

    /* TODO: These two values still need to be implemented, so we don't expose them publicly... yet!

    public float Battery
    {
        get { return this.battery; }
    }

    public float Temperature
    {
        get { return this.temperature; }
    }
    */

    public PSMoveConnectionType ConnectionType
    {
        get { return psmove_connection_type(handle); }
    }

    #region private methods

    /// <summary>
    /// Process all the raw data on the Playstation Move controller
    /// </summary>
    private void ProcessData()
    {
        trigger = ((int)psmove_get_trigger(handle)) / 255f;

        int x = 0, y = 0, z = 0;

        psmove_get_accelerometer(handle, ref x, ref y, ref z);

        rawAccel.x = x;
        rawAccel.y = y;
        rawAccel.z = z;


        float ax = 0, ay = 0, az = 0;
        psmove_get_accelerometer_frame(handle, PSMove_Frame.Frame_SecondHalf, ref ax, ref ay, ref az);

        accel.x = ax;
        accel.y = ay;
        accel.z = az;

        psmove_get_gyroscope(handle, ref x, ref y, ref z );

        rawGyro.x = x;
        rawGyro.y = y;
        rawGyro.z = z;


        float gx = 0, gy = 0, gz = 0;
        psmove_get_gyroscope_frame(handle, PSMove_Frame.Frame_SecondHalf, ref gx, ref gy, ref gz);

        gyro.x = gx;
        gyro.y = gy;
        gyro.z = gz;



        psmove_get_magnetometer(handle, ref x, ref y, ref z );

        // TODO: Should these values be converted into a more human-understandable range?
        magnet.x = x;
        magnet.y = y;
        magnet.z = z;

        //Orientation
        float q0 = 0.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
        psmove_get_orientation(handle, ref q0, ref q1, ref q2, ref q3);
        //Quaternion w has to be moved to front (swapped) for Unity
        orientation.w=q0;
        orientation.x=-q1;
        orientation.y=q3;
        orientation.z=q2;

        battery = psmove_get_battery(handle);

        temperature = psmove_get_temperature(handle);

    }
    #endregion


    #region importfunctions

    /* The following functions are bindings to Thomas Perl's C API for the PlayStation Move (http://thp.io/2010/psmove/)
     * See README for more details.
     *
     * NOTE! We have included bindings for the psmove_pair() function, even though we don't use it here
     * See README and Pairing Utility code for more about pairing.
     *
     * TODO: Expose hooks to psmove_get_btaddr() and psmove_set_btadd()
     * These functions are already called by psmove_pair(), so unless you need to do something special, you won't need them.
     */

    [DllImport("libpsmoveapi")]
    private static extern int psmove_count_connected();

    [DllImport("libpsmoveapi")]
    private static extern IntPtr psmove_connect();

    [DllImport("libpsmoveapi")]
    private static extern IntPtr psmove_connect_by_id(int id);

    [DllImport("libpsmoveapi")]
    private static extern int psmove_pair(IntPtr move);

    [DllImport("libpsmoveapi")]
    private static extern PSMoveConnectionType psmove_connection_type(IntPtr move);

    [DllImport("libpsmoveapi")]
    private static extern int psmove_has_calibration(IntPtr move);

    [DllImport("libpsmoveapi")]
    private static extern void psmove_set_leds(IntPtr move, char r, char g, char b);

    [DllImport("libpsmoveapi")]
    private static extern int psmove_update_leds(IntPtr move);

    [DllImport("libpsmoveapi")]
    private static extern void psmove_set_rumble(IntPtr move, char rumble);

    [DllImport("libpsmoveapi")]
    private static extern uint psmove_poll(IntPtr move);

    [DllImport("libpsmoveapi")]
    private static extern uint psmove_get_buttons(IntPtr move);

    [DllImport("libpsmoveapi")]
    private static extern uint psmove_get_button_events(IntPtr move, ref uint pressed, ref uint released);

    [DllImport("libpsmoveapi")]
    private static extern char psmove_get_trigger(IntPtr move);

    [DllImport("libpsmoveapi")]
    private static extern float psmove_get_temperature(IntPtr move);

    [DllImport("libpsmoveapi")]
    private static extern PSMove_Battery_Level psmove_get_battery(IntPtr move);

    [DllImport("libpsmoveapi")]
    private static extern void psmove_get_accelerometer(IntPtr move, ref int ax, ref int ay, ref int az);

    [DllImport("libpsmoveapi")]
    private static extern void psmove_get_accelerometer_frame(IntPtr move,PSMove_Frame frame, ref float ax, ref float ay, ref float az);

    [DllImport("libpsmoveapi")]
    private static extern void psmove_get_gyroscope(IntPtr move, ref int gx, ref int gy, ref int gz);

    [DllImport("libpsmoveapi")]
    private static extern void psmove_get_gyroscope_frame(IntPtr move,PSMove_Frame frame, ref float gx, ref float gy, ref float gz);

    [DllImport("libpsmoveapi")]
    private static extern void psmove_get_magnetometer(IntPtr move, ref int mx, ref int my, ref int mz);

    [DllImport("libpsmoveapi")]
    private static extern void psmove_disconnect(IntPtr move);

    [DllImport("libpsmoveapi")]
    private static extern string psmove_get_serial(IntPtr move);

    //Orientation
    [DllImport("libpsmoveapi")]
    private static extern int psmove_has_orientation(IntPtr move);

    [DllImport("libpsmoveapi")]
    private static extern void psmove_enable_orientation(IntPtr move, int enabled);

    [DllImport("libpsmoveapi")]
    private static extern void psmove_reset_orientation(IntPtr move);

    [DllImport("libpsmoveapi")]
    private static extern void psmove_get_orientation(IntPtr move, ref float q0, ref float q1, ref float q2, ref float q3);

    #endregion
}