using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Cinemachine;

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
    private int _edgeScrollSize = 20;

    /*
     * Drag Pan Scrolling is using the mouse to move the camera. 
     * Hold the right mouse button and mouse the cursor to move the camera.
     */
    [SerializeField] private bool UseDragPanScrolling = true;
    private bool _dragPanMoveActive = false;
    private Vector2 _lastMousePosition;
    private float _dragPanSpeed = 0.01f;

    /* 
     * This is the reference to the virtual camera object.
     * Make sure in the editor to add the camera to this field.
     */
    [SerializeField] private CinemachineVirtualCamera cinemachineVirtualCamera;
    private float _targetFieldOfView = 50f;
    private float _FieldOfViewMax = 50f;
    private float _FieldOfViewMin = 10f;

    /*
     * Zooming is used by the mouse wheel. The default is ON
     */
    [SerializeField] private bool UseMouseZooming = true;
    private float _zoomStep = 5f;
    private float _zoomSpeed = 10f;

    private float _moveSpeed = 50f;
    private float _rotateSpeed = 100f;



    // Update is called once per frame
    private void Update()
    {
        if (UseKeyboardScrolling) _HandleCameraKeyBoardMovement();
        if (UseEgdeScrolling) _HandleCameraMovementEdgeScrolling();
        if (UseDragPanScrolling) _HandleCameraMoveDragPan();
        if (UseKeyboardRotating) _HandleCameraKeyboardRotation();
        if (UseMouseZooming) _HandleCameraZoom();
    }

    private void _HandleCameraMovementEdgeScrolling()
    {
        Vector3 InputDir = new Vector3(0, 0, 0);

        if (Input.mousePosition.x < _edgeScrollSize)
        {
            InputDir.x = -1f;
        }
        if (Input.mousePosition.y < _edgeScrollSize)
        {
            InputDir.z = -1f;
        }
        if (Input.mousePosition.x > Screen.width - _edgeScrollSize)
        {
            InputDir.x = +1f;
        }
        if (Input.mousePosition.y > Screen.height - _edgeScrollSize)
        {
            InputDir.z = +1f;
        }

        Vector3 moveDir = transform.forward * InputDir.z + transform.right * InputDir.x;
        transform.position += moveDir * _moveSpeed * Time.deltaTime;
    }

    private void _HandleCameraMoveDragPan()
    {
        Vector3 InputDir = new Vector3(0, 0, 0);

        if (Input.GetMouseButtonDown(1))
        {
            _dragPanMoveActive = true;
            _lastMousePosition = Input.mousePosition;
        }
        if (Input.GetMouseButtonUp(1))
        {
            _dragPanMoveActive = false;
        }
        if (_dragPanMoveActive)
        {
            Vector2 mouseMovementDelta = (Vector2)Input.mousePosition - _lastMousePosition;

            InputDir.x = mouseMovementDelta.x * _dragPanSpeed;
            InputDir.z = mouseMovementDelta.y * _dragPanSpeed;

            _lastMousePosition = Input.mousePosition;
        }

        Vector3 moveDir = transform.forward * InputDir.z + transform.right * InputDir.x;
        transform.position += moveDir * _moveSpeed * Time.deltaTime;
    }

    private void _HandleCameraKeyBoardMovement()
    {
        Vector3 InputDir = new Vector3(0, 0, 0);

        if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow)) InputDir.z = +1f;
        if (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow)) InputDir.z = -1f;
        if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow)) InputDir.x = -1f;
        if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow)) InputDir.x = +1f;

        Vector3 moveDir = transform.forward * InputDir.z + transform.right * InputDir.x;
        transform.position += moveDir * _moveSpeed * Time.deltaTime;
    }

    private void _HandleCameraKeyboardRotation()
    {
        float rotateDir = 0f;
        if (Input.GetKey(KeyCode.Q)) rotateDir = +1f;
        if (Input.GetKey(KeyCode.E)) rotateDir = -1f;

        transform.eulerAngles += new Vector3(0, rotateDir * _rotateSpeed * Time.deltaTime, 0);
    }

    private void _HandleCameraZoom()
    {
        if (Input.mouseScrollDelta.y > 0)
        {
            _targetFieldOfView -= _zoomStep;
        }
        if (Input.mouseScrollDelta.y < 0)
        {
            _targetFieldOfView += _zoomStep;
        }
        _targetFieldOfView = Mathf.Clamp(_targetFieldOfView, _FieldOfViewMin, _FieldOfViewMax);

        Mathf.Lerp(cinemachineVirtualCamera.m_Lens.FieldOfView, _targetFieldOfView, _zoomSpeed * Time.deltaTime);
        cinemachineVirtualCamera.m_Lens.FieldOfView = _targetFieldOfView;
    }
}
