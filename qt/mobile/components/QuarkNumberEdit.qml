// QuarkNumberEdit.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

import "qrc:/lib/numberFunctions.js" as NumberFunctions

Item
{
    id: numberEdit

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

    property int fontPointSize: 12;
    property int popUpWidth: editRect.width;
    property int itemRightPadding: -5;
    property int itemLeftPadding: 5;
    property int itemHorizontalAlignment: Text.AlignHCenter;
    property bool popUpAlignLeft: true;

    property string priceUpColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
    property string priceDownColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
    property string trend: ""

    property alias popUpOpened: popup.opened

    property bool percentButtons: false;
    property bool marketButton: false;
    property bool xButtons: true;
    property bool readOnly: false;

    property real number: 0.0;
    property real zeroNumber: 0.000000000001;
    property string numberString: "";
    property int fillTo: 9;
    property string minDelta: "";

    property bool emitPopupClosed_: true;

    signal numberStringModified();
    signal popupClosed();

    signal percentClicked(var percent);
    signal marketClicked();

    function closePopup()
    {
        popup.close();
    }

    onFillToChanged:
    {
        numberLabel.fillTo = fillTo;
    }

    Component.onCompleted:
    {
        numberLabel.numberText = numberString;
    }

    onNumberStringChanged:
    {
        numberLabel.numberText = numberString;
    }

    onPopUpWidthChanged:
    {
        popup.adjustPosition();
    }

    onPopUpAlignLeftChanged:
    {
        popup.adjustPosition();
    }

    QuarkSymbolTap
    {
        id: minusButton
        x: 0
        y: 0
        symbol: Fonts.minusSym
        border.color: "transparent"
        labelColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.zeroes");
        visible: !readOnly
        width: readOnly ? 0 : parent.height

        onClicked:
        {
            //
            if (!numberString.length) return;

            var lMinDelta = getMinDelta(numberString);
            var lNumber = parseFloat(numberString);

            if (lNumber >= zeroNumber)
            {
                var lResult = appHelper.minusReal(lNumber, lMinDelta);
                if (lResult === "0") numberString = "";
                else numberString = NumberFunctions.scientificToDecimal(lResult);
            }
            else
            {
                numberString = "";
            }

            numberStringModified();
        }
    }

    Rectangle
    {
        id: editRect

        x: minusButton.width - 1
        height: parent.height
        width: numberEdit.width - minusButton.width - plusButton.width + 1
        clip: true

        border.color: "transparent"
        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background");

        QuarkPriceLabel
        {
            id: numberLabel

            x: parent.width / 2 - numberLabel.calculatedWidth / 2
            y: parent.height / 2 - numberLabel.calculatedHeight / 2

            trend: numberEdit.trend

            leftPadding: itemLeftPadding
            rightPadding: itemRightPadding

            numberText: numberText
            useNumberText: true

            fillTo: fillTo
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: itemHorizontalAlignment
            elide: Text.ElideRight
            font.pointSize: numberEdit.fontPointSize

            priceUpColor: numberEdit.priceUpColor
            priceDownColor: numberEdit.priceDownColor
        }

        MouseArea
        {
            x: 0
            y: 0
            width: parent.width
            height: parent.height
            enabled: !readOnly

            onClicked:
            {
                if (popup.visible) popup.close();
                else popup.open();
            }
        }

        Popup
        {
            id: popup

            x: adjustPosition()
            y: parent.height
            width: 200
            height: 320 - (!percentButtons && !xButtons ? 50 : 0)
            topPadding: 10
            visible: false

            //closePolicy: Popup.NoAutoClose

            Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
            Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
            Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background");
            Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
            Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

            function allowEditing()
            {
                var lPointIdx = numberString.indexOf(".");
                if ((lPointIdx > 0 && numberString.split(".")[1].length < fillTo) || lPointIdx == -1)
                {
                    return true;
                }

                return false;
            }

            onOpened:
            {
                emitPopupClosed_ = true;
            }

            onClosed:
            {
                if (emitPopupClosed_) popupClosed();
            }

            contentItem: Item
            {
                Grid
                {
                    id: grid
                    columns: 3
                    columnSpacing: 10
                    rowSpacing: 0

                    x: parent.width / 2 - grid.width / 2
                    y: 0

                    QuarkButton { width: 50; id: keyPad1; text: "1"; onClicked: numberString += "1"; font.pointSize: 16; enabled: popup.allowEditing(); }
                    QuarkButton { width: 50; id: keyPad2; text: "2"; onClicked: numberString += "2"; font.pointSize: 16; enabled: popup.allowEditing(); }
                    QuarkButton { width: 50; id: keyPad3; text: "3"; onClicked: numberString += "3"; font.pointSize: 16; enabled: popup.allowEditing(); }
                    QuarkButton { width: 50; id: keyPad4; text: "4"; onClicked: numberString += "4"; font.pointSize: 16; enabled: popup.allowEditing(); }
                    QuarkButton { width: 50; id: keyPad5; text: "5"; onClicked: numberString += "5"; font.pointSize: 16; enabled: popup.allowEditing(); }
                    QuarkButton { width: 50; id: keyPad6; text: "6"; onClicked: numberString += "6"; font.pointSize: 16; enabled: popup.allowEditing(); }
                    QuarkButton { width: 50; id: keyPad7; text: "7"; onClicked: numberString += "7"; font.pointSize: 16; enabled: popup.allowEditing(); }
                    QuarkButton { width: 50; id: keyPad8; text: "8"; onClicked: numberString += "8"; font.pointSize: 16; enabled: popup.allowEditing(); }
                    QuarkButton { width: 50; id: keyPad9; text: "9"; onClicked: numberString += "9"; font.pointSize: 16; enabled: popup.allowEditing(); }
                    QuarkButton { width: 50; id: keyPadP; text: ".";
                        onClicked: { if (!numberString.length) numberString += "0."; else numberString += "."; }
                        font.pointSize: 16;
                        enabled: numberString.indexOf(".") == -1; }
                    QuarkButton { width: 50; id: keyPad0; text: "0";
                        onClicked: { if (!numberString.length) numberString += "0."; else numberString += "0"; }
                        font.pointSize: 16;
                        enabled: popup.allowEditing(); }
                    QuarkSymbolButton { width: 50; symbol: Fonts.backspaceSym;
                        onClicked: { if (numberString == "0.") numberString = ""; else numberString = numberString.slice(0, -1); } }

                    QuarkButton { width: 50; visible: !percentButtons && xButtons && !marketButton; id: keyPadx5; text: "x5"; onClicked: numberString = "0.00000"; font.pointSize: 16; enabled: fillTo > 5; }
                    QuarkButton { width: 50; visible: !percentButtons && xButtons && !marketButton; id: keyPadx6; text: "x6"; onClicked: numberString = "0.000000"; font.pointSize: 16; enabled: fillTo > 6; }
                    QuarkButton { width: 50; visible: !percentButtons && xButtons && !marketButton; id: keyPadx7; text: "x7"; onClicked: numberString = "0.0000000"; font.pointSize: 16; enabled: fillTo > 7; }

                    QuarkButton { width: 50; visible: !marketButton && percentButtons; id: keyPadx25; text: "25%"; onClicked: percentClicked(25); font.pointSize: 12; }
                    QuarkButton { width: 50; visible: !marketButton && percentButtons; id: keyPadx50; text: "50%"; onClicked: percentClicked(50); font.pointSize: 12; }
                    QuarkButton { width: 50; visible: !marketButton && percentButtons; id: keyPadx75; text: "75%"; onClicked: percentClicked(75); font.pointSize: 12; }
                }

                height: grid.height + submitButton.height + 10
                width: grid.width

                QuarkButton
                {
                    width: 150 + 20;
                    visible: marketButton; id: keyPadMarket;
                    text: buzzerApp.getLocalization(buzzerClient.locale, "Button.Market");
                    font.pointSize: 12;

                    x: grid.x
                    y: grid.y + grid.height + 5

                    onClicked:
                    {
                        emitPopupClosed_ = false;
                        popup.close();

                        marketClicked();
                    }
                }

                QuarkSymbolButton
                {
                    id: clearButton
                    symbol: Fonts.closeSym
                    width: 50

                    x: grid.x
                    y: grid.y + grid.height + 10 + (marketButton ? keyPadMarket.height : 0)

                    onClicked:
                    {
                        numberString = "";
                    }
                }

                QuarkButton
                {
                    id: submitButton
                    text: buzzerApp.getLocalization(buzzerClient.locale, "Button.Submit")
                    enabled: true

                    x: clearButton.x + clearButton.width + 10
                    y: grid.y + grid.height + 10 + (marketButton ? keyPadMarket.height: 0)

                    width: clearButton.width * 2 + 10

                    onClicked:
                    {
                        emitPopupClosed_ = false;
                        popup.close();
                        numberStringModified();
                    }
                }
            }

            function adjustPosition()
            {
                x = popUpAlignLeft === true ? -minusButton.width: (parent.width - popUpWidth + plusButton.width);
                return x;
            }
        }
    }

    QuarkSymbolTap
    {
        id: plusButton
        x: parent.width - parent.height
        y: 0
        symbol: Fonts.plusSym
        border.color: "transparent"
        labelColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.zeroes");
        visible: !readOnly
        width: readOnly ? 0 : parent.height

        onClicked:
        {
            //
            if (!numberString.length)
            {
                numberString = minNumber();
            }
            else
            {
                var lMinDelta = getMinDelta(numberString);
                var lNumber = parseFloat(numberString);
                var lResult = appHelper.addReal(lNumber, lMinDelta);
                numberString = NumberFunctions.scientificToDecimal(lResult);
            }

            numberStringModified();
        }
    }

    function minNumber()
    {
        if (minDelta.length) return minDelta;

        var lNumber = "0.";
        for (var lIdx = 0; lIdx < fillTo - 1; lIdx++)
        {
            lNumber += "0";
        }

        lNumber += "1";

        return lNumber;
    }

    function getMinDelta(num)
    {
        if (minDelta.length) return parseFloat(minDelta);

        var lParts = num.split(".");
        if (!lParts[1] || !lParts[1].length || lParts[1] === "0") return 1.0;

        var lSymDelta = "0.";
        for (var lIdx = 0; lIdx < lParts[1].length-1; lIdx++)
        {
            lSymDelta += "0";
        }

        lSymDelta += "1";

        return parseFloat(lSymDelta);
    }
}

