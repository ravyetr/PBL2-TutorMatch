import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TutorMatch

Rectangle {
    id: adminOverviewRoot
    color: Theme.cardBg
    radius: 10
    border.color: Theme.cardBorder
    border.width: 1

    property var platformStats: ({})
    property var recentConnections: []
    property var pendingTutors: []

    function reloadData() {
        if (typeof guiBridge !== "undefined") {
            platformStats = guiBridge.qmlGetAdminPlatformStats()
            recentConnections = guiBridge.qmlGetRecentConnections(6)
            pendingTutors = guiBridge.qmlGetPendingVerificationTutors()
        }
    }

    Component.onCompleted: {
        reloadData()
    }

    Connections {
        target: (typeof guiBridge !== "undefined") ? guiBridge : null
        ignoreUnknownSignals: true
        function onDataChanged() { reloadData() }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 20

            // 1. Title Header & Reload Button
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ColumnLayout {
                    spacing: 4
                    Text {
                        text: "TỔNG QUAN NỀN TẢNG QUẢN TRỊ TUTORMATCH"
                        font.bold: true
                        font.pixelSize: 18
                        color: Theme.textDark
                    }
                    Text {
                        text: "Theo dõi chỉ số thực tế, kết nối lớp học & duyệt xác minh hồ sơ gia sư"
                        font.pixelSize: 12
                        color: Theme.textMuted
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Làm mới"
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        font.pixelSize: 12
                        color: Theme.textLight
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 100
                        implicitHeight: 34
                        radius: 6
                        color: parent.hovered ? Theme.cSilverLakeBlue : Theme.sidebarBg
                    }
                    onClicked: reloadData()
                }
            }

            // Status Banner Notification
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
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12

                    Text {
                        text: "Thông báo: " + ((typeof guiBridge !== "undefined") ? guiBridge.statusMessage : "")
                        font.pixelSize: 12
                        font.bold: true
                        color: Theme.textDark
                    }
                }
            }

            // 2. Metric Cards Grid (4 Cards)
            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                // Card 1: Học viên
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    radius: 8
                    color: Theme.bgApp
                    border.color: Theme.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 4

                        RowLayout {
                            spacing: 8
                            Text { text: "👤"; font.pixelSize: 16 }
                            Text { text: "Tổng học viên"; font.pixelSize: 12; color: Theme.textMuted; font.bold: true }
                        }
                        Text {
                            text: (adminOverviewRoot.platformStats.studentCount || 0).toString()
                            font.bold: true
                            font.pixelSize: 26
                            color: Theme.textDark
                        }
                    }
                }

                // Card 2: Gia sư
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    radius: 8
                    color: Theme.bgApp
                    border.color: Theme.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 4

                        RowLayout {
                            spacing: 8
                            Text { text: "🎓"; font.pixelSize: 16 }
                            Text { text: "Tổng gia sư"; font.pixelSize: 12; color: Theme.textMuted; font.bold: true }
                        }
                        Text {
                            text: (adminOverviewRoot.platformStats.tutorCount || 0).toString()
                            font.bold: true
                            font.pixelSize: 26
                            color: Theme.textDark
                        }
                    }
                }

                // Card 3: Đang kết nối
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    radius: 8
                    color: Theme.bgApp
                    border.color: Theme.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 4

                        RowLayout {
                            spacing: 8
                            Text { text: "🔗"; font.pixelSize: 16 }
                            Text { text: "Đang kết nối (ACCEPTED)"; font.pixelSize: 12; color: Theme.textMuted; font.bold: true }
                        }
                        Text {
                            text: (adminOverviewRoot.platformStats.activeMatches || 0).toString()
                            font.bold: true
                            font.pixelSize: 26
                            color: "#1890ff"
                        }
                    }
                }

                // Card 4: Chờ xác minh
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    radius: 8
                    color: (adminOverviewRoot.platformStats.pendingVerification > 0) ? Theme.accentBlue : Theme.bgApp
                    border.color: Theme.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 4

                        RowLayout {
                            spacing: 8
                            Text { text: "✓"; font.pixelSize: 16; font.bold: true }
                            Text { text: "Gia sư chờ xác minh"; font.pixelSize: 12; color: Theme.textDark; font.bold: true }
                        }
                        Text {
                            text: (adminOverviewRoot.platformStats.pendingVerification || 0).toString()
                            font.bold: true
                            font.pixelSize: 26
                            color: (adminOverviewRoot.platformStats.pendingVerification > 0) ? "#cf1322" : Theme.textDark
                        }
                    }
                }
            }

            // 3. Split Section: Recent Connections & Pending Verifications
            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                // Left Panel: Recent Connections
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 3
                    implicitHeight: 380
                    color: Theme.cardBg
                    radius: 8
                    border.color: Theme.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Text {
                            text: "KẾT NỐI HỌC TẬP GẦN ĐÂY"
                            font.bold: true
                            font.pixelSize: 14
                            color: Theme.textDark
                        }

                        ListView {
                            id: connectionsListView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 8
                            model: adminOverviewRoot.recentConnections

                            delegate: Rectangle {
                                width: connectionsListView.width
                                height: 50
                                color: Theme.bgApp
                                radius: 6
                                border.color: Theme.cardBorder
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 10

                                    // Student -> Tutor
                                    ColumnLayout {
                                        Layout.preferredWidth: 180
                                        spacing: 2
                                        Text {
                                            text: modelData.studentName + " ↔ " + modelData.tutorName
                                            font.bold: true
                                            font.pixelSize: 12
                                            color: Theme.textDark
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: modelData.subject
                                            font.pixelSize: 11
                                            color: Theme.textMuted
                                            elide: Text.ElideRight
                                        }
                                    }

                                    // Schedule slot
                                    Text {
                                        text: modelData.slotLabel || ""
                                        font.pixelSize: 11
                                        color: Theme.textDark
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }

                                    // Status Badge
                                    Rectangle {
                                        Layout.preferredWidth: 100
                                        height: 24
                                        radius: 4
                                        color: (modelData.status === "ACCEPTED") ? "#e6f7ff" :
                                               (modelData.status === "COMPLETED") ? "#f6ffed" :
                                               (modelData.status === "PENDING") ? "#fffbe6" : "#fff1f0"
                                        border.color: (modelData.status === "ACCEPTED") ? "#91d5ff" :
                                                      (modelData.status === "COMPLETED") ? "#b7eb8f" :
                                                      (modelData.status === "PENDING") ? "#ffe58f" : "#ffa39e"
                                        border.width: 1

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.statusText || modelData.status
                                            font.pixelSize: 10
                                            font.bold: true
                                            color: (modelData.status === "ACCEPTED") ? "#096dd9" :
                                                   (modelData.status === "COMPLETED") ? "#389e0d" :
                                                   (modelData.status === "PENDING") ? "#d48806" : "#cf1322"
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                visible: adminOverviewRoot.recentConnections.length === 0
                                color: "transparent"
                                Text {
                                    anchors.centerIn: parent
                                    text: "Chưa có kết nối học tập nào"
                                    font.pixelSize: 13
                                    color: Theme.textMuted
                                }
                            }
                        }
                    }
                }

                // Right Panel: Pending Tutor Verifications
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 2
                    implicitHeight: 380
                    color: Theme.cardBg
                    radius: 8
                    border.color: Theme.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        Text {
                            text: "GIA SƯ CHỜ XÁC MINH HỒ SƠ"
                            font.bold: true
                            font.pixelSize: 14
                            color: Theme.textDark
                        }

                        ListView {
                            id: pendingTutorsListView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 8
                            model: adminOverviewRoot.pendingTutors

                            delegate: Rectangle {
                                width: pendingTutorsListView.width
                                height: 50
                                color: Theme.bgApp
                                radius: 6
                                border.color: Theme.cardBorder
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 10

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Text {
                                            text: modelData.fullName
                                            font.bold: true
                                            font.pixelSize: 12
                                            color: Theme.textDark
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            text: modelData.subject || "Chưa cập nhật chuyên môn"
                                            font.pixelSize: 11
                                            color: Theme.textMuted
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Button {
                                        text: "Xác minh"
                                        contentItem: Text {
                                            text: parent.text
                                            font.pixelSize: 11
                                            font.bold: true
                                            color: Theme.btnText
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                        }
                                        background: Rectangle {
                                            implicitWidth: 80
                                            implicitHeight: 28
                                            radius: 4
                                            color: parent.hovered ? "#b8aba0" : Theme.btnPrimary
                                        }
                                        onClicked: {
                                            if (typeof guiBridge !== "undefined") {
                                                guiBridge.qmlVerifyTutor(modelData.id, true)
                                                adminOverviewRoot.reloadData()
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                visible: adminOverviewRoot.pendingTutors.length === 0
                                color: "transparent"
                                Text {
                                    anchors.centerIn: parent
                                    text: "Tất cả gia sư đều đã được xác minh"
                                    font.pixelSize: 13
                                    color: Theme.textMuted
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
