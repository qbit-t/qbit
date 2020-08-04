// QuarkSymbolTap.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

Rectangle
{
    id: tapRect

    width: parent.height
    height: parent.height

    border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.border");
    color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");

    property string labelColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");

    property string symbol;
    //property bool clicked: false;

    signal clicked()

    Label
    {
        id: tapLabel
        anchors.fill: parent
        leftPadding: 0
        x: 0
        //renderType: Text.NativeRendering
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: symbol
        color: labelColor

        font.family: Fonts.icons
        font.weight: Font.Normal
        font.pointSize: parent.height - 10 //?

        MouseArea
        {
            id: tapArea

            x: 0
            y: 0
            width: parent.width
            height: parent.height

            onClicked:
            {
                tapRect.clicked();
            }
        }
    }
}
