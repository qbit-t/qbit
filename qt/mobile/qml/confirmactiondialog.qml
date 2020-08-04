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
    id: confirmActionDialog_
    modal: true
    focus: true
    title: buzzerApp.getLocalization(buzzerClient.locale, "Confirm.action")
    x: (parent.width - width) / 2
    y: window.height - contentHeight - bottom - 150 // header
    width: Math.min(parent.width, parent.height) - 50
    contentHeight: 420

    signal pinConfirm(var result);
    signal pinConfirmWithCode(var result, var pin);
    signal fingertipConfirm();

    property string label;
    property int bottom: 0;
    property bool fingertip: true;

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accentUltra");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

    Connections
    {
        target: Qt.application
        onStateChanged:
        {
            if(Qt.application.state === 4 /* active */)
            {
                close();
            }
        }
    }

    onClosed:
    {
        buzzerApp.stopFingerprintAuth();
        buzzerApp.fingertipAuthSuccessed.disconnect(fingertipAuthSuccessed);
        buzzerApp.fingertipAuthFailed.disconnect(fingertipAuthFailed);
    }

    function show()
    {
        buzzerApp.fingertipAuthSuccessed.connect(fingertipAuthSuccessed);
        buzzerApp.fingertipAuthFailed.connect(fingertipAuthFailed);
        buzzerApp.startFingerprintAuth();
        fingerprintLabel.init();
        animationTimer.start();
        open();
    }

    function showForPin()
    {
        fingertip = false;
        open();
    }

    function fingertipAuthSuccessed(key)
    {
        fingerprintLabel.setSuccessed();
        fingertipConfirm();
    }

    function fingertipAuthFailed()
    {
        fingerprintLabel.setFailed();
        buzzerApp.startFingerprintAuth();
    }

    QuarkTextField
    {
        id: pinField
        readOnly: true
        width: grid.width
        horizontalAlignment: TextInput.AlignHCenter
        font.pointSize: 18

        property string hiddenText: ""
        property bool hideText: true

        onHideTextChanged:
        {
            updateTextField();
        }

        onHiddenTextChanged:
        {
            updateTextField();
        }

        function updateTextField()
        {
            var lHiddenSymbol = "*";
            if (!hideText) text = hiddenText;
            else text = lHiddenSymbol.repeat(hiddenText.length);
        }

        x: parent.width / 2 - pinField.width / 2
        y: 5

        onTextChanged:
        {
            keyPad9.enabled = keyPad8.enabled = keyPad7.enabled = keyPad6.enabled =
            keyPad5.enabled = keyPad4.enabled = keyPad3.enabled = keyPad2.enabled =
            keyPad1.enabled = keyPad0.enabled = pinField.hiddenText.length < 6;
        }
    }

    QuarkToolButton
    {
        id: eyeButton
        //visible: true
        Material.background: "transparent"
        x: pinField.x + pinField.width + 10
        y: pinField.y + pinField.height / 2 - height / 2
        visible: !(pinField.x + pinField.width + 30 + width > confirmActionDialog_.width)

        symbol: pinField.hideText ? Fonts.eyeSym : Fonts.eyeSlashSym

        onClicked:
        {
            pinField.hideText = !pinField.hideText;
        }
    }

    Grid
    {
        id: grid
        columns: 3
        columnSpacing: 16
        rowSpacing: 16

        x: parent.width / 2 - grid.width / 2
        y: pinField.y + pinField.height + 10

        QuarkButton { id: keyPad1; text: "1"; onClicked: pinField.hiddenText += "1"; font.pointSize: 16; }
        QuarkButton { id: keyPad2; text: "2"; onClicked: pinField.hiddenText += "2"; font.pointSize: 16; }
        QuarkButton { id: keyPad3; text: "3"; onClicked: pinField.hiddenText += "3"; font.pointSize: 16; }
        QuarkButton { id: keyPad4; text: "4"; onClicked: pinField.hiddenText += "4"; font.pointSize: 16; }
        QuarkButton { id: keyPad5; text: "5"; onClicked: pinField.hiddenText += "5"; font.pointSize: 16; }
        QuarkButton { id: keyPad6; text: "6"; onClicked: pinField.hiddenText += "6"; font.pointSize: 16; }
        QuarkButton { id: keyPad7; text: "7"; onClicked: pinField.hiddenText += "7"; font.pointSize: 16; }
        QuarkButton { id: keyPad8; text: "8"; onClicked: pinField.hiddenText += "8"; font.pointSize: 16; }
        QuarkButton { id: keyPad9; text: "9"; onClicked: pinField.hiddenText += "9"; font.pointSize: 16; }
        QuarkButton { Material.background: "transparent"; }
        QuarkButton { id: keyPad0; text: "0"; onClicked: pinField.hiddenText += "0"; font.pointSize: 16; }
        QuarkSymbolButton { symbol: Fonts.backspaceSym; onClicked: pinField.hiddenText = pinField.hiddenText.slice(0, -1); }
    }

    QuarkButton
    {
        id: submitButton
        text: buzzerApp.getLocalization(buzzerClient.locale, "Button.Confirm")
        enabled: pinField.hiddenText.length == 6

        x: parent.width / 2 - submitButton.width / 2
        y: grid.y + grid.height + 10

        width: grid.width

        onClicked:
        {
            var lResult = buzzerClient.verify(pinField.hiddenText);
            pinConfirm(lResult);
            pinConfirmWithCode(lResult, pinField.hiddenText);

            pinField.hiddenText = "";
        }
    }

    QuarkSymbolLabel
    {
        property int maxFontSize: 30
        property int minFontSize: 25
        property bool expanded: true
        property bool failed: false
        property int failedAnimation: 10
        property int failedAnimationStep: 0

        id: fingerprintLabel
        symbol: Fonts.fingerPrintSym
        font.pointSize: maxFontSize
        x: parent.width / 2 - width / 2
        y: submitButton.y + submitButton.height + 10
        visible: buzzerApp.isFingerprintAuthAvailable() && fingertip

        function setFailed()
        {
            failed = true;
        }

        function setSuccessed()
        {
            animationTimer.stop();
            color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Fingertip.success");
        }

        function init()
        {
            fingerprintLabel.opacity = 1.0;
            color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
        }

        onFailedChanged:
        {
            if (failed)
            {
                failedAnimationStep = 0;
                color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Fingertip.error");
            }
            else
            {
                fingerprintLabel.opacity = 1.0;
                color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
            }
        }

        Timer
        {
            id: animationTimer
            interval: 50
            repeat: true
            running: buzzerApp.isFingerprintAuthAvailable()

            onTriggered:
            {
                if (fingerprintLabel.failed)
                {
                    if (fingerprintLabel.failedAnimationStep < fingerprintLabel.failedAnimation)
                    {
                        fingerprintLabel.failedAnimationStep++;
                    }
                    else
                    {
                        fingerprintLabel.failed = false;
                    }
                }

                if (!fingerprintLabel.expanded)
                {
                    fingerprintLabel.opacity += 0.02;
                    if (fingerprintLabel.opacity >= 1.0) fingerprintLabel.expanded = true;
                }
                else if (fingerprintLabel.expanded)
                {
                    fingerprintLabel.opacity -= 0.02;
                    if (fingerprintLabel.opacity <= 0.4) fingerprintLabel.expanded = false;
                }
            }
        }
    }
}
