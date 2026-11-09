import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: studentSmartMatchView

    property var matchesList: []
    property bool isRefreshing: false
    property var studentProfile: (typeof guiBridge !== "undefined") ? guiBridge.currentStudentProfile : ({})

    function reloadMatches() {
        isRefreshing = true
        if (typeof guiBridge !== "undefined") {
            studentProfile = guiBridge.currentStudentProfile
            matchesList = guiBridge.qmlGetSmartMatches(5)
        }
        isRefreshing = false
    }

    Component.onCompleted: {
        reloadMatches()
    }

    Connections {
        target: (typeof guiBridge !== "undefined") ? guiBridge : null
        ignoreUnknownSignals: true
        function onStateChanged() { reloadMatches() }
        function onDataChanged() { reloadMatches() }
        function onStudentProfileChanged() { reloadMatches() }
        function onStudentWeeklyScheduleChanged() { reloadMatches() }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        // Header Title Banner
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true
                Layout.minimumWidth: 0

                Text {
                    text: "GỢI Ý GIA SƯ PHÙ HỢP NHẤT (SMART MATCHING ENGINE)"
                    font.bold: true
                    font.pixelSize: 18
                    color: Theme.textDark
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
                Text {
                    text: "Phân tích đa tiêu chí bằng C++ MaxHeap Engine: Môn học mục tiêu, Ngân sách học phí, Ma trận lịch rảnh & Uy tín gia sư"
                    font.pixelSize: 12
                    color: Theme.textMuted
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }
            }

            Item { Layout.fillWidth: true }

            // Student Criteria Summary Badge
            Rectangle {
                height: 36
                implicitWidth: criteriaText.implicitWidth + 24
                radius: 6
                color: Theme.cardBg
                border.color: Theme.cardBorder
                border.width: 1

                Text {
                    id: criteriaText
                    anchors.centerIn: parent
                    text: "Môn: " + (studentSmartMatchView.studentProfile.targetSubject || "Tất cả") +
                          " | Ngân sách: " + ((studentSmartMatchView.studentProfile.maxBudget || 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ")
                    font.pixelSize: 11
                    font.bold: true
                    color: "#d97706"
                }
            }

            Button {
                text: "Làm mới đề xuất"
                focusPolicy: Qt.NoFocus
                contentItem: Text {
                    text: parent.text
                    font.bold: true
                    font.pixelSize: 11
                    color: parent.down ? Theme.textLight : (parent.hovered ? Theme.textDark : Theme.btnText)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitWidth: 120
                    implicitHeight: 36
                    radius: 6
                    color: parent.down ? Theme.sidebarBg : (parent.hovered ? Theme.sidebarActive : Theme.btnPrimary)
                    border.color: Theme.cardBorder
                    border.width: 1
                }
                onClicked: studentSmartMatchView.reloadMatches()
            }
        }

        // Notification Banner
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
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    elide: Text.ElideRight
                }
            }
        }

        // Loading State Card
        Rectangle {
            Layout.fillWidth: true
            height: 100
            color: Theme.cardBg
            radius: 10
            border.color: Theme.cardBorder
            visible: studentSmartMatchView.isRefreshing

            RowLayout {
                anchors.centerIn: parent
                spacing: 12
                BusyIndicator { running: true }
                Text {
                    text: "Đang phân tích hồ sơ và ma trận lịch rảnh của bạn bằng thuật toán C++ MaxHeap..."
                    font.pixelSize: 13
                    font.bold: true
                    color: Theme.textDark
                }
            }
        }

        // Empty State Card
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.cardBg
            radius: 10
            border.color: Theme.cardBorder
            border.width: 1
            visible: !studentSmartMatchView.isRefreshing && studentSmartMatchView.matchesList.length === 0

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 14

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "🔍 Chưa tìm thấy gia sư phù hợp với tiêu chí hiện tại"
                    font.bold: true
                    font.pixelSize: 18
                    color: Theme.textDark
                }

                ColumnLayout {
                    spacing: 4
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: "Gợi ý cải thiện độ tương thích:"; font.bold: true; font.pixelSize: 13; color: Theme.textDark }
                    Text { text: "• Cập nhật Môn học mục tiêu trong Hồ sơ cá nhân (Tab 0)"; font.pixelSize: 12; color: Theme.textMuted }
                    Text { text: "• Nâng hạn mức Ngân sách học phí tối đa"; font.pixelSize: 12; color: Theme.textMuted }
                    Text { text: "• Mở rộng các ô ca RẢNH trong Ma trận lịch học hàng tuần"; font.pixelSize: 12; color: Theme.textMuted }
                }

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Làm mới danh sách gợi ý"
                    focusPolicy: Qt.NoFocus
                    contentItem: Text { text: parent.text; font.bold: true; color: Theme.btnText }
                    background: Rectangle { implicitWidth: 160; implicitHeight: 36; radius: 6; color: Theme.btnPrimary }
                    onClicked: studentSmartMatchView.reloadMatches()
                }
            }
        }

        // Main Recommendations View (Flickable Content Area)
        ScrollView {
            id: matchScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            visible: !studentSmartMatchView.isRefreshing && studentSmartMatchView.matchesList.length > 0

            ColumnLayout {
                width: matchScrollView.availableWidth
                spacing: 16

                // 1. TOP MATCH CARD (Rank 1 Highlight Card)
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: topMatchCol.implicitHeight + 32
                    color: Theme.cardBg
                    radius: 10
                    border.color: Theme.cardBorder
                    border.width: 1
                    clip: true

                    ColumnLayout {
                        id: topMatchCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 16
                        spacing: 14

                        // Top Header Row
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Rectangle {
                                height: 26
                                implicitWidth: rank1Text.implicitWidth + 16
                                radius: 4
                                color: "#132719"
                                border.color: "#52c41a"
                                border.width: 1

                                Text {
                                    id: rank1Text
                                    anchors.centerIn: parent
                                    text: "★ ĐỀ XUẤT PHÙ HỢP NHẤT (RANK #1)"
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: "#52c41a"
                                }
                            }

                            Item { Layout.fillWidth: true }

                            // Composite Match Score Pill
                            Rectangle {
                                height: 28
                                implicitWidth: scoreText.implicitWidth + 24
                                radius: 14
                                color: Theme.cMistyRose
                                border.color: Theme.cardBorder
                                border.width: 1

                                Text {
                                    id: scoreText
                                    anchors.centerIn: parent
                                    text: "Độ tương thích: " + (studentSmartMatchView.matchesList[0] ? studentSmartMatchView.matchesList[0].compositeScore : 0) + "%"
                                    font.bold: true
                                    font.pixelSize: 12
                                    color: Theme.textDark
                                }
                            }
                        }

                        // Tutor Profile Header Row
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 14

                            Rectangle {
                                width: 56
                                height: 56
                                radius: 28
                                color: Theme.sidebarBg
                                Text {
                                    anchors.centerIn: parent
                                    text: (studentSmartMatchView.matchesList[0] && studentSmartMatchView.matchesList[0].initials) ? studentSmartMatchView.matchesList[0].initials : "G"
                                    font.bold: true
                                    font.pixelSize: 22
                                    color: Theme.textLight
                                }
                            }

                            ColumnLayout {
                                spacing: 4
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        text: (studentSmartMatchView.matchesList[0] ? studentSmartMatchView.matchesList[0].fullName : "")
                                        font.bold: true
                                        font.pixelSize: 17
                                        color: Theme.textDark
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        elide: Text.ElideRight
                                    }

                                    Rectangle {
                                        visible: (studentSmartMatchView.matchesList[0] && studentSmartMatchView.matchesList[0].isVerified === true)
                                        width: 80
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

                                    Item { Layout.fillWidth: true }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: (studentSmartMatchView.matchesList[0] ? studentSmartMatchView.matchesList[0].subject : "") +
                                          " • " + ((studentSmartMatchView.matchesList[0] ? studentSmartMatchView.matchesList[0].hourlyRate : 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ/buổi")
                                    font.pixelSize: 13
                                    font.bold: true
                                    color: "#d97706"
                                    wrapMode: Text.WordWrap
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Text {
                                        text: (studentSmartMatchView.matchesList[0] ? studentSmartMatchView.matchesList[0].rating : 5.0).toFixed(1) + " ★"
                                        font.bold: true
                                        font.pixelSize: 12
                                        color: "#d97706"
                                    }
                                    Text {
                                        text: "(" + (studentSmartMatchView.matchesList[0] ? studentSmartMatchView.matchesList[0].totalReviews : 0) + " đánh giá)"
                                        font.pixelSize: 11
                                        color: Theme.textMuted
                                    }
                                }
                            }
                        }

                        // Match Reasons Breakdown Card
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: reasonsCol.implicitHeight + 24
                            color: Theme.bgApp
                            radius: 8
                            border.color: Theme.cardBorder
                            border.width: 1

                            ColumnLayout {
                                id: reasonsCol
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 12
                                spacing: 6

                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    text: "Lý do hệ thống gợi ý gia sư này:"
                                    font.bold: true
                                    font.pixelSize: 12
                                    color: Theme.textDark
                                }

                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    wrapMode: Text.WordWrap
                                    visible: (studentSmartMatchView.matchesList[0] && studentSmartMatchView.matchesList[0].subjectScore >= 80)
                                    text: "✓ Chuyên môn giảng dạy khớp 100% với môn học mục tiêu (" + (studentSmartMatchView.matchesList[0] ? studentSmartMatchView.matchesList[0].subject : "") + ")"
                                    font.pixelSize: 11
                                    color: "#2b6cb0"
                                }

                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    wrapMode: Text.WordWrap
                                    visible: (studentSmartMatchView.matchesList[0] && studentSmartMatchView.matchesList[0].priceScore >= 80)
                                    text: "✓ Mức học phí nằm hoàn toàn trong ngân sách dự kiến của học viên"
                                    font.pixelSize: 11
                                    color: "#2b6cb0"
                                }

                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    wrapMode: Text.WordWrap
                                    visible: (studentSmartMatchView.matchesList[0] && studentSmartMatchView.matchesList[0].scheduleOverlapCount > 0)
                                    text: "✓ Trùng " + (studentSmartMatchView.matchesList[0] ? studentSmartMatchView.matchesList[0].scheduleOverlapCount : 0) + " ca rảnh tương thích trong Ma trận thời khóa biểu"
                                    font.pixelSize: 11
                                    color: "#2b6cb0"
                                }

                                Text {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    wrapMode: Text.WordWrap
                                    visible: (studentSmartMatchView.matchesList[0] && studentSmartMatchView.matchesList[0].isVerified)
                                    text: "✓ Tài khoản gia sư đã qua xác minh bằng cấp & giấy tờ chứng thực"
                                    font.pixelSize: 11
                                    color: "#2b6cb0"
                                }
                            }
                        }

                        // Action Buttons for Top Match
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Item { Layout.fillWidth: true }

                            Button {
                                text: "Xem chi tiết"
                                focusPolicy: Qt.NoFocus
                                implicitWidth: 120
                                implicitHeight: 36
                                contentItem: Text {
                                    text: parent.text
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: Theme.textDark
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    radius: 8
                                    color: parent.down ? Theme.sidebarActive : (parent.hovered ? Theme.bgApp : Theme.cMistyRose)
                                    border.color: Theme.cardBorder
                                    border.width: 1
                                }
                                onClicked: {
                                    if (studentSmartMatchView.matchesList[0]) {
                                        tutorDetailModal.openWithTutorId(studentSmartMatchView.matchesList[0].tutorId)
                                    }
                                }
                            }

                            Button {
                                text: "Đăng ký học"
                                focusPolicy: Qt.NoFocus
                                implicitWidth: 120
                                implicitHeight: 36
                                contentItem: Text {
                                    text: parent.text
                                    font.bold: true
                                    font.pixelSize: 11
                                    color: Theme.btnText
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    radius: 8
                                    color: parent.down ? Theme.btnPrimaryHover : (parent.hovered ? Theme.btnPrimaryHover : Theme.sidebarBg)
                                }
                                onClicked: {
                                    if (studentSmartMatchView.matchesList[0]) {
                                        bookingModal.openWithTutor(studentSmartMatchView.matchesList[0])
                                    }
                                }
                            }
                        }
                    }
                }

                // 2. OTHER RANKED RECOMMENDATIONS CATALOG (Rank #2+)
                Text {
                    visible: studentSmartMatchView.matchesList.length > 1
                    text: "DANH SÁCH ĐỀ XUẤT BỔ SUNG (THEO THUẬT TOÁN MAXHEAP)"
                    font.bold: true
                    font.pixelSize: 14
                    color: Theme.textDark
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    wrapMode: Text.WordWrap
                }

                Repeater {
                    model: studentSmartMatchView.matchesList.slice(1)

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: matchRowCol.implicitHeight + 32
                        color: Theme.cardBg
                        radius: 10
                        border.color: Theme.cardBorder
                        border.width: 1
                        clip: true

                        ColumnLayout {
                            id: matchRowCol
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 16
                            spacing: 12

                            // Top Header Row for Rank #2+ Cards
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                // Rank Pill
                                Rectangle {
                                    width: 64
                                    height: 24
                                    radius: 4
                                    color: Theme.sidebarBg
                                    Text {
                                        anchors.centerIn: parent
                                        text: "RANK #" + modelData.rank
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: Theme.sidebarActive
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                // Compatibility Score Badge
                                Rectangle {
                                    height: 28
                                    implicitWidth: scorePillText.implicitWidth + 24
                                    radius: 14
                                    color: Theme.cMistyRose
                                    border.color: Theme.cardBorder
                                    border.width: 1

                                    Text {
                                        id: scorePillText
                                        anchors.centerIn: parent
                                        text: modelData.compositeScore + "% Tương thích"
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: Theme.textDark
                                    }
                                }
                            }

                            // Profile Header Row
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 14

                                Rectangle {
                                    width: 48
                                    height: 48
                                    radius: 24
                                    color: Theme.sidebarBg
                                    Text {
                                        anchors.centerIn: parent
                                        text: (modelData.initials ? modelData.initials : "G")
                                        font.bold: true
                                        font.pixelSize: 18
                                        color: Theme.textLight
                                    }
                                }

                                ColumnLayout {
                                    spacing: 4
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Text {
                                            text: modelData.fullName || "Gia sư"
                                            font.bold: true
                                            font.pixelSize: 15
                                            color: Theme.textDark
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            elide: Text.ElideRight
                                        }

                                        Rectangle {
                                            visible: (modelData.isVerified === true)
                                            width: 76
                                            height: 18
                                            radius: 4
                                            color: "#132719"
                                            border.color: "#52c41a"
                                            border.width: 1
                                            Text {
                                                anchors.centerIn: parent
                                                text: "Đã xác minh"
                                                font.pixelSize: 9
                                                font.bold: true
                                                color: "#52c41a"
                                            }
                                        }

                                        Text {
                                            text: (modelData.rating || 5.0).toFixed(1) + " ★ (" + (modelData.totalReviews || 0) + " đánh giá)"
                                            font.bold: true
                                            font.pixelSize: 11
                                            color: "#d97706"
                                        }

                                        Item { Layout.fillWidth: true }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        text: (modelData.subject || "N/A") + " • " + ((modelData.hourlyRate || 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0) + " VNĐ/buổi")
                                        font.pixelSize: 12
                                        font.bold: true
                                        color: "#d97706"
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }

                            // Match Reasons Snippet Card
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: reasonSnippetCol.implicitHeight + 20
                                color: Theme.bgApp
                                radius: 6
                                border.color: Theme.cardBorder
                                border.width: 1

                                ColumnLayout {
                                    id: reasonSnippetCol
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 10
                                    spacing: 4

                                    Text {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        wrapMode: Text.WordWrap
                                        text: "✓ Lý do phù hợp: Môn học (" + modelData.subjectScore + "%) | Học phí (" + modelData.priceScore + "%) | Lịch rảnh trùng (" + modelData.scheduleOverlapCount + " ca)"
                                        font.pixelSize: 11
                                        color: "#2b6cb0"
                                    }
                                }
                            }

                            // Action Buttons Row
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                Item { Layout.fillWidth: true }

                                Button {
                                    text: "Xem chi tiết"
                                    focusPolicy: Qt.NoFocus
                                    implicitWidth: 120
                                    implicitHeight: 36
                                    contentItem: Text {
                                        text: parent.text
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: Theme.textDark
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 8
                                        color: parent.down ? Theme.sidebarActive : (parent.hovered ? Theme.bgApp : Theme.cMistyRose)
                                        border.color: Theme.cardBorder
                                        border.width: 1
                                    }
                                    onClicked: tutorDetailModal.openWithTutorId(modelData.tutorId)
                                }

                                Button {
                                    text: "Đăng ký học"
                                    focusPolicy: Qt.NoFocus
                                    implicitWidth: 120
                                    implicitHeight: 36
                                    contentItem: Text {
                                        text: parent.text
                                        font.bold: true
                                        font.pixelSize: 11
                                        color: Theme.btnText
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        radius: 8
                                        color: parent.down ? Theme.btnPrimaryHover : (parent.hovered ? Theme.btnPrimaryHover : Theme.sidebarBg)
                                    }
                                    onClicked: bookingModal.openWithTutor(modelData)
                                }
                            }
                        }
                    }
                }

                // Bottom Spacer to prevent Rank #4 clipping
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                }
            }
        }
    }

    // Modals Integration
    TutorDetailModal {
        id: tutorDetailModal
        onBookCourseRequested: function(tutor) {
            bookingModal.openWithTutor(tutor)
        }
    }

    BookingModal {
        id: bookingModal
        onBookingCompleted: {
            studentSmartMatchView.reloadMatches()
        }
    }
}
