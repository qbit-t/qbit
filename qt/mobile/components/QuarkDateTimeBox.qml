// QuarkDateTimeBox.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/fonts"

Rectangle
{
    id: dateTimeBox

    height: 50

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");
    border.color: "transparent"
    color: "transparent"

    property string symbol;
    property string text;
    property string placeholderText;
    property int symbolLeftPadding: 0;
    property bool start: false;
    property bool end: false;
    property bool initialized: false;

    function reset()
    {
        dayBox.currentIndex = -1;
        monthBox.currentIndex = -1;
        yearBox.currentIndex = -1;
        hourBox.currentIndex = -1;
        minuteBox.currentIndex = -1;
    }

    function dateTime()
    {
        //console.log(yearBox.currentText + "-" + monthBox.currentText + "-" + dayBox.currentText + " " +
        //            hourBox.currentText + ":" + minuteBox.currentText + ":00.000");
        if (dayBox.currentText.length && monthBox.currentText.length && yearBox.currentText.length && hourBox.currentText.length && minuteBox.currentText.length)
            return yearBox.currentText + "-" + monthBox.currentText + "-" + dayBox.currentText + "T" +
                    hourBox.currentText + ":" + minuteBox.currentText + ":00Z";
        return "";
    }

    function setDateTime(day, month, year, hour, minute)
    {
        if (initialized) return;

        if (day !== -1) dayBox.currentIndex = day-1;
        if (month !== -1) monthBox.currentIndex = month;
        if (year !== -1)
        {
            var lNow = new Date();
            yearBox.currentIndex = lNow.getFullYear() - year;
        }

        if (hour !== -1) hourBox.currentIndex = hour;
        if (minute !== -1) minuteBox.currentIndex = minute;

        initialized = true;
    }

    function getWidthPart()
    {
        return boxRect.getWidth();
    }

    Rectangle
    {
        id: symbolRect

        width: 50
        height: 50

        border.color: "transparent";
        color: "transparent";

        Label
        {
            id: symbolLabel
            anchors.fill: parent
            leftPadding: symbolLeftPadding
            x: symbolLeftPadding
            //renderType: Text.NativeRendering
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: dateTimeBox.symbol
            color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");

            font.family: Fonts.icons
            font.weight: Font.Normal
            font.pointSize: 20
        }

        Label
        {
            id: extraLabel
            x: symbolLabel.x + symbolLabel.width - extraLabel.width - 2
            y: symbolLabel.y + 3
            text: (dateTimeBox.start ? Fonts.startSym : (dateTimeBox.end ? Fonts.endSym : ""))
            color: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");

            font.family: Fonts.icons
            font.weight: Font.Normal
            font.pointSize: 12
        }

        MouseArea
        {
            x: 0
            y: 0

            width: parent.width
            height: parent.height

            onClicked:
            {
                dayBox.currentIndex = -1;
                monthBox.currentIndex = -1;
                yearBox.currentIndex = -1;
                hourBox.currentIndex = -1;
                minuteBox.currentIndex = -1;
            }
        }
    }

    Rectangle
    {
        id: boxRect

        x: symbolRect.width - 1
        height: 50
        width: dateTimeBox.width - symbolRect.width + 1

        border.color: "transparent";
        color: "transparent";

        function getWidth()
        {
            var lTotalWidth = boxRect.width - 3*7 - 30;
            return lTotalWidth / 6;
        }

        QuarkComboBox
        {
            id: dayBox
            width: boxRect.getWidth()
            x: 5
            itemHorizontalAlignment: Text.AlignLeft
            itemLeftPadding: 10
            popup.height: 200

            //Material.background: "transparent"

            model: ListModel
            {
                id: dayModel_
            }

            Component.onCompleted:
            {
                fillModel();
            }

            function fillModel()
            {
                for (var lIdx = 1; lIdx <= 31; lIdx++)
                {
                    var lNumber = "";
                    if (lIdx < 10) lNumber = "0";
                    lNumber += lIdx.toString();
                    dayModel_.append({ name: lNumber, key: lNumber });
                }
            }

            indicator: Canvas
            {
                id: canvas0
                x: dayBox.width - width - dayBox.rightPadding + 5
                y: dayBox.topPadding + (dayBox.availableHeight - height) / 2
                width: 10
                height: 6
                contextType: "2d"

                Connections
                {
                    target: dayBox
                    onPressedChanged: canvas0.requestPaint()
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

        QuarkLabel
        {
            id: dayBoxSym
            x: dayBox.x + dayBox.width + 1
            y: dayBox.height / 2 - dayBoxSym.height / 2
            text: "-"
        }

        QuarkComboBox
        {
            id: monthBox
            width: boxRect.getWidth()
            x: dayBoxSym.x + dayBoxSym.width + 1
            itemHorizontalAlignment: Text.AlignLeft
            itemLeftPadding: 10
            popup.height: 200

            //Material.background: "transparent"

            model: ListModel
            {
                id: monthModel_
            }

            Component.onCompleted:
            {
                fillModel();
            }

            function fillModel()
            {
                for (var lIdx = 1; lIdx <= 12; lIdx++)
                {
                    var lNumber = "";
                    if (lIdx < 10) lNumber = "0";
                    lNumber += lIdx.toString();
                    monthModel_.append({ name: lNumber, key: lNumber });
                }
            }

            indicator: Canvas
            {
                id: canvas1
                x: monthBox.width - width - monthBox.rightPadding + 5
                y: monthBox.topPadding + (monthBox.availableHeight - height) / 2
                width: 10
                height: 6
                contextType: "2d"

                Connections
                {
                    target: monthBox
                    onPressedChanged: canvas1.requestPaint()
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

        QuarkLabel
        {
            id: monthBoxSym
            x: monthBox.x + monthBox.width + 1
            y: monthBox.height / 2 - monthBoxSym.height / 2
            text: "-"
        }

        QuarkComboBox
        {
            id: yearBox
            width: boxRect.getWidth() * 2
            x: monthBoxSym.x + monthBoxSym.width + 1
            itemHorizontalAlignment: Text.AlignLeft
            itemLeftPadding: 10
            popup.height: 200

            //Material.background: "transparent"

            model: ListModel
            {
                id: yearModel_
            }

            Component.onCompleted:
            {
                fillModel();
            }

            function fillModel()
            {
                var lNow = new Date();
                for (var lIdx = lNow.getFullYear(); lIdx >= 2015; lIdx--)
                {
                    yearModel_.append({ name: "" + lIdx, key: "" + lIdx });
                }
            }

            indicator: Canvas
            {
                id: canvas2
                x: yearBox.width - width - yearBox.rightPadding + 5
                y: yearBox.topPadding + (yearBox.availableHeight - height) / 2
                width: 10
                height: 6
                contextType: "2d"

                Connections
                {
                    target: yearBox
                    onPressedChanged: canvas2.requestPaint()
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

        QuarkComboBox
        {
            id: hourBox
            width: boxRect.getWidth()
            x: yearBox.x + yearBox.width + 10
            itemHorizontalAlignment: Text.AlignLeft
            itemLeftPadding: 10
            popup.height: 200

            //Material.background: "transparent"

            model: ListModel
            {
                id: hourModel_
            }

            Component.onCompleted:
            {
                fillModel();
            }

            function fillModel()
            {
                for (var lIdx = 0; lIdx <= 23; lIdx++)
                {
                    var lNumber = "";
                    if (lIdx < 10) lNumber = "0";
                    lNumber += lIdx.toString();
                    hourModel_.append({ name: lNumber, key: lNumber });
                }
            }

            indicator: Canvas
            {
                id: canvas3
                x: hourBox.width - width - hourBox.rightPadding + 5
                y: hourBox.topPadding + (hourBox.availableHeight - height) / 2
                width: 10
                height: 6
                contextType: "2d"

                Connections
                {
                    target: hourBox
                    onPressedChanged: canvas3.requestPaint()
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

        QuarkLabel
        {
            id: hourBoxSym
            x: hourBox.x + hourBox.width + 1
            y: hourBox.height / 2 - hourBoxSym.height / 2
            text: ":"
        }

        QuarkComboBox
        {
            id: minuteBox
            width: boxRect.getWidth()
            x: hourBoxSym.x + hourBoxSym.width + 1
            itemHorizontalAlignment: Text.AlignLeft
            itemLeftPadding: 10
            popup.height: 200

            //Material.background: "transparent"

            model: ListModel
            {
                id: minuteModel_
            }

            Component.onCompleted:
            {
                fillModel();
            }

            function fillModel()
            {
                for (var lIdx = 0; lIdx <= 59; lIdx++)
                {
                    var lNumber = "";
                    if (lIdx < 10) lNumber = "0";
                    lNumber += lIdx.toString();
                    minuteModel_.append({ name: lNumber, key: lNumber });
                }
            }

            indicator: Canvas
            {
                id: canvas4
                x: minuteBox.width - width - minuteBox.rightPadding + 5
                y: minuteBox.topPadding + (minuteBox.availableHeight - height) / 2
                width: 10
                height: 6
                contextType: "2d"

                Connections
                {
                    target: minuteBox
                    onPressedChanged: canvas4.requestPaint()
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

