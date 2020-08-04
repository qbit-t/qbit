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
    id: infoDialog
    modal: true
    focus: true
	title: "Buzzer"
    x: (parent.width - width) / 2
    y: parent.height / 3
    width: Math.min(parent.width, parent.height) - 50
    contentHeight: infoColumn.height
    standardButtons: Dialog.Ok

    property int bottom: 0;

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

    function show(msgs)
    {
        infoColumn.msgs = msgs;
        open();
    }

    Column
    {
        id: infoColumn
        spacing: 10

        property var msgs: []
        property int calculatedHeight: 0
        property var labels: []

        function linkActivated(link)
        {
            if (isValidURL(link))
            {
               Qt.openUrlExternally(link);
            }
        }

        function isValidURL(str)
        {
           var regexp = /(ftp|http|https):\/\/(\w+:{0,1}\w*@)?(\S+)(:[0-9]+)?(\/|\/([\w#!:.?+=&%@!\-\/]))?/
           return regexp.test(str);
        }

        onMsgsChanged:
        {
            var lIdx;
            for (lIdx = 0; lIdx < labels.length; lIdx++)
            {
                labels[lIdx].destroy();
            }

            calculatedHeight = 0;
            labels = [];

            var lAdvance = 0;
            for (lIdx = 0; lIdx < msgs.length; lIdx++)
            {
                var lLabelComponent = Qt.createComponent("qrc:/components/QuarkLabel.qml");
                var lLabel = lLabelComponent.createObject(infoColumn);

                lLabel.visible = true;
                lLabel.font.pixelSize = 14;
                lLabel.text = msgs[lIdx];
                lLabel.wrapMode = Label.Wrap;
                lLabel.width = infoDialog.availableWidth;
                lLabel.linkActivated.connect(linkActivated);
                lAdvance += lLabel.height;
                calculatedHeight += lLabel.height + infoColumn.spacing;
                labels.push(lLabel);
            }

            if (infoDialog.bottom)
            {
                infoDialog.y = infoDialog.parent.height - calculatedHeight - infoDialog.bottom - 150;
            }
        }
    }
}
