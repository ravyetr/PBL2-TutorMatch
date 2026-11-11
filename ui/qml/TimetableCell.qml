import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: cellRoot

    property int dayIndex: 0
    property int slotIndex: 0
    property string status: "FREE" // "FREE", "BUSY", "BOOKED", "ACCEPTED"
    property string displayName: ""
    property string primaryText: ""
    property string secondaryText: ""
    property string subjectName: ""
    property string tutorName: ""
    property string studentName: ""
    property string label: ""
    property string subLabel: ""
    property string timeRange: ""
    property string sessionLabel: ""
    property string date: ""
    property bool isCourse: false
    property int bookingId: 0
    property bool editable: true
    property bool locked: false

    signal toggleRequested(int day, int slot)

    readonly property bool isBooked: status === "BOOKED" || status === "ACCEPTED"
    readonly property bool isBusy: status === "BUSY"
    readonly property bool isFree: !isBooked && !isBusy

    // Adaptive sizing helpers for vertical constraints
    readonly property bool hasBadge: sessionLabel !== ""
    readonly property bool isUltraCompact: height < 40
    readonly property bool isCompact: height < 48

    clip: true
    radius: 6
    border.width: 1
    border.color: isBooked ? Theme.slotBookedText :
                  isBusy ? Theme.cardBorder : Theme.slotFreeBorder

    color: isBooked ? Theme.slotBookedBg :
           isBusy ? Theme.slotBusyBg :
           (cellMouseArea.containsMouse ? Theme.bgApp : Theme.cardBg)

    MouseArea {
        id: cellMouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: (cellRoot.isBooked || cellRoot.locked || !cellRoot.editable) ? Qt.ArrowCursor : Qt.PointingHandCursor
        enabled: true

        onClicked: {
            if (!cellRoot.isBooked && !cellRoot.locked && cellRoot.editable) {
                cellRoot.toggleRequested(cellRoot.dayIndex, cellRoot.slotIndex)
            }
        }

        ToolTip.visible: cellMouseArea.containsMouse && cellRoot.isBooked
        ToolTip.delay: 300
        ToolTip.text: (cellRoot.displayName ? (cellRoot.displayName + "\n") : "") +
                      (cellRoot.subjectName ? ("Môn: " + cellRoot.subjectName + "\n") : "") +
                      (cellRoot.sessionLabel ? (cellRoot.sessionLabel + "\n") : "") +
                      (cellRoot.timeRange ? ("Giờ: " + cellRoot.timeRange) : "")
    }

    Column {
        id: textColumn
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        spacing: (cellRoot.isCompact && cellRoot.hasBadge) ? 1 : 2
        z: 10
        visible: cellRoot.isBooked

        // Session badge (e.g. Buổi 1/4)
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: cellRoot.hasBadge
            spacing: 2

            Rectangle {
                implicitWidth: Math.min(parent.parent ? parent.parent.width : 60, badgeTxt.implicitWidth + 8)
                implicitHeight: cellRoot.isUltraCompact ? 10 : (cellRoot.isCompact ? 11 : 13)
                radius: 3
                color: Theme.cBurgundy

                Text {
                    id: badgeTxt
                    anchors.centerIn: parent
                    text: cellRoot.sessionLabel
                    font.bold: true
                    font.pixelSize: cellRoot.isUltraCompact ? 7 : (cellRoot.isCompact ? 7.5 : 8)
                    color: Theme.textLight
                }
            }
        }

        // Information Column: Counterpart Name + Subject Name with distinct vertical spacing
        Column {
            id: infoColumn
            width: parent.width
            spacing: cellRoot.isUltraCompact ? 1 : ((cellRoot.isCompact && cellRoot.hasBadge) ? 2 : 3)

            // Primary Text: Counterpart User Name (displayName)
            Text {
                id: primaryTxt
                objectName: "primaryTextItem"
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: cellRoot.displayName || cellRoot.primaryText || cellRoot.tutorName || cellRoot.studentName || cellRoot.label || ""
                font.bold: true
                font.pixelSize: cellRoot.isUltraCompact ? 8.5 : ((cellRoot.isCompact && cellRoot.hasBadge) ? 9.5 : (cellRoot.height >= 52 ? 11 : 10.5))
                color: Theme.slotBookedText
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            // Secondary Text: Subject Name (subjectName)
            Text {
                id: secondaryTxt
                objectName: "secondaryTextItem"
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                visible: text !== ""
                text: cellRoot.subjectName || cellRoot.secondaryText || cellRoot.subLabel || ""
                font.pixelSize: cellRoot.isUltraCompact ? 7.5 : ((cellRoot.isCompact && cellRoot.hasBadge) ? 8.5 : (cellRoot.height >= 52 ? 10 : 9.5))
                color: Theme.slotBookedText
                opacity: 0.90
                elide: Text.ElideRight
                maximumLineCount: 1
            }
        }
    }
}






