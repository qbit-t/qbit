// QuarkProgressButton.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1

import QtQuick 2.0
import QtQml 2.2

import "qrc:/fonts"

QuarkRoundProgress
{
    id: progressBar
    size: 70
    colorCircle: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
    colorBackground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
    arcBegin: 0
    arcEnd: 0
    lineWidth: 3

    property string symbol;
    signal pressed();
    property int fontPointSize: 20

    function cancelAction()
    {
        tapArea.stop();
    }

    QuarkSymbolLabel
    {
        id: progressLabel
        symbol: progressBar.symbol
        font.pointSize: fontPointSize
        x: parent.width / 2 - width / 2
        y: parent.height / 2 - height / 2
    }

    MouseArea
    {
        id: tapArea

        width: parent.width + 20
        height: parent.height + 20
        hoverEnabled: true

        onPressed:
        {
            progressBar.arcBegin = 0;
            progressBar.arcEnd = 0;
            holdTimer.start();
        }

        onExited:
        {
            stop();
        }

        onReleased:
        {
            stop();
        }

        function stop()
        {
            holdTimer.stop();
            progressBar.arcBegin = 0;
            progressBar.arcEnd = 0;
        }

        Timer
        {
            id: holdTimer
            running: false
            repeat: true
            interval: 100

            onTriggered:
            {
                if (!parent.containsMouse) { parent.stop(); return; }

                progressBar.arcEnd += 27;

                if (progressBar.arcEnd >= 360)
                {
                    running = false;
                    progressBar.arcBegin = 0;
                    progressBar.arcEnd = 0;
                    pressed();
                }
            }
        }
    }
}
