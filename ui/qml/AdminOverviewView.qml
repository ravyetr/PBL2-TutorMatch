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

    function getPrimarySchedule(item) {
        if (!item) return ""
        if (item.slotText) return item.slotText
        var label = item.slotLabel || ""
        var idx = label.indexOf(" (")
        if (idx !== -1) {
            return label.substring(0, idx)
        }
        return label
    }

    function getSecondarySchedule(item) {
        if (!item) return ""
        if (item.weeksCount && item.startDate) {
            return "Khóa " + item.weeksCount + " tuần · Bắt đầu: " + item.startDate
        }
        var label = item.slotLabel || ""
        var idx = label.indexOf(" (")
        if (idx !== -1) {
            var sub = label.substring(idx + 2)
            if (sub.endsWith(")")) sub = sub.substring(0, sub.length - 1)
            return sub.replace(", từ ", " · Bắt đầu: ").replace("từ ", "Bắt đầu: ")
        }
        if (item.startDate) {
            return "Bắt đầu: " + item.startDate
        }
        return ""
    }

    function reloadData() {
        if (typeof guiBridge !== "undefined") {
            platformStats = guiBridge.qmlGetAdminPlatformStats()
            recentConnections = guiBridge.qmlGetRecentConnections(6)
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
                        text: "Theo dõi chỉ số thực tế, kết nối lớp học & hoạt động hệ thống"
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

                // Card 4: Khóa học hoàn thành
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
                            Text { text: "✓"; font.pixelSize: 16; font.bold: true; color: "#52c41a" }
                            Text { text: "Khóa học hoàn thành"; font.pixelSize: 12; color: Theme.textDark; font.bold: true }
                        }
                        Text {
                            text: (adminOverviewRoot.platformStats.completedMatches || adminOverviewRoot.platformStats.completedBookings || 0).toString()
                            font.bold: true
                            font.pixelSize: 26
                            color: "#52c41a"
                        }
                    }
                }
            }

            // 3. Section: Recent Connections
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 430
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
                            spacing: 6
                            boundsBehavior: Flickable.StopAtBounds
                            model: adminOverviewRoot.recentConnections

                            ScrollBar.vertical: ScrollBar {
                                id: connScrollBar
                                policy: ScrollBar.AsNeeded
                            }

                            delegate: Rectangle {
                                width: connectionsListView.width
                                height: 56
                                color: Theme.bgApp
                                radius: 6
                                border.color: Theme.cardBorder
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 12

                                    // Region A: Student ↔ Tutor Identity & Course Subject
                                    ColumnLayout {
                                        id: regionA
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 160
                                        Layout.preferredWidth: 220
                                        Layout.maximumWidth: 280
                                        Layout.alignment: Qt.AlignVCenter
                                        spacing: 2

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: (modelData.studentName || "Học viên") + " ↔ " + (modelData.tutorName || "Gia sư")
                                            font.bold: true
                                            font.pixelSize: 12
                                            color: Theme.textDark
                                            elide: Text.ElideRight
                                            maximumLineCount: 1
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: "📚 " + (modelData.subject || "Chưa cập nhật môn")
                                            font.pixelSize: 11
                                            color: Theme.textMuted
                                            elide: Text.ElideRight
                                            maximumLineCount: 1
                                        }
                                    }

                                    // Region B: Schedule Information (Two-line layout)
                                    ColumnLayout {
                                        id: regionB
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 160
                                        Layout.preferredWidth: 230
                                        Layout.maximumWidth: 320
                                        Layout.alignment: Qt.AlignVCenter
                                        spacing: 2

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: "⏰ " + adminOverviewRoot.getPrimarySchedule(modelData)
                                            font.bold: true
                                            font.pixelSize: 11
                                            color: Theme.textDark
                                            elide: Text.ElideRight
                                            maximumLineCount: 1
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: "🗓️ " + adminOverviewRoot.getSecondarySchedule(modelData)
                                            visible: text.length > 4
                                            font.pixelSize: 10
                                            color: Theme.textMuted
                                            elide: Text.ElideRight
                                            maximumLineCount: 1
                                        }
                                    }

                                    // Region C: Connection Status Badge
                                    Rectangle {
                                        id: statusBadge
                                        Layout.preferredWidth: 96
                                        Layout.minimumWidth: 90
                                        Layout.maximumWidth: 110
                                        height: 24
                                        radius: 4
                                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                        color: (modelData.status === "ACCEPTED") ? "#e6f7ff" :
                                               (modelData.status === "COMPLETED") ? "#f6ffed" :
                                               (modelData.status === "PENDING") ? "#fffbe6" :
                                               (modelData.status === "EARLY_TERMINATED") ? "#fff7e6" : "#fff1f0"
                                        border.color: (modelData.status === "ACCEPTED") ? "#91d5ff" :
                                                      (modelData.status === "COMPLETED") ? "#b7eb8f" :
                                                      (modelData.status === "PENDING") ? "#ffe58f" :
                                                      (modelData.status === "EARLY_TERMINATED") ? "#ffa940" : "#ffa39e"
                                        border.width: 1

                                        Text {
                                            anchors.centerIn: parent
                                            anchors.leftMargin: 4
                                            anchors.rightMargin: 4
                                            text: (modelData.status === "EARLY_TERMINATED") ? "KẾT THÚC SỚM" :
                                                  (modelData.status === "ACCEPTED") ? "ĐANG HỌC" :
                                                  (modelData.status === "COMPLETED") ? "HOÀN THÀNH" :
                                                  (modelData.status === "PENDING") ? "CHỜ DUYỆT" :
                                                  (modelData.status === "CANCELLED") ? "ĐÃ HỦY" :
                                                  (modelData.status === "REJECTED") ? "TỪ CHỐI" :
                                                  (modelData.statusText || modelData.status)
                                            font.pixelSize: 10
                                            font.bold: true
                                            color: (modelData.status === "ACCEPTED") ? "#096dd9" :
                                                   (modelData.status === "COMPLETED") ? "#389e0d" :
                                                   (modelData.status === "PENDING") ? "#d48806" :
                                                   (modelData.status === "EARLY_TERMINATED") ? "#d46b08" : "#cf1322"
                                            elide: Text.ElideRight
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                anchors.centerIn: parent
                                visible: !adminOverviewRoot.recentConnections || adminOverviewRoot.recentConnections.length === 0
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
            }
        }
    }
