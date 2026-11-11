import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: studentProfileCard
    color: Theme.cardBg
    radius: 12
    border.color: Theme.cardBorder
    border.width: 1

    signal editRequested()

    property var profile: (typeof guiBridge !== "undefined" && guiBridge.currentStudentProfile) ? guiBridge.currentStudentProfile : ({})

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        // Top Header Row
        RowLayout {
            Layout.fillWidth: true

            // Avatar & Name Header
            RowLayout {
                spacing: 12
                Rectangle {
                    width: 48
                    height: 48
                    radius: 24
                    color: Theme.sidebarBg
                    Text {
                        anchors.centerIn: parent
                        text: (studentProfileCard.profile.fullName && studentProfileCard.profile.fullName.length > 0) ? studentProfileCard.profile.fullName.substring(0, 1).toUpperCase() : "S"
                        font.bold: true
                        font.pixelSize: 20
                        color: Theme.textLight
                    }
                }

                ColumnLayout {
                    spacing: 2
                    Text {
                        text: studentProfileCard.profile.fullName || "Học Viên"
                        font.bold: true
                        font.pixelSize: 18
                        color: Theme.textDark
                    }
                    Text {
                        text: "Học viên  |  ID: " + (studentProfileCard.profile.id || 0)
                        font.pixelSize: 12
                        color: Theme.textMuted
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // Account Status Badge
            Rectangle {
                width: 120
                height: 30
                radius: 6
                color: (studentProfileCard.profile.isActive !== false) ? "#e6f7ff" : "#fff1f0"
                border.color: (studentProfileCard.profile.isActive !== false) ? Theme.sidebarActive : "#ff4d4f"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: (studentProfileCard.profile.isActive !== false) ? "HOẠT ĐỘNG" : "ĐÃ BỊ KHÓA"
                    font.bold: true
                    font.pixelSize: 11
                    color: (studentProfileCard.profile.isActive !== false) ? Theme.textDark : "#cf1322"
                }
            }

            // Edit Profile Button
            Button {
                text: "Chỉnh sửa hồ sơ"
                contentItem: Text {
                    text: parent.text
                    font.bold: true
                    font.pixelSize: 12
                    color: Theme.btnText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitWidth: 130
                    implicitHeight: 34
                    radius: 6
                    color: parent.hovered ? Theme.sidebarBg : Theme.btnPrimary
                }
                onClicked: studentProfileCard.editRequested()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.cardBorder
        }

        // Stable Profile Details Grid (Search-specific criteria removed)
        GridLayout {
            columns: 4
            rowSpacing: 12
            columnSpacing: 24
            Layout.fillWidth: true

            // Email
            ColumnLayout {
                spacing: 2
                Text { text: "Địa chỉ Email:"; font.pixelSize: 11; color: Theme.textMuted }
                Text { text: studentProfileCard.profile.email || "N/A"; font.bold: true; font.pixelSize: 13; color: Theme.textDark; elide: Text.ElideRight }
            }

            // Grade Level
            ColumnLayout {
                spacing: 2
                Text { text: "Khối Lớp Đang Học:"; font.pixelSize: 11; color: Theme.textMuted }
                Text { text: (studentProfileCard.profile.gradeLevel === 0 ? "Khác" : "Lớp " + (studentProfileCard.profile.gradeLevel || 10)); font.bold: true; font.pixelSize: 13; color: Theme.textDark }
            }

            // Username
            ColumnLayout {
                spacing: 2
                Text { text: "Tên Tài Khoản:"; font.pixelSize: 11; color: Theme.textMuted }
                Text { text: "@" + (studentProfileCard.profile.username || "student"); font.bold: true; font.pixelSize: 13; color: Theme.textDark }
            }

            // Platform Role & Integrity
            ColumnLayout {
                spacing: 2
                Text { text: "Loại Tài Khoản:"; font.pixelSize: 11; color: Theme.textMuted }
                Text { text: "Học viên nền tảng"; font.bold: true; font.pixelSize: 13; color: "#2b6cb0" }
            }
        }
    }
}
