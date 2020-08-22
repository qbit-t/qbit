// QuarkNumberLabel.qml

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1
import QtQuick.Controls.Styles 1.4

import "qrc:/lib/numberFunctions.js" as NumberFunctions

Label
{
    id: numberLabel

    Material.theme: buzzerClient.themeSelector == "dark" ? Material.Dark : Material.Light;
    Material.accent: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.accent");
    Material.background: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.background");
    Material.foreground: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground");
    Material.primary: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.primary");

    property real number: -1000000000.0;
    property real zeroNumber: 0.000000000001;
    property int fillTo: 4;
    property bool useSign: false;
	property int unitsGap: 2

    property string units: "";

	property string sign: "";

	property bool compact: false;

    property int calculatedWidth: 0;
    property int calculatedHeight: 0;

    property bool mayCompact: false;

    property var numberColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
    property var mantissaColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
    property var zeroesColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")
    property var unitsColor: buzzerApp.getColor(buzzerClient.theme, buzzerClient.themeSelector, "Material.foreground")

    font.family: Qt.platform.os == "android" ? font.family : "Menlo-Regular"

    onNumberChanged:
    {
        formatNumber();
    }

    onCompactChanged:
    {
        formatNumber();
    }

    onUseSignChanged:
    {
        formatNumber();
    }

    onUnitsChanged:
    {
        formatNumber();
    }

    onFillToChanged:
    {
        formatNumber();
    }

    function compactNumber()
    {
        if (text.length) return;

        var lNumberCompact = NumberFunctions.numberToCompact(Math.abs(number)).toString();
        var lText = "";

		if (useSign) {
			if (sign === "") {
				if (number < zeroNumber) lText = "-"; else lText = "+";
			} else {
				lText += sign;
			}
		}

        lText += lNumberCompact;

        significant.text = lText;
        mantissa.text = "";
        zeroes.text = "";
    }

    function formatNumber()
    {
        if (text.length) return;

        if (!compact)
        {
            if (number > NumberFunctions.minCompactNumber() && mayCompact)
            {
                compactNumber();
            }
            else
            {
                var lNumber = NumberFunctions.scientificToDecimal(number.toString()).toString();
                if (lNumber === "0") lNumber += ".0";
                else if (lNumber === "0.") lNumber += "0";
                var lParts = lNumber.split(".");

                // lNumber[0] - significant part, lNumber[1] - decimal
                var lSignificant = "";

                if (useSign)
                {
					if (sign === "") {
						if (lParts[0][0] !== '-' && number < zeroNumber) lSignificant = "-";
						else lSignificant = "+";
					} else {
						lSignificant += sign;
					}
				}

                lSignificant += lParts[0] + "."; // +|-000.
                significant.text = lSignificant;

                // extract mantissa
                var lFillTo = 0;
                var lMaxFillTo = fillTo;

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
            }
        }
        else
        {
            compactNumber();
        }

        calculateWidth();
        calculatedHeight = significant.height;
    }

    onWidthChanged: calculateWidth()

    function calculateWidth()
    {
        calculatedWidth = significant.width + mantissa.width + zeroes.width + units.width;
    }

    QuarkLabel
    {
        id: significant
        x: 0
        y: 0

        onWidthChanged:
        {
            //numberLabel.calculatedWidth = significant.width + mantissa.width + zeroes.width + units.width + 2;
            calculateWidth();
        }
        onHeightChanged:
        {
            numberLabel.calculatedHeight = significant.height;
        }

        color: numberColor
    }

    QuarkLabel
    {
        id: mantissa
        x: significant.x + significant.width
        y: 0

        onWidthChanged:
        {
            //numberLabel.calculatedWidth = significant.width + mantissa.width + zeroes.width + units.width + 2;
            calculateWidth();
        }
        onHeightChanged:
        {
            numberLabel.calculatedHeight = mantissa.height;
        }

        color: mantissaColor
    }

    QuarkLabel
    {
        id: zeroes
        x: mantissa.x + mantissa.width
        y: 0

        onWidthChanged:
        {
            //numberLabel.calculatedWidth = significant.width + mantissa.width + zeroes.width + units.width + 2;
            calculateWidth();
        }
        onHeightChanged:
        {
            numberLabel.calculatedHeight = zeroes.height;
        }

        color: zeroesColor
    }

    QuarkLabel
    {
        id: units
		x: zeroes.x + zeroes.width + unitsGap
        y: 0

        text: numberLabel.units

        onWidthChanged:
        {
            //numberLabel.calculatedWidth = significant.width + mantissa.width + zeroes.width + units.width + 2;
            calculateWidth();
        }
        onHeightChanged:
        {
            numberLabel.calculatedHeight = units.height;
        }

        color: unitsColor
    }
}

