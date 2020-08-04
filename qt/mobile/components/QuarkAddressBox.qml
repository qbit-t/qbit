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
    id: addressBox

    height: 50

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
    border.color: "transparent"
    color: "transparent"

    property string symbol: Fonts.tagSym;
    property string text: "";
    property string placeholderText;
    property int symbolLeftPadding: 0;
    property int textLeftPadding: 0;
    property string borderColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.border")
    property bool scan: true
    property bool copy: true
    property bool help: false
    property string innerAction: "dropDown"; // "add"

    property int highlightedIndex: -1

    property QuarkListModel model: null

    signal scanButtonClicked();
    signal helpButtonClicked();
    signal addAddress(var address);
    signal removeAddress(var id);

    function synchronize()
    {
        infoLabel.text = addressBox.text;
        highlightedIndex = getAddressIndex(addressBox.text);
        dropDownAction.adjust();
    }

    onTextChanged:
    {
        synchronize();
    }

    onModelChanged:
    {
        popUpListView.model = addressBox.model;
    }

    Rectangle
    {
        id: symbolRect

        width: 50
        height: 50

        border.color: borderColor
        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background")

        QuarkSymbolButton
        {
            x: -5
            y: -5
            width: parent.width + 10
            height: parent.height + 10

            symbol: Fonts.closeSym
            Material.background: "transparent"

            onClicked:
            {
                addressBox.text = "";
                infoLabel.text = addressBox.placeholderText;
            }
        }
    }

    function isAddressExists(address)
    {
        if (!model) return false;
        for (var lIdx = 0; lIdx < model.count; lIdx++)
        {
            if (model.get(lIdx).address === address) return true;
        }

        return false;
    }

    function getAddressIndex(address)
    {
        if (!model) return -1;
        for (var lIdx = 0; lIdx < model.count; lIdx++)
        {
            if (model.get(lIdx).address === address) return lIdx;
        }

        return -1;
    }

    Rectangle
    {
        id: addressRect

        x: symbolRect.width - 1
        height: 50
        width: addressBox.width - symbolRect.width - scanButton.width * scan - copyButton.width * copy - helpButton.width * help +
               scan*1 + copy*1 + help*1 + 1

        border.color: borderColor
        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Box.background")

        Label
        {
            id: infoLabel
            x: 5 + textLeftPadding
            y: parent.height / 2 - height / 2
            width: parent.width - dropDownAction.width - (15 + textLeftPadding)
            elide: Text.ElideRight

            text: !addressBox.text.length ? addressBox.placeholderText : addressBox.text
            color: getColor()

            onTextChanged:
            {
                color = getColor();
                dropDownAction.adjust();
            }

            function getColor()
            {
                if (getText().length) return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
                return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.textDisabled");
            }

            function getText()
            {
                if (text === addressBox.placeholderText) return "";
                return text;
            }
        }

        QuarkSymbolLabel
        {
            id: dropDownAction
            x: infoLabel.width + 9
            y: parent.height / 2 - height / 2
            font.pointSize: 16
            symbol: getSymbol()

            function getSymbol()
            {
                var lText = infoLabel.getText();
                var lExists = isAddressExists(lText);
                if (lText.length && !lExists) { innerAction = "add"; return Fonts.circleAddSym; }
                innerAction = "dropDown";
                return Fonts.chevronCircleSym;
            }

            function adjust()
            {
                symbol = getSymbol();
            }

            color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
        }

        MouseArea
        {
            x: dropDownAction.x - 20
            y: 0
            width: dropDownAction.width + 25
            height: parent.height

            onClicked:
            {
                if (innerAction === "dropDown")
                {
                    if (popUp.visible) popUp.close();
                    else popUp.open();
                }
                else
                {
                    addAddress(infoLabel.text);
                }
            }
        }

        MouseArea
        {
            x: infoLabel.x
            y: 0
            width: infoLabel.width - 20
            height: parent.height

            onClicked:
            {
                if (!infoLabel.getText().length) popUp.open();
                else if (infoLabel.elide == Text.ElideRight) infoLabel.elide = Text.ElideLeft;
                else infoLabel.elide = Text.ElideRight;
            }
        }
    }

    Rectangle
    {
        id: scanButton

        x: addressRect.x + addressRect.width - 1
        width: getWidth()
        height: 50
        visible: scan
        clip: true

        border.color: borderColor;
        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");

        function getWidth()
        {
            if (scan) return 50;
            return 0;
        }

        QuarkSymbolButton
        {
            x: -5
            y: -5
            width: parent.width + 10
            height: parent.height + 10

            symbol: Fonts.qrcodeSym
            Material.background: "transparent"

            onClicked:
            {
                scanButtonClicked();
            }
        }
    }

    Rectangle
    {
        id: copyButton

        x: scanButton.x + scanButton.width - 1
        width: getWidth()
        height: 50
        visible: copy
        clip: true

        border.color: borderColor;
        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");

        function getWidth()
        {
            if (copy) return 50;
            return 0;
        }

        QuarkSymbolButton
        {
            x: -5
            y: -5
            width: parent.width + 10
            height: parent.height + 10

            symbol: Fonts.pasteSym
            Material.background: "transparent"

            onClicked:
            {
                addressBox.text = clipboard.getText();
            }
        }
    }

    Rectangle
    {
        id: helpButton

        x: copyButton.x + copyButton.width - 1
        width: getWidth()
        height: 50
        visible: help
        clip: true

        border.color: borderColor;
        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");

        function getWidth()
        {
            if (help) return 50;
            return 0;
        }

        QuarkSymbolButton
        {
            x: -5
            y: -5
            width: parent.width + 10
            height: parent.height + 10

            symbol: Fonts.helpSym
            Material.background: "transparent"

            onClicked:
            {
                helpButtonClicked();
            }
        }
    }

    Popup
    {
        id: popUp
        x: 0
        y: addressBox.height
        width: addressBox.width - 50
        implicitHeight: contentItem.implicitHeight
        padding: 0

        Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
        Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
        Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
        Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
        Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

        signal adjust();

        onOpened:
        {
            popUp.adjust();
        }

        contentItem: QuarkListView
        {
            id: popUpListView
            clip: true
            implicitHeight: contentHeight
            model: addressBox.model
            //currentIndex: addressBox.highlightedIndex

            delegate: SwipeDelegate
            {
                id: listDelegate
                width: popUp.width
                height: 48
                leftPadding: 0
                rightPadding: 0
                topPadding: 0
                bottomPadding: 0

                onClicked:
                {
                    addressBox.highlightedIndex = index;
                    listRect.setHighlight();
                    addressBox.text = address;
                    popUp.close();
                }

                contentItem: Rectangle
                {
                    id: listRect

                    function setHighlight()
                    {
                        listDelegate.highlighted = true;
                        color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.highlight");
                    }

                    function resetHighlight()
                    {
                        listDelegate.highlighted = false;
                        color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
                    }

                    function getColor()
                    {
                        return listDelegate.highlighted ?
                            buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.highlight"):
                            buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
                    }

                    color: getColor()
                    border.color: "transparent"
                    width: listDelegate.width
                    height: listDelegate.height

                    QuarkLabel
                    {
                        id: addressLabel
                        x: 10
                        y: 5
                        width: popUp.width - 10

                        elide: Text.ElideRight
                        text: address

                        Material.background: "transparent"
                    }

                    QuarkLabel
                    {
                        id: labelLabel
                        x: 10
                        y: addressLabel.y + addressLabel.height + 2
                        font.pointSize: 12
                        text: label
                        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.textDisabled")
                    }

//                    MouseArea
//                    {
//                        x: listRect.x
//                        y: listRect.y
//                        width: listRect.width
//                        height: listRect.height
//                        hoverEnabled: true

//                        onEntered:
//                        {
//                            listRect.setHighlight();
//                        }
//                        onExited:
//                        {
//                            listRect.resetHighlight();
//                        }
//                        onClicked:
//                        {
//                            addressBox.highlightedIndex = index;
//                            infoLabel.text = address;
//                            popUp.close();
//                        }
//                    }
                }
                swipe.right: Rectangle
                {
                    id: swipeRect
                    width: listDelegate.height
                    height: listDelegate.height
                    color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground")

                    QuarkSymbolLabel
                    {
                        symbol: Fonts.trashSym
                        font.pointSize: 16

                        x: parent.width / 2 - width / 2
                        y: parent.height / 2 - height / 2
                    }

                    MouseArea
                    {
                        anchors.fill: swipeRect
                        onPressed:
                        {
                            swipeRect.color = Qt.darker(buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground"), 1.1);
                        }
                        onReleased:
                        {
                            swipeRect.color = buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.downBackground");
                        }
                        onClicked:
                        {
                            listDelegate.swipe.close();
                            removeAddress(id);
                        }
                    }

                    anchors.right: parent.right
                }

                Component.onCompleted:
                {
                    popUp.adjust.connect(adjustState);
                }

                Component.onDestruction:
                {
                    popUp.adjust.disconnect(adjustState);
                }

                function adjustState()
                {
                    if (addressBox.highlightedIndex === index)
                    {
                        listRect.setHighlight();
                    }
                    else listRect.resetHighlight();

                    listDelegate.swipe.close();
                }
            }

            ScrollIndicator.vertical: ScrollIndicator { }
        }
    }
}

