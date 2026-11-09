import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TutorMatch

Item {
    id: tutorStudentsRoot

    property var activeList: []
    property var completedList: []
    property int selectedSubTab: 0 // 0: Đang theo học, 1: Lịch sử đã học

    function reloadData() {
        if (typeof guiBridge !== "undefined") {
            activeList = guiBridge.qmlGetTutorActiveStudents() || []
            completedList = guiBridge.qmlGetTutorCompletedStudents() || []
        }
    }

    Component.onCompleted: {
        console.log("[FORENSIC] TutorStudents loaded | selectedSubTab=" + selectedSubTab)
        reloadData()
    }

    Connections {
        target: (typeof guiBridge !== "undefined") ? guiBridge : null
        ignoreUnknownSignals: true
        function onDataChanged() { reloadData() }
        function onTutorScheduleChanged() { reloadData() }
        function onTutorProfileChanged() { reloadData() }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 20

            // Header Title
            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 4
                    Text { text: "HỌC VIÊN & KHÓA HỌC"; font.bold: true; font.pixelSize: 18; color: Theme.textDark }
                    Text { text: "Quản lý danh sách học viên đang theo học và lịch sử kết nối khóa học đã hoàn thành"; font.pixelSize: 12; color: Theme.textMuted }
                }
                Item { Layout.fillWidth: true }
            }

            // Sub-tab Segmented Control
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                SegmentedFilterButton {
                    text: "Đang theo học (" + tutorStudentsRoot.activeList.length + ")"
                    active: tutorStudentsRoot.selectedSubTab === 0
                    implicitWidth: 170
                    implicitHeight: 36
                    onClicked: tutorStudentsRoot.selectedSubTab = 0
                }

                SegmentedFilterButton {
                    text: "Lịch sử đã học (" + tutorStudentsRoot.completedList.length + ")"
                    active: tutorStudentsRoot.selectedSubTab === 1
                    implicitWidth: 170
                    implicitHeight: 36
                    onClicked: tutorStudentsRoot.selectedSubTab = 1
                }

                Item { Layout.fillWidth: true }
            }

            // SUB-TAB 0: ĐANG THEO HỌC (ACTIVE STUDENTS)
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 14
                visible: tutorStudentsRoot.selectedSubTab === 0

                // Empty State Active
                Rectangle {
                    Layout.fillWidth: true
                    height: 200
                    radius: 12
                    color: Theme.cardBg
                    border.color: Theme.cardBorder
                    visible: tutorStudentsRoot.activeList.length === 0

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 12
                        Text {
                            text: "🎓 Bạn chưa có học viên đang theo học"
                            font.bold: true
                            font.pixelSize: 18
                            color: Theme.textDark
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Text {
                            text: "Khi học viên gửi yêu cầu và bạn chấp nhận đơn, học viên sẽ xuất hiện ở đây."
                            font.pixelSize: 13
                            color: Theme.textMuted
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Active Student Cards Grid
                GridLayout {
                    Layout.fillWidth: true
                    columns: (tutorStudentsRoot.width > 900) ? 2 : 1
                    rowSpacing: 14
                    columnSpacing: 14
                    visible: tutorStudentsRoot.activeList.length > 0

                    Repeater {
                        model: tutorStudentsRoot.activeList

                        Rectangle {
                            Layout.fillWidth: true
                            height: 140
                            radius: 10
                            color: Theme.cardBg
                            border.color: Theme.cardBorder

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 16
                                spacing: 10

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 14

                                    Rectangle {
                                        width: 46
                                        height: 46
                                        radius: 23
                                        color: Theme.sidebarBg
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.studentInitials || "H"
                                            font.bold: true
                                            font.pixelSize: 20
                                            color: Theme.textLight
                                        }
                                    }

                                    ColumnLayout {
                                        spacing: 2
                                        Text { text: modelData.studentName; font.bold: true; font.pixelSize: 16; color: Theme.textDark }
                                        Text { text: (modelData.gradeLevel === 0 ? "Khối lớp: Khác" : "Khối lớp " + modelData.gradeLevel) + " · " + (modelData.email || "Email chưa cập nhật"); font.pixelSize: 12; color: Theme.textMuted }
                                    }

                                    Item { Layout.fillWidth: true }

                                    Rectangle {
                                        height: 24
                                        implicitWidth: 90
                                        radius: 4
                                        color: "#f6ffed"
                                        border.color: "#b7eb8f"
                                        Text { anchors.centerIn: parent; text: "ĐANG HỌC"; font.bold: true; font.pixelSize: 10; color: "#52c41a" }
                                    }
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.cardBorder }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "📚 Môn: " + modelData.subject + " · ⏰ Lịch: " + modelData.nextLessonSlot; font.pixelSize: 12; font.bold: true; color: Theme.textDark }
                                    Item { Layout.fillWidth: true }
                                    Button {
                                        text: "🏁 Hoàn thành khóa học"
                                        onClicked: {
                                            if (typeof guiBridge !== "undefined") {
                                                guiBridge.terminateCourse(modelData.bookingId)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // SUB-TAB 1: LỊCH SỬ ĐÃ HỌC (COMPLETED HISTORY)
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 14
                visible: tutorStudentsRoot.selectedSubTab === 1

                // Empty State History
                Rectangle {
                    Layout.fillWidth: true
                    height: 200
                    radius: 12
                    color: Theme.cardBg
                    border.color: Theme.cardBorder
                    visible: tutorStudentsRoot.completedList.length === 0

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 12
                        Text {
                            text: "📜 Chưa có lịch sử khóa học đã hoàn thành"
                            font.bold: true
                            font.pixelSize: 18
                            color: Theme.textDark
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Text {
                            text: "Khi một khóa học kết thúc, lịch sử khóa học và đánh giá sẽ lưu trữ tại đây."
                            font.pixelSize: 13
                            color: Theme.textMuted
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Completed Student Cards Grid
                GridLayout {
                    Layout.fillWidth: true
                    columns: (tutorStudentsRoot.width > 900) ? 2 : 1
                    rowSpacing: 14
                    columnSpacing: 14
                    visible: tutorStudentsRoot.completedList.length > 0

                    Repeater {
                        model: tutorStudentsRoot.completedList

                        Rectangle {
                            Layout.fillWidth: true
                            height: modelData.isReviewed ? 160 : 130
                            radius: 10
                            color: Theme.cardBg
                            border.color: Theme.cardBorder

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 16
                                spacing: 8

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 14

                                    Rectangle {
                                        width: 44
                                        height: 44
                                        radius: 22
                                        color: Theme.sidebarBg
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.studentInitials || "H"
                                            font.bold: true
                                            font.pixelSize: 18
                                            color: Theme.textLight
                                        }
                                    }

                                    ColumnLayout {
                                        spacing: 2
                                        Text { text: modelData.studentName; font.bold: true; font.pixelSize: 15; color: Theme.textDark }
                                        Text { text: (modelData.gradeLevel === 0 ? "Khối lớp: Khác" : "Khối lớp " + modelData.gradeLevel) + " · " + (modelData.email || "Email chưa cập nhật"); font.pixelSize: 12; color: Theme.textMuted }
                                    }

                                    Item { Layout.fillWidth: true }

                                    Rectangle {
                                        height: 24
                                        implicitWidth: 110
                                        radius: 4
                                        color: Theme.bgApp
                                        border.color: Theme.cardBorder
                                        Text { anchors.centerIn: parent; text: "ĐÃ HOÀN THÀNH"; font.bold: true; font.pixelSize: 10; color: Theme.textDark }
                                    }
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.cardBorder }

                                Text {
                                    text: "📚 Môn: " + modelData.subject + " · ⏰ Lịch từng học: " + modelData.nextLessonSlot
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: Theme.textDark
                                }

                                // Review Display if available
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 32
                                    radius: 6
                                    color: Theme.cMistyRose
                                    visible: modelData.isReviewed

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 10
                                        anchors.rightMargin: 10
                                        spacing: 8
                                        Text { text: "⭐ " + modelData.ratingScore + " ★"; font.bold: true; font.pixelSize: 11; color: "#d97706" }
                                        Text {
                                            text: "\"" + modelData.comment + "\""
                                            font.italic: true
                                            font.pixelSize: 11
                                            color: Theme.textDark
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
