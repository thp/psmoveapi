 /**
 * Pairing Utility - A Bluetooth Pairing Utility for the PlayStation Move controller
 * Copyright (C) 2011, Copenhagen Game Collective (http://www.cphgc.org)
 * 					   Douglas Wilson (http://www.doougle.net)
 * 
 * Version 0.3 (Beta)
 * 2011-07-18
 * 
 * Email us at: code@cphgc.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 **/

using System;
using System.Collections;
using System.Runtime.InteropServices;
using UnityEngine;


public enum PSMoveConnectionType 
{
    Bluetooth,
    USB,
    Unknown,
};


public class PairingUtility : MonoBehaviour 
{	
	/**********************************************/
	/******* Constants & Instance Variables *******/
	/**********************************************/
	
	public static bool DEBUG_MODE = true;
	
	public static int MAX_NUM_CONTROLLERS = 6;
	
	private int numControllers = 0;
	private int numUSBConnections = 0;
	
	private int nextScreen = 0;
	
	private Color background = new Color(0.78f, 0.682f, 0.831f);
	
	private int textAreaWidth;
	private int textAreaHeight;
	
	public GUIStyle titleStyle;
	public GUIStyle textStyle;
	private String title = "";
	private String text = "";
	private bool displayTitle = true;
	
	private bool errorEncountered = false;
	
	public AudioClip SOUND_REGISTRATION;
	public static AudioSource efxSource;
	
	
	/**********************************************/
	
	
	/***********************/
	/******* Methods *******/
	/***********************/
	
	void Awake()
	{		
		title = "PLAYSTATION  MOVE\nPAIRING  UTILITY";
				
		text = 	"By Douglas Wilson &\n" + "the Copenhagen Game Collective\n" +
				"(www.cphgc.org)\n\n" +
				"Press ENTER to continue";
	}
	
	
	void Start()
	{
		Camera.mainCamera.backgroundColor = background;
		GUI.backgroundColor = background;
		
		textAreaWidth = (int) (Screen.width * .8);
		textAreaHeight = (int) (Screen.height * .8);
		
		// Sound removed from the open-source version
		// efxSource = gameObject.AddComponent<AudioSource>();
	}
	
	
	void OnGUI() 
	{
		GUILayout.BeginArea(new Rect((Screen.width - textAreaWidth) / 2, (Screen.height - textAreaHeight) / 2, textAreaWidth, textAreaHeight));
		if (displayTitle) GUILayout.Label(title, titleStyle);
		GUILayout.Label(text, textStyle);	
		GUILayout.EndArea();
	}
	
	
	void Update()
	{
		if (Input.GetKey(KeyCode.Escape)) Exit();
		
		if (Input.GetKeyDown(KeyCode.Return) || Input.GetKeyDown(KeyCode.KeypadEnter))
		{
			switch (nextScreen)
			{
				case 0:		
					displayTitle = false;
		
					text =	"Each Move controller must be \"paired\" to this computer via Bluetooth.\n\n" +
							"You'll only ever need to do this once for each controller - or at least until you pair it to another computer or PS3.\n\n" +
							"Press ENTER to continue";
										
					nextScreen++;
					break;
				
				
				case 1:
					text =	"Connect the Move controller(s) to this computer by mini-USB wire.\n\n" +
							"Press ENTER when you're ready";
				
					nextScreen++;
					break;
				
				
				case 2:
					PairControllers(); 	// This is where all the pairing magic happens
					
					if (numUSBConnections <= 0)
					{
						text =	"No USB-connected Move controllers detected!\n\n" +
								"First connect the Move controller(s) to this computer.\n\n" + 
								"Press ENTER to try again.\n\n" +
								"Or press ESC to quit";
						
						nextScreen--;
						break;
					}
					
					else if (numUSBConnections > 0)
					{
						// Sound removed from the public open-source version
						// efxSource.PlayOneShot(SOUND_REGISTRATION);
					
						text =	"Just paired " + numUSBConnections + " PS Move controller(s)!\n\n" +
								"Press the small circular PS button to connect it via Bluetooth.\n\n" +
								"Wait a second or two...\n\n" +
								"Then press ENTER to continue";
										
						nextScreen++;
						break;
					}
				
					// Should never get here...
					Exit();
					break;
						
				
				case 3:
					text =	"The controller(s) should now be visible in your Bluetooth Preferences.\n\n" +	
							"You can now unplug it.\n\n" +
							"Press ENTER to continue";
				
					nextScreen++;
					break;
				
				
				case 4:
					text =	"Remember, whenever you want to start an application that uses this controller(s), make sure to first press " +
							"the PS button to wirelessly connect the controller via Bluetooth.\n\n" + 
							"Press ENTER to continue";
				
					nextScreen++;
					break;
				
				
				case 5:
					text =	"Press ENTER if you'd like to pair additional controllers.\n\n" +
							"Or press ESC to quit";	
					
					nextScreen = 1;
					break;
								
				
				default: break;
			}
		}
	}
	
	
	private void PairControllers()
	{	
		numUSBConnections = 0;
		
		numControllers = psmove_count_connected();	
		if (numControllers >= MAX_NUM_CONTROLLERS) numControllers = MAX_NUM_CONTROLLERS;
		
		for (int i = 0; i < numControllers; i++) 
		{
			PairOneController(i);
			if (errorEncountered) break;
		}
	}
	
	
	private void PairOneController(int id)
	{
		IntPtr controller = psmove_connect_by_id(id);		
		if (controller == IntPtr.Zero)
		{
			ErrorText("Error! Controller " + id + " did not connect.");
			return;
		}
		
		PSMoveConnectionType ctype = psmove_connection_type(controller);	
	    if (ctype == PSMoveConnectionType.USB)	// This avoids trying to pair any Bluetooth-connected controller objects 
		{
			numUSBConnections++;
	        if (psmove_pair(controller) == 0) 
			{
				ErrorText("Error! Controller " + id + " failed to pair.");
				return;
			}
		}
	}
		
		
	public void ErrorText(String msg) 
	{
		title = "";
		text = msg + "\n\n Press ESC to quit";
		nextScreen = -1;
		errorEncountered = true;
	}
	
	
	public static void Exit()
	{
		Application.Quit();
	}
		
	
	/* The following functions are bindings to Thomas Perl's C API for the PlayStation Move (http://thp.io/2010/psmove/)
	 * I don't use most of these functions in this application. But here they are just in case...
	 * See README for more information
	 * 
	 * TODO: Expose hooks to psmove_get_btaddr() and psmove_set_btadd()
	 * These functions are already called by psmove_pair(), so unless you need to do something special, you won't need them.
	 * 
	 * It's not clear to us whether we can just pass in a char array (depends on how memory is laid out in C#?)
	 * When we were trying to get this to work in Windows, we compiled in our own versions of the two functions
	 * that took six separate char arguments. When we have time for more testing, we may add these back in.
	 */
	
