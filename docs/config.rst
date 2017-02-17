Configuration
=============

Environment Variables
---------------------

Some aspects of the tracker can be configured at runtime using environment variables. The following environment variables are currently recognized by the tracker (with examples for the Linux/Unix bash):

PSMOVE_TRACKER_CAMERA
    If set, this is the camera that will be used when using ``psmove_tracker_new()`` instead of using auto-detection.

    Example: ::

        export PSMOVE_TRACKER_CAMERA=2  # Will use the 3rd camera

PSMOVE_TRACKER_FILENAME
    If set, this will use a video file to playback instead of capturing from a camera. Any camera settings are ignored.

    Example: ::

         export PSMOVE_TRACKER_FILENAME=demo.avi # Will play demo.avi

PSMOVE_TRACKER_ROI_SIZE
    If set, this controls the size of the biggest (initial) ROI that will be used to track the controller. Bigger means slower in general, but recovery from tracking loss might be faster.

    Example: ::

        export PSMOVE_TRACKER_ROI_SIZE=200

PSMOVE_TRACKER_WIDTH, PSMOVE_TRACKER_HEIGHT
    If set, these variables control the desired size of the camera picture.

    Example: ::

        export PSMOVE_TRACKER_WIDTH=1280
        export PSMOVE_TRACKER_HEIGHT=720


