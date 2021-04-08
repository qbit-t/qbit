// QuarkBoxShadow.qml

import QtQuick 2.12
import QtQuick.Templates 2.12 as T
import QtQuick.Controls.Material 2.12
import QtQuick.Controls.Material.impl 2.12

T.BusyIndicator {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 6

    contentItem: BusyIndicatorImpl {
        implicitWidth: control.Material.touchTarget
        implicitHeight: control.Material.touchTarget
        color: control.Material.accentColor

        running: control.running
        opacity: control.running ? 1 : 0
        // Behavior on opacity { OpacityAnimator { duration: 250 } }
    }
}
