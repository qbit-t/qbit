// QuarkSymbolCustomButton.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

Rectangle
{
    id: symbolRect

    property string symbol;
    property string symbolColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    property int symbolFontSize: 20;

    signal clicked();

    width: 50
    height: 50
    clip: true

    border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.border");
    color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");

    QuarkSymbolButton
    {
        x: -5
        y: -5
        width: parent.width + 10
        height: parent.height + 10
        visible: true

        symbol: symbolRect.symbol
        Material.background: "transparent"
        font.pointSize: symbolFontSize

        onClicked:
        {
            symbolRect.clicked();
        }
    }
}

