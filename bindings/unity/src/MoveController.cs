using UnityEngine;
using System.Collections;

public class MoveController : MonoBehaviour {

	Renderer sphereRenderer;
	private Light sphereLight;

	void Awake () {
		Transform[] trs = GetComponentsInChildren<Transform>();
		foreach (Transform tr in trs) {
			if(tr.name == "Sphere") sphereRenderer = tr.GetComponent<Renderer>();
		}
		sphereLight = GetComponentInChildren<Light>();
	}

	public void SetLED(Color color)
	{
		sphereLight.enabled = color != Color.black;
		sphereRenderer.material.color = color == Color.black ? Color.white * 0.2f : color;
		sphereLight.color = color;
		sphereLight.intensity =  color.grayscale * 4;
	}
}
