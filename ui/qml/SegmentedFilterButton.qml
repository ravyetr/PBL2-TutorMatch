import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TutorMatch

Rectangle {
    id: filterButtonRoot

    property string text: ""
    property bool active: false

    property color activeBgColor: Theme.cBurgundy        // #6C0820
    property color activeTextColor: Theme.textLight      // #FFFFFF
    property color activeBorderColor: Theme.cBurgundy    // #6C0820

    property color inactiveBgColor: Theme.cardBg         // #FFFFFF
    property color inactiveTextColor: Theme.sidebarBg    // #3D5D91
    property color inactiveBorderColor: Theme.cardBorder // #F2DCDB

    property color hoverBgColor: Theme.cSilverLakeBlue   // #5A86CB
    property color hoverTextColor: Theme.textLight       // #FFFFFF
    property color hoverBorderColor: Theme.cSilverLakeBlue // #5A86CB

    signal clicked()

    implicitWidth: labelText.implicitWidth + 24
    implicitHeight: 34
    radius: 8

    color: active
           ? activeBgColor
           : (mouseArea.containsMouse ? hoverBgColor : inactiveBgColor)

    border.color: active
                  ? activeBorderColor
                  : (mouseArea.containsMouse ? hoverBorderColor : inactiveBorderColor)
    border.width: 1

    Text {
        id: labelText
        anchors.centerIn: parent
        anchors.margins: 4
        text: filterButtonRoot.text
        font.bold: filterButtonRoot.active
        font.pixelSize: 13
        color: filterButtonRoot.active
               ? filterButtonRoot.activeTextColor
               : (mouseArea.containsMouse ? filterButtonRoot.hoverTextColor : filterButtonRoot.inactiveTextColor)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        maximumLineCount: 1
    }

    objectName: "FORENSIC_SEGMENTED_" + text
    property string forensicMarker: "SEGMENTED_FILTER_BUTTON_RUNTIME_V9"

    Component.onCompleted: {
        console.log("================================================")
        console.log(
            "[SEGMENTED_FORENSIC_V9]",
            "text=" + text,
            "active=" + active,
            "rootColor=" + filterButtonRoot.color,
            "textColor=" + labelText.color,
            "qml=" + Qt.resolvedUrl(".")
        )
        console.log("================================================")
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: filterButtonRoot.clicked()
    }
}
