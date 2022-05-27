// QuarkPriceLabel.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/lib/numberFunctions.js" as NumberFunctions

Label
{
    id: priceLabel

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

    property string trend: "up"; // "down"
    property real number: -1000000000.0;
    property string numberText: "";
    property bool useNumberText: false;
    property int fillTo: 9;

    property int calculatedWidth: 0;
    property int calculatedHeight: 0;

    property var priceUpColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.up")
    property var priceDownColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.down")
    property var mantissaColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.mantissa")
    property var zeroesColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.zeroes")
    property var subLevelZeroColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.subLevelAbsent")
    property var subLevelColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Price.subLevelPresent")

	//font.family: Qt.platform.os == "android" ? font.family : "Menlo-Regular"

    onTrendChanged:
    {
        applyChanges();
    }

    onFillToChanged:
    {
        applyChanges();
    }

    onNumberChanged:
    {
        applyChanges();
    }

    onNumberTextChanged:
    {
        applyChanges();
    }

    function applyChanges()
    {
        var lNumber = useNumberText ? numberText: NumberFunctions.scientificToDecimal(number.toString());
        if (typeof(lNumber) === "number")
        {
            lNumber = lNumber.toString();
        }

        if (!useNumberText && lNumber === "0") lNumber += ".0";
        else if (!useNumberText && lNumber === "0.") lNumber += "0";

        if (lNumber === "") lNumber = "0.";

        var lParts = lNumber.split(".");

        // lNumber[0] - significant part, lNumber[1] - decimal

		significant.text = lParts[0]; // + "."; // 000.
		if (fillTo > 0) significant.text += ".";

        // extract mantissa
        var lFillTo = 0;
        var lMaxFillTo = fillTo;
        var lAddSubLevel = true;
        if (fillTo == 9) lMaxFillTo = 8;
        else lAddSubLevel = false;

        var lMantissa = "";
        if (lParts[1])
        {
            for (var lIdx = 0; lIdx < lParts[1].length && lFillTo < lMaxFillTo; lIdx++)
            {
                lMantissa += lParts[1][lIdx];
                lFillTo++;
            }
        }

        mantissa.text = lMantissa;

        // add zeroes
        var lZeroes = "";
        for (lIdx = lFillTo; lIdx < lMaxFillTo; lIdx++) lZeroes += "0";

        zeroes.text = lZeroes;

        // add sublevel
        var lSubLevel = "";
        if (lAddSubLevel)
        {
            if (lParts[1] && lParts[1].length >= fillTo) lSubLevel += lParts[1][lMaxFillTo];
            else lSubLevel += "0";
        }

        sublevel.text = lSubLevel;

        calculatedWidth = significant.width + mantissa.width + zeroes.width + sublevel.width;
    }

	QuarkLabelRegular
    {
        id: significant
        x: 0
        y: 0
		font.pointSize: parent.font.pointSize

        onWidthChanged:
        {
            priceLabel.calculatedWidth = significant.width + mantissa.width + zeroes.width + sublevel.width;
        }

        onHeightChanged:
        {
            priceLabel.calculatedHeight = significant.height;
        }

        color: getColor()

        function getColor()
        {
            if (useNumberText && numberText == "") return zeroesColor;
            if (priceLabel.trend == "") return mantissaColor;
            return priceLabel.trend === "up" ? priceUpColor : priceDownColor;
        }
    }

	QuarkLabelRegular
    {
        id: mantissa
        x: significant.x + significant.width
        y: 0
		font.pointSize: parent.font.pointSize

        onWidthChanged:
        {
            priceLabel.calculatedWidth = significant.width + mantissa.width + zeroes.width + sublevel.width;
        }

        onHeightChanged:
        {
            priceLabel.calculatedHeight = significant.height;
        }

        color: mantissaColor
    }

	QuarkLabelRegular
    {
        id: zeroes
        x: mantissa.x + mantissa.width
        y: 0
		font.pointSize: parent.font.pointSize

        onWidthChanged:
        {
            priceLabel.calculatedWidth = significant.width + mantissa.width + zeroes.width + sublevel.width;
        }

        onHeightChanged:
        {
            priceLabel.calculatedHeight = significant.height;
        }

        color: zeroesColor
    }

	QuarkLabelRegular
    {
        id: sublevel
        x: zeroes.x + zeroes.width
        y: 0
		font.pointSize: parent.font.pointSize

        onWidthChanged:
        {
            priceLabel.calculatedWidth = significant.width + mantissa.width + zeroes.width + sublevel.width;
        }

        onHeightChanged:
        {
            priceLabel.calculatedHeight = significant.height;
        }

        color: sublevel.text === "0" ? subLevelZeroColor : subLevelColor
    }
}

