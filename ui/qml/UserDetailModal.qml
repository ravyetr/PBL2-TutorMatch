import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TutorMatch

Rectangle {
    id: userDetailModal
    anchors.fill: parent
    color: "#80000000"
    visible: false
    z: 100

    property int userId: 0
    property var userDetail: ({})
    property int activeTab: 0

    function openForUser(targetUserId) {
        userId = targetUserId
        activeTab = 0
        if (typeof guiBridge !== "undefined") {
            userDetail = guiBridge.qmlGetUserDetailsForAdmin(targetUserId)
        } else {
            userDetail = {}
        }
        visible = true
    }

    function closeModal() {
        visible = false
        userId = 0
        userDetail = {}
    }

    // Traps clicks on backdrop
    MouseArea {
        anchors.fill: parent
        onClicked: userDetailModal.closeModal()
    }

    // Modal Card Dialog
    Rectangle {
        width: Math.min(860, parent.width - 40)
        height: Math.min(680, parent.height - 40)
        anchors.centerIn: parent
        color: Theme.cardBg
        radius: 12
        border.color: Theme.cardBorder
        border.width: 1

        // Trap clicks inside dialog content
        MouseArea {
            anchors.fill: parent
            onClicked: {}
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 14

            // 1. Header with User Summary & Close Button
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                // User Avatar Circle
                Rectangle {
                    width: 44
                    height: 44
                    radius: 22
                    color: (userDetail.found !== false && userDetail.profile && (userDetail.profile.roleKey === "ADMIN" || userDetail.profile.role === "Admin")) ? Theme.sidebarBg :
                           (userDetail.found !== false && userDetail.profile && userDetail.profile.roleKey === "TUTOR") ? Theme.sidebarActive : Theme.accentBlue

                    Text {
                        anchors.centerIn: parent
                        text: (userDetail.found !== false && userDetail.profile && userDetail.profile.fullName && userDetail.profile.fullName !== "") ? userDetail.profile.fullName.substring(0, 1).toUpperCase() : (userDetail.found === false ? "?" : "U")
                        font.bold: true
                        font.pixelSize: 18
                        color: (userDetail.found !== false && userDetail.profile && (userDetail.profile.roleKey === "ADMIN" || userDetail.profile.role === "Admin")) ? Theme.textLight : Theme.textDark
                    }
                }

                // Name & Role Header
                ColumnLayout {
                    spacing: 2
                    RowLayout {
                        spacing: 8
                        Text {
                            text: (userDetail && userDetail.found === false) ? "Không tìm thấy người dùng" :
                                  (userDetail.profile && userDetail.profile.fullName) ? userDetail.profile.fullName : "Hồ sơ chi tiết"
                            font.bold: true
                            font.pixelSize: 16
                            color: (userDetail && userDetail.found === false) ? "#cf1322" : Theme.textDark
                        }

                        // Role Badge
                        Rectangle {
                            visible: (userDetail && userDetail.found !== false && userDetail.profile && userDetail.profile.role)
                            height: 20
                            implicitWidth: roleLabel.implicitWidth + 12
                            radius: 4
                            color: (userDetail.profile && userDetail.profile.roleKey === "TUTOR") ? Theme.sidebarActive : Theme.bgApp
                            border.color: Theme.cardBorder
                            border.width: 1

                            Text {
                                id: roleLabel
                                anchors.centerIn: parent
                                text: (userDetail.profile && userDetail.profile.role) ? userDetail.profile.role : ""
                                font.pixelSize: 10
                                font.bold: true
                                color: Theme.textDark
                            }
                        }

                        // Active / Locked Badge (Never displayed if user not found)
                        Rectangle {
                            visible: (userDetail && userDetail.found !== false && userDetail.profile && userDetail.profile.isActive !== undefined)
                            height: 20
                            implicitWidth: activeLabel.implicitWidth + 12
                            radius: 4
                            color: (userDetail.profile && userDetail.profile.isActive) ? "#e6f7ff" : "#fff1f0"
                            border.color: (userDetail.profile && userDetail.profile.isActive) ? "#91d5ff" : "#ffa39e"
                            border.width: 1

                            Text {
                                id: activeLabel
                                anchors.centerIn: parent
                                text: (userDetail.profile && userDetail.profile.isActive) ? "HOẠT ĐỘNG" : "ĐÃ BỊ KHÓA"
                                font.pixelSize: 10
                                font.bold: true
                                color: (userDetail.profile && userDetail.profile.isActive) ? "#096dd9" : "#cf1322"
                            }
                        }

                        // Not Found / Error Badge
                        Rectangle {
                            visible: (userDetail && userDetail.found === false)
                            height: 20
                            implicitWidth: errorLabel.implicitWidth + 12
                            radius: 4
                            color: "#fff1f0"
                            border.color: "#ffa39e"
                            border.width: 1

                            Text {
                                id: errorLabel
                                anchors.centerIn: parent
                                text: "KHÔNG TỒN TẠI"
                                font.pixelSize: 10
                                font.bold: true
                                color: "#cf1322"
                            }
                        }
                    }

                    Text {
                        text: (userDetail && userDetail.found === false) ? ("Mã ID truy vấn: #" + userDetailModal.userId) :
                              (userDetail.profile && userDetail.profile.email) ? (userDetail.profile.email + (userDetail.profile.username ? " (@" + userDetail.profile.username + ")" : "")) : ""
                        font.pixelSize: 12
                        color: Theme.textMuted
                    }
                }

                Item { Layout.fillWidth: true }

                // Close Button [X]
                Button {
                    text: "✕"
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        font.pixelSize: 14
                        color: Theme.textDark
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        implicitWidth: 32
                        implicitHeight: 32
                        radius: 16
                        color: parent.hovered ? Theme.bgApp : "transparent"
                        border.color: Theme.cardBorder
                        border.width: 1
                    }
                    onClicked: userDetailModal.closeModal()
                }
            }

            // Separator Line
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.cardBorder
            }

            // 2. Navigation Tabs (4 options) - Hidden when user not found
            Rectangle {
                Layout.fillWidth: true
                height: 40
                radius: 6
                color: Theme.bgApp
                border.color: Theme.cardBorder
                border.width: 1
                visible: (userDetail && userDetail.found !== false)

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 3
                    spacing: 4

                    Repeater {
                        model: ["Tổng quan", "Thời khóa biểu", "Lịch sử kết nối", "Đánh giá"]

                        SegmentedFilterButton {
                            text: modelData
                            active: index === userDetailModal.activeTab
                            onClicked: userDetailModal.activeTab = index
                        }
                    }
                }
            }

            // 3. Tab Body Content Views Stack
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // Error / Not Found View (When query fails or user not found)
                Rectangle {
                    anchors.centerIn: parent
                    width: Math.min(520, parent.width - 40)
                    height: 200
                    visible: (userDetail && userDetail.found === false)
                    color: Theme.bgApp
                    radius: 8
                    border.color: "#ffa39e"
                    border.width: 1

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 12
                        Text {
                            text: "⚠️"
                            font.pixelSize: 36
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Text {
                            text: (userDetail && userDetail.errorMessage) ? userDetail.errorMessage : ("Không tìm thấy thông tin người dùng với ID #" + userDetailModal.userId)
                            font.bold: true
                            font.pixelSize: 14
                            color: "#cf1322"
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Text {
                            text: "Người dùng không tồn tại trong hệ thống hoặc đã bị gỡ bỏ."
                            font.pixelSize: 12
                            color: Theme.textMuted
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Button {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Đóng cửa sổ"
                            contentItem: Text {
                                text: parent.text
                                font.bold: true
                                font.pixelSize: 11
                                color: Theme.textDark
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                implicitWidth: 110
                                implicitHeight: 30
                                radius: 4
                                color: parent.hovered ? Theme.bgApp : Theme.accentBlue
                                border.color: Theme.cardBorder
                                border.width: 1
                            }
                            onClicked: userDetailModal.closeModal()
                        }
                    }
                }

                // =========================================================
                // TAB 0: TỔNG QUAN (OVERVIEW)
                // =========================================================
                ScrollView {
                    anchors.fill: parent
                    visible: (userDetail && userDetail.found !== false && userDetailModal.activeTab === 0)
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 14

                        // Account Information Card
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: accColumn.implicitHeight + 24
                            color: Theme.bgApp
                            radius: 8
                            border.color: Theme.cardBorder
                            border.width: 1

                            ColumnLayout {
                                id: accColumn
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 8

                                Text {
                                    text: "THÔNG TIN TÀI KHOẢN"
                                    font.bold: true
                                    font.pixelSize: 13
                                    color: Theme.textDark
                                }

                                GridLayout {
                                    columns: 2
                                    rowSpacing: 6
                                    columnSpacing: 20

                                    Text { text: "Mã người dùng ID:"; font.pixelSize: 12; color: Theme.textMuted }
                                    Text { text: userDetail.profile ? ("#" + userDetail.profile.id) : ""; font.bold: true; font.pixelSize: 12; color: Theme.textDark }

                                    Text { text: "Tên đăng nhập:"; font.pixelSize: 12; color: Theme.textMuted }
                                    Text { text: userDetail.profile ? userDetail.profile.username : ""; font.bold: true; font.pixelSize: 12; color: Theme.textDark }

                                    Text { text: "Họ và tên:"; font.pixelSize: 12; color: Theme.textMuted }
                                    Text { text: userDetail.profile ? userDetail.profile.fullName : ""; font.bold: true; font.pixelSize: 12; color: Theme.textDark }

                                    Text { text: "Địa chỉ Email:"; font.pixelSize: 12; color: Theme.textMuted }
                                    Text { text: userDetail.profile ? userDetail.profile.email : ""; font.bold: true; font.pixelSize: 12; color: Theme.textDark }

                                    Text { text: "Ngày tạo tài khoản:"; font.pixelSize: 12; color: Theme.textMuted }
                                    Text { text: userDetail.profile ? userDetail.profile.createdAt : ""; font.pixelSize: 12; color: Theme.textDark }
                                }
                            }
                        }

                        // Role Profile Specific Card (Student / Tutor)
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: roleColumn.implicitHeight + 24
                            color: Theme.bgApp
                            radius: 8
                            border.color: Theme.cardBorder
                            border.width: 1

                            ColumnLayout {
                                id: roleColumn
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 8

                                Text {
                                    text: (userDetail.profile && userDetail.profile.roleKey === "STUDENT") ? "HỒ SƠ HỌC VIÊN" :
                                          (userDetail.profile && userDetail.profile.roleKey === "ADMIN") ? "HỒ SƠ QUẢN TRỊ VIÊN" : "HỒ SƠ GIA SƯ & GIẢNG DẠY"
                                    font.bold: true
                                    font.pixelSize: 13
                                    color: Theme.textDark
                                }

                                // Admin Specific Profile Fields
                                GridLayout {
                                    visible: (userDetail.profile && userDetail.profile.roleKey === "ADMIN")
                                    columns: 2
                                    rowSpacing: 6
                                    columnSpacing: 20

                                    Text { text: "Cấp độ phân quyền:"; font.pixelSize: 12; color: Theme.textMuted }
                                    Text { text: userDetail.profile ? ("Level " + (userDetail.profile.permissionLevel !== undefined ? userDetail.profile.permissionLevel : 1) + " (Toàn quyền quản trị)") : ""; font.bold: true; font.pixelSize: 12; color: Theme.textDark }

                                    Text { text: "Phạm vi quản lý:"; font.pixelSize: 12; color: Theme.textMuted }
                                    Text { text: "Người dùng, Thời khóa biểu, Kết nối, Khiếu nại & Kiểm duyệt"; font.pixelSize: 12; color: Theme.textDark }
                                }

                                // Student Specific Profile Fields
                                GridLayout {
                                    visible: (userDetail.profile && userDetail.profile.roleKey === "STUDENT")
                                    columns: 2
                                    rowSpacing: 6
                                    columnSpacing: 20

                                    Text { text: "Khối lớp học:"; font.pixelSize: 12; color: Theme.textMuted }
                                    Text { text: userDetail.profile ? ("Lớp " + userDetail.profile.gradeDisplay) : ""; font.bold: true; font.pixelSize: 12; color: Theme.textDark }

                                    Text { text: "Cơ chế ghép gia sư:"; font.pixelSize: 12; color: Theme.textMuted }
                                    Text { text: "Tìm kiếm SmartMatch độc lập theo từng lượt"; font.pixelSize: 12; color: Theme.textDark }
                                }

                                // Tutor Specific Profile Fields
                                GridLayout {
                                    visible: (userDetail.profile && userDetail.profile.roleKey === "TUTOR")
                                    columns: 2
                                    rowSpacing: 6
                                    columnSpacing: 20

                                    Text { text: "Chuyên môn giảng dạy:"; font.pixelSize: 12; color: Theme.textMuted }
                                    Text { text: userDetail.profile ? userDetail.profile.subjectSpecialty : ""; font.bold: true; font.pixelSize: 12; color: Theme.textDark }

                                    Text { text: "Học phí đề xuất:"; font.pixelSize: 12; color: Theme.textMuted }
                                    Text { text: userDetail.profile ? (userDetail.profile.hourlyRate + " đ/giờ") : ""; font.bold: true; font.pixelSize: 12; color: Theme.textDark }

                                    Text { text: "Đánh giá trung bình:"; font.pixelSize: 12; color: Theme.textMuted }
                                    RowLayout {
                                        spacing: 4
                                        Text { text: userDetail.profile ? (userDetail.profile.ratingFormatted + " ★") : ""; font.bold: true; font.pixelSize: 12; color: "#D97706" }
                                        Text { text: userDetail.profile ? ("(" + userDetail.profile.totalReviews + " đánh giá)") : ""; font.pixelSize: 11; color: Theme.textMuted }
                                    }

                                    Text { text: "Giới thiệu bản thân (Bio):"; font.pixelSize: 12; color: Theme.textMuted; Layout.alignment: Qt.AlignTop }
                                    Text {
                                        text: (userDetail.profile && userDetail.profile.bio !== "") ? userDetail.profile.bio : "(Chưa cập nhật giới thiệu)"
                                        font.pixelSize: 12
                                        color: Theme.textDark
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }
                }

                // =========================================================
                // TAB 1: THỜI KHÓA BIỂU (READ-ONLY SCHEDULE)
                // =========================================================
                Item {
                    anchors.fill: parent
                    visible: (userDetail && userDetail.found !== false && userDetailModal.activeTab === 1)

                    TimetableGrid {
                        anchors.fill: parent
                        scheduleData: userDetail ? userDetail.schedule : []
                        readOnly: true
                    }
                }

                // =========================================================
                // TAB 2: LỊCH SỬ KẾT NỐI (CONNECTIONS)
                // =========================================================
                Item {
                    anchors.fill: parent
                    visible: (userDetail && userDetail.found !== false && userDetailModal.activeTab === 2)

                    ListView {
                        id: connListView
                        anchors.fill: parent
                        clip: true
                        spacing: 10
                        model: userDetail ? userDetail.connections : []

                        delegate: Rectangle {
                            width: connListView.width
                            height: 70
                            color: Theme.bgApp
                            radius: 8
                            border.color: Theme.cardBorder
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 12

                                ColumnLayout {
                                    spacing: 2
                                    Text {
                                        text: (userDetail.profile && userDetail.profile.roleKey === "STUDENT") ? ("Gia sư: " + modelData.tutorName) : ("Học viên: " + modelData.studentName)
                                        font.bold: true
                                        font.pixelSize: 13
                                        color: Theme.textDark
                                    }
                                    Text {
                                        text: "Môn: " + modelData.subject + " | Học phí: " + modelData.hourlyRate + " đ/h"
                                        font.pixelSize: 11
                                        color: Theme.textMuted
                                    }
                                    Text {
                                        text: "Lịch dạy: " + modelData.slotFormatted
                                        font.pixelSize: 11
                                        color: Theme.textDark
                                        elide: Text.ElideRight
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                ColumnLayout {
                                    Layout.alignment: Qt.AlignRight
                                    spacing: 4

                                    Rectangle {
                                        implicitWidth: connStatusText.implicitWidth + 14
                                        height: 22
                                        radius: 4
                                        color: (modelData.status === "ACCEPTED") ? "#132719" :
                                               (modelData.status === "COMPLETED") ? "#e6f7ff" :
                                               (modelData.status === "EARLY_TERMINATED") ? "#fff7e6" :
                                               (modelData.status === "CANCELLED" || modelData.status === "REJECTED") ? "#fff1f0" : "#fffbe6"
                                        border.color: (modelData.status === "ACCEPTED") ? "#52c41a" :
                                                      (modelData.status === "COMPLETED") ? "#91d5ff" :
                                                      (modelData.status === "EARLY_TERMINATED") ? "#ffa940" :
                                                      (modelData.status === "CANCELLED" || modelData.status === "REJECTED") ? "#ffa39e" : "#ffe58f"
                                        border.width: 1

                                        Text {
                                            id: connStatusText
                                            anchors.centerIn: parent
                                            text: (modelData.status === "EARLY_TERMINATED") ? "KẾT THÚC SỚM" : modelData.statusText
                                            font.pixelSize: 10
                                            font.bold: true
                                            color: (modelData.status === "ACCEPTED") ? "#52c41a" :
                                                   (modelData.status === "COMPLETED") ? "#096dd9" :
                                                   (modelData.status === "EARLY_TERMINATED") ? "#d46b08" :
                                                   (modelData.status === "CANCELLED" || modelData.status === "REJECTED") ? "#cf1322" : "#d48806"
                                        }
                                    }

                                    Text {
                                        text: modelData.createdAt
                                        font.pixelSize: 10
                                        color: Theme.textMuted
                                    }
                                }
                            }
                        }

                        // Empty State
                        Rectangle {
                            anchors.centerIn: parent
                            visible: !userDetail.connections || userDetail.connections.length === 0
                            color: "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "Chưa có lịch sử kết nối khóa học nào"
                                font.pixelSize: 13
                                color: Theme.textMuted
                            }
                        }
                    }
                }

                // =========================================================
                // TAB 3: ĐÁNH GIÁ (REVIEWS)
                // =========================================================
                Item {
                    anchors.fill: parent
                    visible: (userDetail && userDetail.found !== false && userDetailModal.activeTab === 3)

                    ListView {
                        id: reviewListView
                        anchors.fill: parent
                        clip: true
                        spacing: 10
                        model: userDetail ? userDetail.reviews : []

                        delegate: Rectangle {
                            width: reviewListView.width
                            implicitHeight: revCol.implicitHeight + 20
                            color: Theme.bgApp
                            radius: 8
                            border.color: Theme.cardBorder
                            border.width: 1

                            ColumnLayout {
                                id: revCol
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 6

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: (userDetail.profile && userDetail.profile.roleKey === "STUDENT") ? ("Đánh giá cho Gia sư: " + modelData.tutorName) : ("Học viên đánh giá: " + modelData.studentName)
                                        font.bold: true
                                        font.pixelSize: 12
                                        color: Theme.textDark
                                    }

                                    Item { Layout.fillWidth: true }

                                    Text {
                                        text: modelData.ratingScore + " ★"
                                        font.bold: true
                                        font.pixelSize: 12
                                        color: "#D97706"
                                    }

                                    Text {
                                        text: modelData.createdAt
                                        font.pixelSize: 10
                                        color: Theme.textMuted
                                    }
                                }

                                Text {
                                    text: "\"" + modelData.comment + "\""
                                    font.pixelSize: 12
                                    font.italic: true
                                    color: Theme.textDark
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        // Empty State
                        Rectangle {
                            anchors.centerIn: parent
                            visible: !userDetail.reviews || userDetail.reviews.length === 0
                            color: "transparent"
                            Text {
                                anchors.centerIn: parent
                                text: "Chưa có đánh giá nào được ghi nhận"
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
