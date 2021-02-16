// QuarkSymbolComboBox.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

Rectangle
{
    id: symbolComboBox

    height: 50

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
    border.color: "transparent"
    color: "transparent"

    property string symbol;
    property string cosymbol;
    property string text;
    property string placeholderText;
    property int symbolLeftPadding: 0;
    property int leftPadding: 0;
    property bool clearEnabled: false;
    property bool readOnly: false;

    property alias comboBox: innerComboBox

    signal activated(var index);

    property string frameColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    property string frameBox: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background");
    property string frameBordercolor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.border");
    property string comboBoxMaterialBackground: "transparent";

    onReadOnlyChanged:
    {
        comboBox.readOnly = readOnly;
    }

    function getSelectedItem()
    {
        return comboBox.model.get(comboBox.currentIndex);
    }

    function reset()
    {
        comboBox.currentIndex = -1;
    }

    function setItem(key)
    {
        comboBox.setItem(key);
    }

    Rectangle
    {
        id: symbolRect

        width: 50
        height: 50

        border.color: symbolComboBox.frameBordercolor
        color: symbolComboBox.frameColor

        Label
        {
            id: symbolLabel
            anchors.fill: parent
            leftPadding: symbolLeftPadding
            x: symbolLeftPadding
            //renderType: Text.NativeRendering
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: symbolComboBox.symbol
            color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");

            font.family: Fonts.icons
            font.weight: Font.Normal
            font.pointSize: 20
        }

        Label
        {
            id: cosymbolLabel
            //anchors.fill: parent
            //leftPadding: symbolLeftPadding
            x: symbolLabel.x + symbolLabel.width - 10
            y: symbolLabel.y + 6
            //renderType: Text.NativeRendering
            //horizontalAlignment: Text.AlignHCenter
            //verticalAlignment: Text.AlignVCenter
            text: symbolComboBox.cosymbol
            color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");

            font.family: Fonts.icons
            font.weight: Font.Normal
            font.pointSize: 10
        }

        MouseArea
        {
            x: 0
            y: 0

            width: parent.width
            height: parent.height

            enabled: symbolComboBox.clearEnabled

            onClicked:
            {
                if (!readOnly)
                {
                    innerComboBox.currentIndex = -1;
                }
            }
        }
    }

    Rectangle
    {
        id: infoRect

        x: symbolRect.width - 1
        height: 50
        width: symbolComboBox.width - symbolRect.width + 1

        border.color: symbolComboBox.frameBordercolor
        color: symbolComboBox.frameBox

        QuarkComboBox
        {
            id: innerComboBox
            width: infoRect.width - 10
            x: 5

            leftPadding: symbolComboBox.leftPadding

            Material.background: symbolComboBox.comboBoxMaterialBackground

            onActivated:
            {
                symbolComboBox.activated(index);
            }

            indicator: Canvas
            {
                id: canvas
                x: innerComboBox.width - width - innerComboBox.rightPadding + 5
                y: innerComboBox.topPadding + (innerComboBox.availableHeight - height) / 2
                width: 10
                height: 6
                contextType: "2d"

                Connections
                {
                    target: innerComboBox
                    onPressedChanged: canvas.requestPaint()
                }

                onPaint:
                {
                    context.reset();
                    context.moveTo(0, 0);
                    context.lineTo(width, 0);
                    context.lineTo(width / 2, height);
                    context.closePath();
                    context.fillStyle = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
                    context.fill();
                }
            }
        }
    }
}

