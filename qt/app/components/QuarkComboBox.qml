// QuarkComboBox.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

ComboBox
{
    id: comboBox

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

    textRole: "name"

    property int fontPixelSize: 14;
    property int fontPointSize: 14;
    property int popUpWidth: comboBox.width;
    property int itemRightPadding: -5;
    property int itemLeftPadding: 5;
    property int itemHorizontalAlignment: Text.AlignRight;
    property bool popUpAlignLeft: true;
    property var keySymbols: [];
    property int keyTopPadding: 0;
    property bool readOnly: false;

    onPopUpWidthChanged:
    {
        popup.adjustPosition();
    }

    onPopUpAlignLeftChanged:
    {
        popup.adjustPosition();
    }

    onWidthChanged:
    {
        popup.adjustPosition();
    }

	contentItem: QuarkLabelRegular
    {
        leftPadding: itemLeftPadding
        rightPadding: itemRightPadding

        text: getText()
        color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: itemHorizontalAlignment
        elide: Text.ElideRight
        font.pixelSize: fontPixelSize

        function getText()
        {
            var lItem = getItem(comboBox.displayText);
            if (!lItem || !getKeySymbol(lItem.key)) return comboBox.displayText;
            return "";
        }

        QuarkSymbolLabel
        {
            color: getSymbolColor()
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: itemHorizontalAlignment
            elide: Text.ElideRight

            visible: getVisible()
            font.pointSize: fontPointSize

            symbol: getSymbol()

            x: getX()
            y: getY()

            function getX()
            {
                if (itemHorizontalAlignment == Text.AlignRight)
                {
                    return comboBox.width - width - 30;
                }
                else if (itemHorizontalAlignment == Text.AlignLeft)
                {
                    return 10;
                }
                else if (itemHorizontalAlignment == Text.AlignHCenter)
                {
                    return comboBox.width / 2 - width / 2;
                }

                return 0;
            }

            function getY()
            {
                return comboBox.height / 2 - height / 2 + keyTopPadding;
            }

            function getVisible()
            {
                var lItem = comboBox.getItem(comboBox.displayText);
                if (lItem && comboBox.getKeySymbol(lItem.key)) return true;
                return false;
            }

            function getSymbol()
            {
                var lItem = comboBox.getItem(comboBox.displayText);
                if (lItem)
                {
                    var lKeySymbol = getKeySymbol(lItem.key);
                    return lKeySymbol ? lKeySymbol.symbol : "";
                }

                return "";
            }

            function getSymbolColor()
            {
                var lItem = comboBox.getItem(comboBox.displayText);
                if (lItem)
                {
                    return lItem.symbolColor ? lItem.symbolColor : buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
                }

                return "";
            }
        }
    }

    function getKeySymbol(key)
    {
        for (var lIdx = 0; lIdx < keySymbols.length; lIdx++)
        {
            if (keySymbols[lIdx].key === key) return keySymbols[lIdx];
        }

        return null;
    }

    function setItem(key)
    {
        for (var lIdx = 0; lIdx < model.count; lIdx++)
        {
            var lItem = model.get(lIdx);
            if (lItem.key === key) { currentIndex = lIdx; break; }
        }
    }

    function getItem(name)
    {
        for (var lIdx = 0; lIdx < model.count; lIdx++)
        {
            var lItem = model.get(lIdx);
            if (lItem.name === name) return lItem;
        }

        return null;
    }

    delegate: ItemDelegate
    {
        id: listDelegate
        width: popUpWidth
        contentItem: Rectangle
        {
            color: comboBox.highlightedIndex === index ?
                       buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.highlight"):
                       buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
            border.color: "transparent"
            anchors.fill: parent

            QuarkSymbolLabel
            {
                anchors.fill: parent
                anchors.leftMargin: 10
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                //horizontalAlignment: itemHorizontalAlignment
                Material.background: "transparent"
                Material.foreground: getSymbolColor()
                visible: getKeySymbol(key)
                font.pointSize: fontPointSize //16

                symbol: getSymbol()

                function getSymbol()
                {
                    var lKeySymbol = getKeySymbol(key);
                    return lKeySymbol ? lKeySymbol.symbol : "";
                }

                function getSymbolColor()
                {
                    try
                    {
                        if (symbolColor) return symbolColor;
                        return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
                    }
                    catch(ex)
                    {
                    }

                    return buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
                }
            }

			QuarkLabelRegular
            {
                id: textLabel
                anchors.fill: parent
                anchors.leftMargin: 10
                text: getText()
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                //horizontalAlignment: itemHorizontalAlignment
                Material.background: "transparent"
                visible: !getKeySymbol(key)
                function getText()
                {
                    return name;
                }
            }

        }
        highlighted: comboBox.highlightedIndex === index
    }

	popup: QuarkPopup
    {
        x: adjustPosition()
        y: comboBox.height - 1
        width: popUpWidth
        implicitHeight: contentItem.implicitHeight
        padding: 0
        enabled: !comboBox.readOnly

        Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
        Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
        Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Page.background");
        Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
        Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

        contentItem: QuarkListView
        {
            clip: true
            implicitHeight: contentHeight
            model: comboBox.popup.visible ? comboBox.delegateModel : null
            currentIndex: comboBox.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        function adjustPosition()
        {
            x = comboBox.popUpAlignLeft == true ? 0 : (comboBox.width - comboBox.popUpWidth);
            return x;
        }
    }
}