	#region importfunctions
	
	[DllImport("PSMove")]
	private static extern int psmove_count_connected();
	
	[DllImport("PSMove")]
	private static extern IntPtr psmove_connect();
	
	[DllImport("PSMove")]
	private static extern IntPtr psmove_connect_by_id(int id);
	
	[DllImport("PSMove")]
	private static extern int psmove_pair(IntPtr move);
	
	[DllImport("PSMove")]
	private static extern PSMoveConnectionType psmove_connection_type(IntPtr move);
	
	[DllImport("PSMove")]
	private static extern void psmove_set_leds(IntPtr move, char r, char g, char b);
	
	[DllImport("PSMove")]
	private static extern int psmove_update_leds(IntPtr move); 
	
	[DllImport("PSMove")]
	private static extern void psmove_set_rumble(IntPtr move, char rumble);
	
	[DllImport("PSMove")]
	private static extern uint psmove_poll(IntPtr move);
		
	[DllImport("PSMove")]
	private static extern uint psmove_get_buttons(IntPtr move);
		
	[DllImport("PSMove")]
	private static extern char psmove_get_trigger(IntPtr move);
				
	[DllImport("PSMove")]
	private static extern void psmove_get_accelerometer(IntPtr move, ref int ax, ref int ay, ref int az);
	
	[DllImport("PSMove")]
	private static extern void psmove_get_gyroscope(IntPtr move, ref int gx, ref int gy, ref int gz);
	
	[DllImport("PSMove")]
	private static extern void psmove_disconnect(IntPtr move);
	
	#endregion
}

