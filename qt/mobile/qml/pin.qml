import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import Qt.labs.settings 1.0
import QtQuick.Dialogs 1.1

import StatusBar 0.1

import "qrc:/fonts"
import "qrc:/components"

QuarkPage
{
    id: pin

    property string pinCaption;
    property string pinSubmit;
    property string pinAction;

    property string actionSetup: "setup";
    property string actionUnlock: "unlock";

    property bool finalized_: false;

    Connections
    {
        target: Qt.application
        onStateChanged:
        {
            if(Qt.application.state === 0 /* suspended */ ||
               Qt.application.state === 1 /* hidden */ ||
               Qt.application.state === 2 /* inactive */ )
            {
                console.log("Pin suspending...");
                unsubscribe();
            }
            else if(Qt.application.state === 4 /* active */)
            {
                console.log("Pin is waking up...");
                prepare();
            }
        }
    }

    Component.onCompleted:
    {
        buzzerApp.lockPortraitOrientation();
        prepare();
    }

    statusBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.statusBar")
    navigationBarColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Window.navigationBar")

    function prepare()
    {
        if (!finalized_ && buzzerApp.isFingerprintAccessConfigured() && buzzerApp.isFingerprintAuthAvailable())
        {
            buzzerApp.fingertipAuthSuccessed.connect(fingertipAuthSuccessed);
            buzzerApp.fingertipAuthFailed.connect(fingertipAuthFailed);
            buzzerApp.startFingerprintAuth();
            fingerprintLabel.init();
            animationTimer.start();
        }
    }

    function unsubscribe()
    {
        if (buzzerApp.isFingerprintAccessConfigured())
        {
            buzzerApp.fingertipAuthSuccessed.disconnect(fingertipAuthSuccessed);
            buzzerApp.fingertipAuthFailed.disconnect(fingertipAuthFailed);

            buzzerApp.stopFingerprintAuth();
        }
    }

    function fingertipAuthSuccessed(key)
    {
        fingerprintLabel.setSuccessed();

        var lResult = buzzerClient.open(key);

        if(localNotificator && localNotificator.getDeviceToken() !== "" &&
				buzzerClient.getProperty("Client.deviceId") === "")
        {
            pin.controller.registerDeviceId(localNotificator.getDeviceToken());
        }

        if (!buzzerClient.unlocked() || lResult === 1001 /* incorrect pin */)
        {
            controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Pin.Incorrect"));
            pinField.hiddenText = "";
        }
        else
        {
            if (nextPage === "back")
            {
                unsubscribe();
                finalized_ = true;

                destroy(500);
                controller.popPage();
            }
            else
            {
                var lComponent = Qt.createComponent(nextPage);
                var lPage = lComponent.createObject(pin.controller);
                lPage.controller = pin.controller;

                unsubscribe();
                finalized_ = true;

                pin.controller.addPage(lPage);
            }
        }
    }

    function fingertipAuthFailed()
    {
        fingerprintLabel.setFailed();
        buzzerApp.startFingerprintAuth();
    }

    QuarkPanel
    {
        id: infoPanel
        x: 0
        y: 0
        width: parent.width
        height: 60

        QuarkLabel
        {
            id: infoText
            x: infoPanel.width / 2 - infoText.width / 2
            y: 15 + topOffset
            color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Panel.textForeground");
            text: pinCaption
            font.pointSize: 14
        }
    }

    RemoveAccount
    {
        id: removeAccount
        container: deleteAccountDialog

        x: infoPanel.width - removeAccount.calculatedWidth - 10
        y: parent.height - removeAccount.calculatedHeight - 10

        visible: pinAction !== actionSetup
    }

    YesNoDialog
    {
        id: deleteAccountDialog

        onAccepted:
        {
            buzzerClient.unlink();

            if (!buzzerClient.configured())
            {
                var lComponent = Qt.createComponent("qrc:/qml/setupaccount.qml");
                var lPage = lComponent.createObject(window);
                lPage.controller = pin.controller;
                pin.controller.addPage(lPage);
            }
            else
            {
				controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Client.unlinkError"));
            }
        }

        onRejected:
        {
            deleteAccountDialog.close();
        }
    }

    QuarkTextField
    {
        id: pinField
        readOnly: true
        width: grid.width
        horizontalAlignment: TextInput.AlignHCenter
        font.pointSize: 18

        property string hiddenText: ""
        property bool hideText: pinAction !== actionSetup

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
        y: 80 + topOffset

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
        visible: pinAction !== actionSetup
        Material.background: "transparent"
        x: pinField.x + pinField.width + 10
        y: pinField.y + pinField.height / 2 - height / 2

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
        text: pinSubmit
        enabled: pinField.hiddenText.length == 6

        x: parent.width / 2 - submitButton.width / 2
        y: grid.y + grid.height + 10

        width: grid.width

        onClicked:
        {
            var lResult = 0;

            if (pinAction === actionSetup)
            {
                buzzerClient.updateKeys(pinField.hiddenText);

                if(localNotificator && localNotificator.getDeviceToken() !== "" &&
						buzzerClient.getProperty("Client.deviceId") === "")
                {
                    pin.controller.registerDeviceId(localNotificator.getDeviceToken());
                }
            }
            else if (pinAction === actionUnlock)
            {
                lResult = buzzerClient.open(pinField.hiddenText);

                if(localNotificator && localNotificator.getDeviceToken() !== "" &&
						buzzerClient.getProperty("Client.deviceId") === "")
                {
                    pin.controller.registerDeviceId(localNotificator.getDeviceToken());
                }
            }

            if (!buzzerClient.unlocked() || lResult === 1001 /* incorrect pin */)
            {
                controller.showError(buzzerApp.getLocalization(buzzerClient.locale, "Pin.Incorrect"));
                pinField.hiddenText = "";
            }
            else
            {
                if (nextPage === "back")
                {
                    unsubscribe();
                    finalized_ = true;

                    destroy(500);
                    controller.popPage();
                }
                else
                {
                    var lComponent = Qt.createComponent(nextPage);
                    var lPage = lComponent.createObject(pin.controller);
                    lPage.controller = pin.controller;

                    unsubscribe();
                    finalized_ = true;

                    pin.controller.addPage(lPage);
                }
            }
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
        visible: buzzerApp.isFingerprintAccessConfigured()

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

    QuarkLabel
    {
        id: versionLabel
        x: 25
        y: removeAccount.y + removeAccount.calculatedHeight / 2 - height / 2
        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.disabled");
        text: "v" + buzzerApp.getVersion()
        font.pointSize: 14
    }
}

