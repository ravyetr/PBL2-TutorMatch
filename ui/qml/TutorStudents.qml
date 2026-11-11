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
                            Layout.preferredWidth: 1
                            Layout.minimumWidth: 0
                            Layout.fillHeight: true
                            implicitHeight: activeCol.implicitHeight + 32
                            radius: 10
                            color: Theme.cardBg
                            border.color: Theme.cardBorder
                            border.width: 1
                            clip: true

                            ColumnLayout {
                                id: activeCol
                                anchors.fill: parent
                                anchors.margins: 16
                                spacing: 10

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    spacing: 14

                                    Rectangle {
                                        width: 46
                                        height: 46
                                        radius: 23
                                        color: Theme.sidebarBg
                                        Layout.alignment: Qt.AlignVCenter
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.studentInitials || "H"
                                            font.bold: true
                                            font.pixelSize: 20
                                            color: Theme.textLight
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        spacing: 2
                                        Layout.alignment: Qt.AlignVCenter

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: modelData.studentName
                                            font.bold: true
                                            font.pixelSize: 16
                                            color: Theme.textDark
                                            elide: Text.ElideRight
                                            maximumLineCount: 1
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: (modelData.gradeLevel === 0 ? "Khối lớp: Khác" : "Khối lớp " + modelData.gradeLevel) + " · " + (modelData.email || "Email chưa cập nhật")
                                            font.pixelSize: 12
                                            color: Theme.textMuted
                                            elide: Text.ElideRight
                                            maximumLineCount: 1
                                        }
                                    }

                                    Rectangle {
                                        height: 24
                                        implicitWidth: 90
                                        radius: 4
                                        color: "#f6ffed"
                                        border.color: "#b7eb8f"
                                        border.width: 1
                                        Layout.alignment: Qt.AlignVCenter
                                        Text { anchors.centerIn: parent; text: "ĐANG HỌC"; font.bold: true; font.pixelSize: 10; color: "#52c41a" }
                                    }
                                }

                                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.cardBorder }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    spacing: 10

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        spacing: 4

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: "📚 Môn: " + modelData.subject
                                            font.pixelSize: 12
                                            font.bold: true
                                            color: Theme.textDark
                                            elide: Text.ElideRight
                                            maximumLineCount: 1
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: "⏰ Lịch: " + modelData.nextLessonSlot
                                            font.pixelSize: 12
                                            color: Theme.textMuted
                                            elide: Text.ElideRight
                                            maximumLineCount: 1
                                        }
                                    }

                                    Button {
                                        text: "🏁 Hoàn thành khóa học"
                                        Layout.alignment: Qt.AlignVCenter
                                        onClicked: {
                                            if (typeof guiBridge !== "undefined") {
                                                guiBridge.terminateCourse(modelData.bookingId)
                                            }
                                        }
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
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
                            Layout.preferredWidth: 1
                            Layout.minimumWidth: 0
                            Layout.fillHeight: true
                            implicitHeight: cardCol.implicitHeight + 32
                            radius: 10
                            color: Theme.cardBg
                            border.color: Theme.cardBorder
                            border.width: 1
                            clip: true

                            ColumnLayout {
                                id: cardCol
                                anchors.fill: parent
                                anchors.margins: 16
                                spacing: 10

                                // SECTION 1: STUDENT IDENTITY
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    spacing: 12

                                    Rectangle {
                                        width: 44
                                        height: 44
                                        radius: 22
                                        color: Theme.sidebarBg
                                        Layout.alignment: Qt.AlignVCenter
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.studentInitials || "H"
                                            font.bold: true
                                            font.pixelSize: 18
                                            color: Theme.textLight
                                        }
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        spacing: 2
                                        Layout.alignment: Qt.AlignVCenter

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: modelData.studentName
                                            font.bold: true
                                            font.pixelSize: 15
                                            color: Theme.textDark
                                            elide: Text.ElideRight
                                            maximumLineCount: 1
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: (modelData.gradeLevel === 0 ? "Khối lớp: Khác" : "Khối lớp " + modelData.gradeLevel) + " · " + (modelData.email || "Email chưa cập nhật")
                                            font.pixelSize: 12
                                            color: Theme.textMuted
                                            elide: Text.ElideRight
                                            maximumLineCount: 1
                                        }
                                    }

                                    Rectangle {
                                        height: 24
                                        implicitWidth: 105
                                        radius: 4
                                        color: Theme.bgApp
                                        border.color: Theme.cardBorder
                                        border.width: 1
                                        Layout.alignment: Qt.AlignVCenter

                                        Text {
                                            anchors.centerIn: parent
                                            text: "ĐÃ HOÀN THÀNH"
                                            font.bold: true
                                            font.pixelSize: 10
                                            color: Theme.textDark
                                        }
                                    }
                                }

                                // DIVIDER
                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 1
                                    color: Theme.cardBorder
                                }

                                // SECTION 2: COURSE INFORMATION
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    spacing: 5

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        spacing: 6

                                        Text {
                                            text: "📚"
                                            font.pixelSize: 12
                                            Layout.alignment: Qt.AlignTop
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: modelData.subject || "Khóa học"
                                            font.bold: true
                                            font.pixelSize: 12
                                            color: Theme.textDark
                                            wrapMode: Text.WordWrap
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 0
                                        spacing: 6

                                        Text {
                                            text: "⏰"
                                            font.pixelSize: 12
                                            Layout.alignment: Qt.AlignTop
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: "Lịch học: " + (modelData.nextLessonSlot || "Chưa cập nhật")
                                            font.pixelSize: 12
                                            color: Theme.textDark
                                            wrapMode: Text.WordWrap
                                        }
                                    }
                                }

                                // SECTION 3: REVIEW (Only when reviewed)
                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    implicitHeight: revCol.implicitHeight + 16
                                    radius: 6
                                    color: Theme.cMistyRose
                                    border.color: "#E8C5C8"
                                    border.width: 1
                                    visible: Boolean(modelData.isReviewed)

                                    ColumnLayout {
                                        id: revCol
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 4

                                        RowLayout {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            spacing: 6

                                            Text {
                                                text: "⭐ " + (modelData.ratingScore || 5) + " ★"
                                                font.bold: true
                                                font.pixelSize: 11
                                                color: "#d97706"
                                            }

                                            Text {
                                                text: "Đánh giá từ học viên"
                                                font.pixelSize: 10
                                                font.bold: true
                                                color: Theme.textMuted
                                            }

                                            Item { Layout.fillWidth: true }
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 0
                                            text: modelData.comment ? ("\"" + modelData.comment + "\"") : ""
                                            font.italic: true
                                            font.pixelSize: 11
                                            color: Theme.textDark
                                            wrapMode: Text.WordWrap
                                        }
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
