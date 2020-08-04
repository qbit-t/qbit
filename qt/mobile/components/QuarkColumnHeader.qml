// QuarkColumnHeader.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

Item
{
    id: columnHeader

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

    property string text: "";
    property int calculatedWidth: 0;
    property int calculatedHeight: 0;
    property string order: ""; // asc, desc
    property int minWidth: 0;
    property int fixedWidth: 0;
    property bool active: false;
    property string field: "";
    property int fontPointSize: 14;
    property int columnAlign: Qt.AlignLeft;
    property int indicatorWidth: 20
    property int indicatorOffset: 7
    property bool allowIndicator: true
    property int labelTextElide: Text.ElideNone

    property string activeColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    property string inActiveColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.textDisabled");

    property string currentColor: inActiveColor;

    signal headerClicked;
    signal orderClicked;

    onFixedWidthChanged:
    {
        header.width = fixedWidth;
    }

    QuarkLabel
    {
        id: header
        text: columnHeader.text
        font.pointSize: fontPointSize
        color: currentColor
        //width: fixedWidth == 0 ? width : fixedWidth
        x: columnAlign == Qt.AlignLeft ? 0 : indicatorWidth + indicatorOffset
        elide: labelTextElide

        onWidthChanged:
        {
            var lWidth = header.width + indicatorWidth + indicatorOffset;
            calculatedWidth = lWidth < minWidth ? minWidth : lWidth;
        }

        onHeightChanged:
        {
            calculatedHeight = height;
        }
    }

    QuarkLabel
    {
        id: order
        x: getX()
        y: header.y + 2
        //renderType: Text.NativeRendering
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: getText()
        color: currentColor
        visible: allowIndicator

        function getX()
        {
            if (columnAlign == Qt.AlignLeft) return (header.width + indicatorOffset);

            return (header.x - indicatorOffset - indicatorWidth) + (indicatorWidth) / 2;
        }

        function getText()
        {
            if (!active) return "";
            if (columnHeader.order === "") return "";
            if (columnHeader.order === "asc") return Fonts.arrowUpSym;
            return Fonts.arrowDownSym;
        }

        font.family: Fonts.icons
        font.weight: Font.Normal
        font.pointSize: fontPointSize - 2

        onWidthChanged:
        {
            var lWidth = header.width + indicatorWidth + indicatorOffset;
            calculatedWidth = lWidth < minWidth ? minWidth : lWidth;
        }
    }

    MouseArea
    {
        x: header.x
        y: header.y

        width: header.width
        height: header.height + 10

        onClicked:
        {
            columnHeader.headerClicked();
        }
    }

    MouseArea
    {
        x: order.x
        y: order.y

        width: order.width + 20
        height: order.height + 10

        onClicked:
        {
            columnHeader.orderClicked();
        }
    }
}

