import QtQuick 2.0
import QtMultimedia 5.8

Rectangle
{
    id: innerCameraContainer
    color: "transparent"
    anchors.fill: parent

    Camera
    {
        id: cameraDevice

        imageCapture
        {
            onImageSaved:
            {
                cameraDevice.stop();

                innerCameraContainer.parentContaner.applyPicture(path);
            }
        }
        focus
        {
            focusMode: Camera.FocusAuto
        }

        Component.onCompleted:
        {
            cameraDevice.stop();
        }
    }

    VideoOutput
    {
        id: videoOutput
        source: cameraDevice
        focus : true // to receive focus and capture key events when visible
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        autoOrientation: true

        // autofocus area positions
        /*
        Repeater
        {
            model: cameraDevice.focus.focusZones

            Rectangle
            {
                border
                {
                    width: 2
                    color: status == Camera.FocusAreaFocused ? "green" : "white"
                }
                color: "transparent"

                // Map from the relative, normalized frame coordinates
                property variant mappedRect: videoOutput.mapNormalizedRectToItem(area);

                x: mappedRect.x
                y: mappedRect.y
                width: mappedRect.width
                height: mappedRect.height
            }
        }
        */
    }

    property variant parentContaner;

    function setup(container)
    {
        innerCameraContainer.parentContaner = container;
    }

    function start()
    {
        videoOutput.visible = true;
        cameraDevice.start();
    }

    function stop()
    {
        videoOutput.visible = false;
        cameraDevice.stop();
    }

    function captureToLocation(location)
    {
        videoOutput.visible = false;
		if (location !== "") cameraDevice.imageCapture.captureToLocation(location);
		else cameraDevice.imageCapture.capture();
    }

    function cameraState()
    {
        return cameraDevice.cameraState;
    }

	function rotate() {
		if (cameraDevice.position === Camera.BackFace) {
			cameraDevice.position = Camera.FrontFace;
		} else {
			cameraDevice.position = Camera.BackFace;
		}
	}
}
