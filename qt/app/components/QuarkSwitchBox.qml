// QuarkSwitchBox.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

Rectangle
{
    id: switchBox
    color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background")
    border.color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.border");

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

    property bool checked: false;
    property string text: "";

    height: 50

    QuarkSwitch
    {
        id: switchItem
        x: 5
        y: switchBox.height / 2 - switchBox.height / 2
        checked: switchBox.checked

        onCheckedChanged:
        {
            switchBox.checked = checked;
        }
    }

    QuarkLabel
    {
        id: label
        x: switchItem.x + switchItem.width + 5
        y: switchItem.y + switchItem.height / 2 - label.height / 2
        text: switchBox.text
        width: switchBox.width - switchItem.width - 10
        elide: Text.ElideRight
    }
}

