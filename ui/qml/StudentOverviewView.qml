import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: studentOverviewView

    ScrollView {
        id: overviewScrollView
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: overviewScrollView.availableWidth
            spacing: 16

            // Status Notification Banner
            Rectangle {
                Layout.fillWidth: true
                height: 36
                radius: 6
                color: Theme.bgApp
                border.color: Theme.accentBlue
                border.width: 1
                visible: (typeof guiBridge !== "undefined" && guiBridge.statusMessage !== "")

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16

                    Text {
                        text: "Thông báo: " + ((typeof guiBridge !== "undefined") ? guiBridge.statusMessage : "")
                        font.pixelSize: 12
                        font.bold: true
                        color: Theme.textDark
                    }
                }
            }

            // Student Profile Header Card
            StudentProfileCard {
                id: profileCard
                Layout.fillWidth: true
                Layout.preferredHeight: 180

                onEditRequested: {
                    editModal.openWithProfile(profileCard.profile)
                }
            }

            // Weekly Timetable 56-Cell Grid
            TimetableGrid {
                id: timetable
                Layout.fillWidth: true
                Layout.preferredHeight: 480
                Layout.minimumHeight: 440
            }
        }
    }

    // Modal Edit Dialog Component Overlay
    ProfileEditModal {
        id: editModal
    }
}
