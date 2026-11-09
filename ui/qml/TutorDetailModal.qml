import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: tutorDetailModal
    anchors.fill: parent
    visible: false
    z: 999

    property var tutorDetail: ({})
    property var availableSlotMap: ({})

    signal bookCourseRequested(var tutor)

    function openWithTutorId(tutorId) {
        if (typeof guiBridge !== "undefined") {
            tutorDetail = guiBridge.getTutorDetail(tutorId)
        }
        rebuildSlotMap()
        tutorDetailModal.visible = true
    }

    function rebuildSlotMap() {
        var slots = (tutorDetail && tutorDetail.availableSlots) ? tutorDetail.availableSlots : []
        var map = {}
        for (var i = 0; i < slots.length; ++i) {
            var s = slots[i]
            map[s.dayIndex + ":" + s.slotIndex] = true
        }
        availableSlotMap = map
    }

    function isSlotAvailable(dayIdx, slotIdx) {
        return availableSlotMap[dayIdx + ":" + slotIdx] === true
    }

    // Modal Background Overlay Dimming
    Rectangle {
        anchors.fill: parent
        color: "#80000000"
        MouseArea {
            anchors.fill: parent
            onClicked: {} // Prevent click-through
        }
    }

    // Centered Modal Window Card
    Rectangle {
        width: 620
        height: 640
        anchors.centerIn: parent
        radius: 12
        color: Theme.cardBg
        border.color: Theme.cardBorder
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            // Header (Fixed Top)
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "THÔNG TIN CHI TIẾT GIA SƯ"
                    font.bold: true
                    font.pixelSize: 18
                    color: Theme.textDark
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: "✕"
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 16
                        color: Theme.textMuted
                    }
                    background: Rectangle { color: "transparent" }
                    onClicked: tutorDetailModal.visible = false
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.cardBorder
            }

            // Scrollable Content Area
            Flickable {
                id: contentFlickable
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: mainColumn.implicitHeight

                ColumnLayout {
                    id: mainColumn
                    width: contentFlickable.width - 12
                    spacing: 16

                    // Section 1: Profile Summary Card
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: profileRow.implicitHeight + 20
                        color: Theme.bgApp
                        radius: 8
                        border.color: Theme.cardBorder
                        border.width: 1

                        RowLayout {
                            id: profileRow
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 14

                            Rectangle {
                                width: 52
                                height: 52
                                radius: 26
                                color: Theme.sidebarBg
                                Layout.alignment: Qt.AlignVCenter
                                Text {
                                    anchors.centerIn: parent
                                    text: (tutorDetail.fullName && tutorDetail.fullName.length > 0) ? tutorDetail.fullName.substring(0, 1).toUpperCase() : "G"
                                    font.bold: true
                                    font.pixelSize: 22
                                    color: Theme.textLight
                                }
                            }

                            ColumnLayout {
                                spacing: 4
                                Layout.fillWidth: true

                                RowLayout {
                                    spacing: 8
                                    Text {
                                        text: tutorDetail.fullName || "Gia sư"
                                        font.bold: true
                                        font.pixelSize: 16
                                        color: Theme.textDark
                                        elide: Text.ElideRight
                                    }
                                    Rectangle {
                                        visible: tutorDetail.isVerified === true
                                        width: 76
                                        height: 20
                                        radius: 4
                                        color: "#132719"
                                        border.color: "#52c41a"
                                        border.width: 1
                                        Text {
                                            anchors.centerIn: parent
                                            text: "Đã xác minh"
                                            font.pixelSize: 10
                                            font.bold: true
                                            color: "#52c41a"
                                        }
                                    }
                                }

                                Text {
                                    text: "Chuyên môn: " + (tutorDetail.subject || "N/A") + "  |  Học phí: " + ((tutorDetail.hourlyRate || 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ / buổi")
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#d97706"
                                    elide: Text.ElideRight
                                }

                                RowLayout {
                                    spacing: 6
                                    Text {
                                        text: (tutorDetail.rating || 5.0).toFixed(1) + " ★"
                                        font.bold: true
                                        font.pixelSize: 12
                                        color: "#d97706"
                                    }
                                    Text {
                                        text: "(" + (tutorDetail.totalReviews || 0) + " lượt đánh giá từ học viên)"
                                        font.pixelSize: 11
                                        color: Theme.textMuted
                                    }
                                }
                            }
                        }
                    }

                    // Section 2: Bio / Introduction
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: "Giới thiệu bản thân & kinh nghiệm:"
                            font.bold: true
                            font.pixelSize: 13
                            color: Theme.textDark
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: bioText.implicitHeight + 20
                            color: Theme.bgApp
                            radius: 6
                            border.color: Theme.cardBorder
                            border.width: 1

                            Text {
                                id: bioText
                                anchors.fill: parent
                                anchors.margins: 10
                                text: tutorDetail.bio || "Chưa có thông tin giới thiệu."
                                font.pixelSize: 12
                                color: Theme.textDark
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    // Section 3: Mini Visual Timetable 7×8 Section
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: "Khung thời gian rảnh hàng tuần (7 ngày × 8 ca):"
                            font.bold: true
                            font.pixelSize: 13
                            color: Theme.textDark
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: timetableColumn.implicitHeight + 24
                            color: Theme.cardBg
                            radius: 8
                            border.color: Theme.cardBorder
                            border.width: 1

                            ColumnLayout {
                                id: timetableColumn
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 6

                                // Header Row: Time Label + 7 Days
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 4

                                    // Time column spacer header
                                    Rectangle {
                                        Layout.preferredWidth: 64
                                        height: 24
                                        color: "transparent"
                                        Text {
                                            anchors.centerIn: parent
                                            text: "Giờ \\ Ngày"
                                            font.pixelSize: 10
                                            font.bold: true
                                            color: Theme.textMuted
                                        }
                                    }

                                    // 7 Day Headers
                                    Repeater {
                                        model: ["T2", "T3", "T4", "T5", "T6", "T7", "CN"]
                                        Rectangle {
                                            Layout.fillWidth: true
                                            height: 24
                                            color: Theme.sidebarBg
                                            radius: 4

                                            Text {
                                                anchors.centerIn: parent
                                                text: modelData
                                                font.bold: true
                                                font.pixelSize: 11
                                                color: Theme.textLight
                                            }
                                        }
                                    }
                                }

                                // 8 Time Slot Rows
                                Repeater {
                                    model: [
                                        "07:30", "09:15", "13:30", "15:15",
                                        "17:00", "18:45", "20:30", "22:00"
                                    ]

                                    RowLayout {
                                        property int slotIdx: index
                                        Layout.fillWidth: true
                                        spacing: 4

                                        // Slot Time Label
                                        Rectangle {
                                            Layout.preferredWidth: 64
                                            height: 26
                                            color: Theme.bgApp
                                            radius: 4
                                            border.color: Theme.cardBorder
                                            border.width: 1

                                            Text {
                                                anchors.centerIn: parent
                                                text: modelData
                                                font.pixelSize: 10
                                                font.bold: true
                                                color: Theme.textDark
                                            }
                                        }

                                        // 7 Day Cells for this slot
                                        Repeater {
                                            model: 7

                                            Rectangle {
                                                property int dayIdx: index
                                                property bool isAvailable: tutorDetailModal.isSlotAvailable(dayIdx, slotIdx)

                                                Layout.fillWidth: true
                                                height: 26
                                                radius: 4
                                                color: isAvailable ? Theme.sidebarActive : Theme.bgApp
                                                border.color: isAvailable ? Theme.sidebarBg : Theme.cardBorder
                                                border.width: 1
                                                opacity: isAvailable ? 1.0 : 0.55

                                                Text {
                                                    anchors.centerIn: parent
                                                    text: isAvailable ? "✓" : "—"
                                                    font.pixelSize: isAvailable ? 11 : 9
                                                    font.bold: isAvailable
                                                    color: isAvailable ? Theme.textDark : Theme.textMuted
                                                }
                                            }
                                        }
                                    }
                                }

                                // Timetable Legend & Status Footer
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 12
                                    Layout.topMargin: 4

                                    RowLayout {
                                        spacing: 4
                                        Rectangle {
                                            width: 14; height: 14; radius: 3
                                            color: Theme.sidebarActive
                                            border.color: Theme.sidebarBg
                                        }
                                        Text {
                                            text: "Có thể đăng ký"
                                            font.pixelSize: 11
                                            font.bold: true
                                            color: Theme.textDark
                                        }
                                    }

                                    RowLayout {
                                        spacing: 4
                                        Rectangle {
                                            width: 14; height: 14; radius: 3
                                            color: Theme.bgApp
                                            border.color: Theme.cardBorder
                                            opacity: 0.55
                                        }
                                        Text {
                                            text: "Không có lịch rảnh"
                                            font.pixelSize: 11
                                            color: Theme.textMuted
                                        }
                                    }

                                    Item { Layout.fillWidth: true }

                                    Text {
                                        text: ((tutorDetail.availableSlots || []).length > 0) ?
                                              ("Tổng số: " + (tutorDetail.availableSlots || []).length + " ca rảnh/tuần") :
                                              "Gia sư chưa cập nhật lịch rảnh"
                                        font.pixelSize: 11
                                        font.bold: true
                                        color: "#d97706"
                                    }
                                }
                            }
                        }
                    }

                    // Section 4: Reviews List Section (Directly after Timetable, no overlap!)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: "Lịch sử đánh giá từ học viên (" + ((tutorDetail.reviews || []).length) + " lượt đánh giá - " + (tutorDetail.rating || 5.0).toFixed(1) + " ★):"
                            font.bold: true
                            font.pixelSize: 13
                            color: Theme.textDark
                        }

                        // Empty State for Reviews
                        Rectangle {
                            visible: (tutorDetail.reviews || []).length === 0
                            Layout.fillWidth: true
                            implicitHeight: 48
                            color: Theme.bgApp
                            radius: 6
                            border.color: Theme.cardBorder
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                text: "💬 Gia sư chưa có lượt đánh giá nào từ học viên."
                                font.pixelSize: 12
                                color: Theme.textMuted
                            }
                        }

                        // Reviews Repeater
                        Repeater {
                            model: tutorDetail.reviews || []

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: reviewItemCol.implicitHeight + 16
                                color: Theme.bgApp
                                radius: 6
                                border.color: Theme.cardBorder
                                border.width: 1

                                ColumnLayout {
                                    id: reviewItemCol
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Text {
                                            text: modelData.studentName || "Học viên"
                                            font.bold: true
                                            font.pixelSize: 12
                                            color: Theme.textDark
                                        }

                                        Text {
                                            text: (modelData.rating || 5) + " ★"
                                            font.bold: true
                                            font.pixelSize: 12
                                            color: "#d97706"
                                        }

                                        Item { Layout.fillWidth: true }

                                        Text {
                                            text: modelData.createdAt || ""
                                            font.pixelSize: 10
                                            color: Theme.textMuted
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.comment || "Không có nhận xét."
                                        font.pixelSize: 11
                                        color: Theme.textDark
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.cardBorder
            }

            // Footer Actions (Fixed Bottom)
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Button {
                    text: "Đóng"
                    Layout.fillWidth: true
                    height: 40
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        color: Theme.textDark
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.bgApp : Theme.accentBlue
                        radius: 6
                        border.color: Theme.cardBorder
                    }
                    onClicked: tutorDetailModal.visible = false
                }

                Button {
                    text: "Đăng ký học ngay"
                    Layout.fillWidth: true
                    height: 40
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        color: Theme.btnText
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.sidebarBg : Theme.btnPrimary
                        radius: 6
                    }
                    onClicked: {
                        tutorDetailModal.visible = false
                        tutorDetailModal.bookCourseRequested(tutorDetailModal.tutorDetail)
                    }
                }
            }
        }
    }
}
