 /**
 * UniMove API - A Unity plugin for the PlayStation Move motion controller
 * Copyright (C) 2011, Copenhagen Game Collective (http://www.cphgc.org)
 * 					   Patrick Jarnfelt
 * 					   Douglas Wilson (http://www.doougle.net)
 * 
 * Version 0.1.0 (Beta)
 * 2011-07-19
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

using UnityEngine;
using System;
using System.Collections.Generic;

public class UniMoveTest : MonoBehaviour 
{
	// We save a list of Move controllers.
	List<UniMoveController> moves = new List<UniMoveController>();
	
	void Start() 
	{
		/* NOTE! We recommend that you limit the maximum frequency between frames.
		 * This is because the controllers use Update() and not FixedUpdate(),
		 * and yet need to update often enough to respond sufficiently fast.
		 * Unity advises to keep this value "between 1/10th and 1/3th of a second."
		 * However, even 100 milliseconds could seem slightly sluggish, so you
		 * might want to experiment w/ reducing this value even more.
		 * Obviously, this should only be relevant in case your framerare is starting
		 * to lag. Most of the time, Update() should be called very regularly.
		 */
		Time.maximumDeltaTime = 0.1f;
		
		int count = UniMoveController.GetNumConnected();
		
		// Iterate through all connections (USB and Bluetooth)
		for (int i = 0; i < count; i++) 
		{
			UniMoveController move = gameObject.AddComponent<UniMoveController>();	// It's a MonoBehaviour, so we can't just call a constructor
			
			// Remember to initialize!
			if (!move.Init(i)) 
			{	
				Destroy(move);	// If it failed to initialize, destroy and continue on
				continue;
			}
					
			// This example program only uses Bluetooth-connected controllers
			PSMoveConnectionType conn = move.ConnectionType;
			if (conn == PSMoveConnectionType.Unknown || conn == PSMoveConnectionType.USB) 
			{
				Destroy(move);
			}
			else 
			{
				moves.Add(move);
				
				// Add our event listeners to catch when a button is pressed and released
				move.OnPSMoveButtonPressed += HandleButtonPressed;
				move.OnPSMoveButtonReleased += HandleButtonReleased;
				move.OnControllerDisconnected += HandleControllerDisconnected;
				
				// Start all controllers with a white LED
				move.SetLED(Color.white);
			}
		}
	}

	void HandleButtonPressed(object sender, UniMoveButtonEventArgs e)
	{
		if (e.button == PSMoveButton.Circle) Debug.Log("Circle pressed!");
	}
	
	void HandleButtonReleased(object sender, UniMoveButtonEventArgs e)
	{
		if (e.button == PSMoveButton.Circle) Debug.Log("Circle released!");
	}
	
	void HandleControllerDisconnected(object sender, EventArgs e)
	{
		// We'd probably want to remove/destroy the controller here
		Debug.Log("Controller disconnected!");
	}
	
	void Update() 
	{
		
		foreach (UniMoveController move in moves) 
		{
			// Instead of this somewhat kludge-y check, we'd probably want to remove/destroy
			// the now-defunct controller in the disconnected event handler above.
			if (move.Disconnected) continue;
			
			// Change the colors of the LEDs based on which button has just been pressed:
			if (move.IsButtonDown(PSMoveButton.Circle)) move.SetLED(Color.cyan);
			else if(move.IsButtonDown(PSMoveButton.Cross)) move.SetLED(Color.red);
			else if(move.IsButtonDown(PSMoveButton.Square)) move.SetLED(Color.yellow);
			else if(move.IsButtonDown(PSMoveButton.Triangle)) move.SetLED(Color.magenta);
			else if(move.IsButtonDown(PSMoveButton.Move)) move.SetLED(Color.black);

			// Set the rumble based on how much the trigger is down
			move.SetRumble(move.Trigger);
		}
	}
	
	void OnGUI() 
	{
        string display = "";
        
		if (moves.Count > 0) 
		{
            for (int i = 0; i < moves.Count; i++) 
			{
                display += string.Format("PS Move {0}: x:{1:0.000}, y:{2:0.000}, z:{3:0.000}\n", i+1, moves[i].Acceleration.x, moves[i].Acceleration.y, moves[i].Acceleration.z);
            }
        }
        else display = "No Bluetooth-connected controllers found. Make sure one or more are both paired and connected to this computer.";

        GUI.Label(new Rect(10, Screen.height-100, 500, 100), display);
    }
}
