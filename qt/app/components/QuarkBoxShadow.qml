// QuarkBoxShadow.qml

import QtQuick 2.12
import QtQuick.Controls.Material 2.12
import QtQuick.Controls.Material.impl 2.12

/*
   A implementation of CSS's box-shadow, used by ElevationEffect for a Material Design
   elevation shadow effect.
 */
RectangularGlow {
    // The 4 properties from CSS box-shadow, plus the inherited color property
    property int offsetX
    property int offsetY
    property int blurRadius
    property int spreadRadius
	property int explicitWidth
	property int explicitHeight
	property double explicitRadius

    // The source item the shadow is being applied to, used for correctly
    // calculating the corner radious
    property Item source

    property bool fullWidth
    property bool fullHeight

    x: (parent.width - width)/2 + offsetX
    y: (parent.height - height)/2 + offsetY

	implicitWidth: explicitWidth ? explicitWidth : source ? source.width : parent.width
	implicitHeight: explicitHeight ? explicitHeight : source ? source.height : parent.height

    width: implicitWidth + 2 * spreadRadius + (fullWidth ? 2 * cornerRadius : 0)
    height: implicitHeight + 2 * spreadRadius + (fullHeight ? 2 * cornerRadius : 0)
    glowRadius: blurRadius/2
    spread: 0.05
	cornerRadius: blurRadius + (explicitRadius ? explicitRadius : (source && source.radius || 0))
}
