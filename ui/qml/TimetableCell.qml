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
        anchors.centerIn: parent
        width: parent.width - 6
        spacing: 2
        z: 10
        visible: cellRoot.isBooked

        // Session badge (e.g. Buổi 1/4)
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: cellRoot.sessionLabel !== ""
            spacing: 3
            Rectangle {
                implicitWidth: badgeTxt.implicitWidth + 8
                implicitHeight: 14
                radius: 3
                color: Theme.cBurgundy
                Text {
                    id: badgeTxt
                    anchors.centerIn: parent
                    text: cellRoot.sessionLabel
                    font.bold: true
                    font.pixelSize: 8
                    color: Theme.textLight
                }
            }
        }

        // Primary Text: Counterpart User Name (displayName)
        Text {
            id: primaryTxt
            objectName: "primaryTextItem"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: cellRoot.displayName || cellRoot.primaryText || cellRoot.tutorName || cellRoot.studentName || cellRoot.label || ""
            font.bold: true
            font.pixelSize: 11
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
            visible: cellRoot.subjectName !== "" || cellRoot.secondaryText !== "" || cellRoot.subLabel !== ""
            text: cellRoot.subjectName || cellRoot.secondaryText || cellRoot.subLabel
            font.pixelSize: 10
            color: Theme.slotBookedText
            elide: Text.ElideRight
            maximumLineCount: 1
        }
    }
}






