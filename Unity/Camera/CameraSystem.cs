using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CameraSystem : MonoBehaviour
{
    /*
     * Install package Cinemachine and create a virtual camera.
     * Create an empty object (Camera System) and make the virtual camera FOLLOW and LOOK AT this object.
     * Attach this script to that object
     * 
     * For best results zero out the Damping in the BODY part of the virtual camera and zero out the 
     * horizontal and vertical damping in the AIM part.
     * 
     * On the virtual camera I put Y as 30 and Z as -26
     */

    /* Keyboard scrolling uses the WASD keys to move the camera around
     * the playing area.  Keyboard Rotation using the Q and E key.
     */
    [SerializeField] private bool UseKeyboardScrolling = true;
    [SerializeField] private bool UseKeyboardRotating = true;

    /* 
     * Edge Scrolling is as the mouse approaches the edge of the screen that will
     * trigger the camera to start moving in that direction.  Default size is 20.
     */
    [SerializeField] private bool UseEgdeScrolling = false;
    private int edgeScrollSize = 20;

    /*
     * Drag Pan Scrolling is using the mouse to move the camera. 
     * Hold the right mouse button and mouse the cursor to move the camera.
     */
    [SerializeField] private bool UseDragPanScrolling = true;
    private bool dragPanMoveActive = false;
    private Vector2 lastMousePosition;
    private float dragPanSpeed = 0.01f;


    private float moveSpeed = 50f;
    private float rotateSpeed = 100f;



    // Update is called once per frame
    private void Update()
    {
        if (UseKeyboardScrolling) HandleCameraKeyBoardMovement();
        if (UseEgdeScrolling) HandleCameraMovementEdgeScrolling();
        if (UseDragPanScrolling) HandleCameraMoveDragPan();
        if (UseKeyboardRotating) HandleCameraKeyboardRotation();
    }

    private void HandleCameraMovementEdgeScrolling()
    {
        Vector3 InputDir = new Vector3(0, 0, 0);

        if (Input.mousePosition.x < edgeScrollSize)
        {
            InputDir.x = -1f;
        }
        if (Input.mousePosition.y < edgeScrollSize)
        {
            InputDir.z = -1f;
        }
        if (Input.mousePosition.x > Screen.width - edgeScrollSize)
        {
            InputDir.x = +1f;
        }
        if (Input.mousePosition.y > Screen.height - edgeScrollSize)
        {
            InputDir.z = +1f;
        }

        Vector3 moveDir = transform.forward * InputDir.z + transform.right * InputDir.x;
        transform.position += moveDir * moveSpeed * Time.deltaTime;
    }

    private void HandleCameraMoveDragPan()
    {
        Vector3 InputDir = new Vector3(0, 0, 0);

        if (Input.GetMouseButtonDown(1))
        {
            dragPanMoveActive = true;
            lastMousePosition = Input.mousePosition;
        }
        if (Input.GetMouseButtonUp(1))
        {
            dragPanMoveActive = false;
        }
        if (dragPanMoveActive)
        {
            Vector2 mouseMovementDelta = (Vector2)Input.mousePosition - lastMousePosition;

            InputDir.x = mouseMovementDelta.x * dragPanSpeed;
            InputDir.z = mouseMovementDelta.y * dragPanSpeed;

            lastMousePosition = Input.mousePosition;
        }

        Vector3 moveDir = transform.forward * InputDir.z + transform.right * InputDir.x;
        transform.position += moveDir * moveSpeed * Time.deltaTime;
    }

    private void HandleCameraKeyBoardMovement()
    {
        Vector3 InputDir = new Vector3(0, 0, 0);

        if (Input.GetKey(KeyCode.W)) InputDir.z = +1f;
        if (Input.GetKey(KeyCode.S)) InputDir.z = -1f;
        if (Input.GetKey(KeyCode.A)) InputDir.x = -1f;
        if (Input.GetKey(KeyCode.D)) InputDir.x = +1f;

        Vector3 moveDir = transform.forward * InputDir.z + transform.right * InputDir.x;
        transform.position += moveDir * moveSpeed * Time.deltaTime;
    }

    private void HandleCameraKeyboardRotation()
    {
        float rotateDir = 0f;
        if (Input.GetKey(KeyCode.Q)) rotateDir = +1f;
        if (Input.GetKey(KeyCode.E)) rotateDir = -1f;

        transform.eulerAngles += new Vector3(0, rotateDir * rotateSpeed * Time.deltaTime, 0);
    }
}
