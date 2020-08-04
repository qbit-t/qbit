import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1

import "qrc:/fonts"
import "qrc:/components"

Dialog
{
    id: errorDialog
    modal: true
    focus: true
    title: "Error"
    x: (parent.width - width) / 2
    y: parent.height / 6
    width: Math.min(parent.width, parent.height) / 3 * 2
    contentHeight: errorColumn.height
    standardButtons: Dialog.Ok

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

    property var callback: null;

    onAccepted:
    {
        if (callback) callback();
    }

    function show(error, accepted)
    {
        var lMessage = error.message + "\n(Code = " + error.code + ")";
        errorText.text = lMessage;
        callback = accepted;
        open();
    }

    function showText(error)
    {
        errorText.text = error;
        open();
    }

    Column
    {
        id: errorColumn
        spacing: 20

        Label
        {
            id: errorText
            width: errorDialog.availableWidth
            wrapMode: Label.Wrap
            font.pixelSize: 12
        }
    }
}
