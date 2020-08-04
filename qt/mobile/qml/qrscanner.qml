import QtMultimedia 5.5

import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1
import StatusBar 0.1

import "qrc:/backend"
import "qrc:/fonts"
import "qrc:/components"
import "qrc:/models"

import "qrc:/lib/dateFunctions.js" as DateHelper
import "qrc:/lib/numberFunctions.js" as NumberFunctions

import "qrc:/qml"

QuarkPage
{
    id: qrScanner_

    width: parent.width
    height: parent.height

    property string caption: buzzerApp.getLocalization(buzzerClient.locale, "LinkAccount.QRCode")

    Component.onCompleted:
    {
        buzzerApp.lockPortraitOrientation();
        closePageHandler = closePage;
    }

    statusBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.statusBar")
    navigationBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.navigationBar")

    function closePage()
    {
        stopPage();
        destroy(500);
        controller.popPage();
    }

    Timer
    {
        interval:1000
        triggeredOnStart: false

        onTriggered:
        {
            if (camera.cameraStatus == Camera.ActiveStatus)
            {
                viewfinder.grabToImage(function (result)
                {
                    cameraController.decodeQMLImage(result);
                });
            }
            else camera.start();
        }

        running: true
        repeat: true
    }

    Connections
    {
        target: cameraController
        onTagFound:
        {
            dataReady(idScanned);
            closePage();
        }

        onErrorMessage:
        {
        }
    }

    Camera
    {
        id: camera

        imageProcessing.whiteBalanceMode: CameraImageProcessing.WhiteBalanceFlash

        focus.focusMode: Camera.FocusContinuous
        captureMode: Camera.CaptureStillImage

        imageCapture
        {
            onImageCaptured:
            {
                photoPreview.source = preview;  // Show the preview in an Image
            }
        }

        exposure
        {
            //exposureMode: Camera.ExposurePortrait
            exposureMode: Camera.ExposureAuto
            exposureCompensation: expoSlider.value
        }

        digitalZoom: zoomSlider.value
    }

    VideoOutput
    {
        id: viewfinder
        source: camera
        anchors.fill: parent
        focus : visible // to receive focus and capture key events when visible
        autoOrientation: true
        fillMode: VideoOutput.PreserveAspectFit
    }

    Image
    {
        id: photoPreview
    }

    QuarkPanel
    {
        id: infoPanel
        x: 0
        y: 0
        width: parent.width
        height: 60

        QuarkToolButton
        {
            id: backButton
            symbol: Fonts.shevronLeftSym
            Material.background: "transparent"
            visible: true
            labelYOffset: 3
            x: 5
            y: topOffset

            onClicked:
            {
                closePage();
            }
        }

        QuarkLabel
        {
            id: infoText
            x: infoPanel.width / 2 - infoText.width / 2
            y: 15 + topOffset
            color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.textForeground");
            text: caption
            font.pointSize: 14
        }
    }

    Column
    {
        id: exposureControl
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        height: expoSlider.height + expoInLabel.height + expoOutLabel.height

        QuarkSymbolLabel
        {
            id: expoInLabel
            symbol: Fonts.adjustSym
            x: exposureControl.width / 2 - width / 2

            QuarkSymbolLabel
            {
                symbol: Fonts.plusSym
                font.pointSize: 9
                x: expoInLabel.width + 3
                y: 0
            }
        }

        Slider
        {
            id: expoSlider
            to: 3
            from: -3
            orientation: Qt.Vertical
            stepSize: 0.5
            height: qrScanner_.height/3
        }

        QuarkSymbolLabel
        {
            id: expoOutLabel
            symbol: Fonts.adjustSym
            x: exposureControl.width / 2 - width / 2

            QuarkSymbolLabel
            {
                symbol: Fonts.minusSym
                font.pointSize: 9
                x: expoInLabel.width + 3
                y: 0
            }
        }
    }

    Column
    {
        id: zoomControl
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        height: zoomSlider.height + zoomOurLabel.height + zoomInLabel.height

        QuarkSymbolLabel
        {
            id: zoomInLabel
            symbol: Fonts.zoomInSym
            x: zoomControl.width / 2 - width / 2 + 2
        }

        Slider
        {
            id: zoomSlider
            to: camera.maximumOpticalZoom * camera.maximumDigitalZoom
            from: 1
            orientation: Qt.Vertical
            stepSize: 0.5
            height: qrScanner_.height/3
        }

        QuarkSymbolLabel
        {
            id: zoomOurLabel
            symbol: Fonts.zoomOutSym
            x: zoomControl.width / 2 - width / 2 + 2
        }
    }
}
