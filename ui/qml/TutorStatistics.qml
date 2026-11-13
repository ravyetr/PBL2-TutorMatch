import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: tutorStatisticsRoot

    property var statsData: ({})
    property var reviewsList: []

    function reloadData() {
        if (typeof guiBridge !== "undefined") {
            statsData = guiBridge.qmlGetTutorStatistics() || {}
            reviewsList = guiBridge.qmlGetTutorReviews() || []
        }
    }

    Component.onCompleted: reloadData()

    Connections {
        target: (typeof guiBridge !== "undefined") ? guiBridge : null
        ignoreUnknownSignals: true
        function onDataChanged() { reloadData() }
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
                    Text { text: "ĐÁNH GIÁ & PHẢN HỒI TỪ HỌC VIÊN"; font.bold: true; font.pixelSize: 18; color: Theme.textDark }
                    Text { text: "Danh sách đánh giá, nhận xét và điểm chất lượng tích lũy từ học viên"; font.pixelSize: 12; color: Theme.textMuted }
                }
            }

            // SECTION 1: STATS METRICS CARDS (4 Cards)
            GridLayout {
                Layout.fillWidth: true
                columns: (width > 800) ? 4 : 2
                rowSpacing: 12
                columnSpacing: 12

                Rectangle {
                    Layout.fillWidth: true; height: 95; radius: 10; color: Theme.cardBg; border.color: Theme.cardBorder
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 14; spacing: 4
                        Text { text: "TỔNG HỌC VIÊN"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                        Text { text: (tutorStatisticsRoot.statsData.totalStudents || 0).toString() + " học viên"; font.bold: true; font.pixelSize: 22; color: Theme.textDark }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true; height: 95; radius: 10; color: Theme.cardBg; border.color: Theme.cardBorder
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 14; spacing: 4
                        Text { text: "CA HỌC CỐ ĐỊNH"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                        Text { text: (tutorStatisticsRoot.statsData.upcomingLessons || 0).toString() + " ca / tuần"; font.bold: true; font.pixelSize: 22; color: "#1677ff" }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true; height: 95; radius: 10; color: Theme.cardBg; border.color: Theme.cardBorder
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 14; spacing: 4
                        Text { text: "ĐÁNH GIÁ TRUNG BÌNH"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                        Text { text: (tutorStatisticsRoot.statsData.averageRating || 5.0).toFixed(1) + " ★ (" + tutorStatisticsRoot.reviewsList.length + ")"; font.bold: true; font.pixelSize: 22; color: "#d97706" }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true; height: 95; radius: 10; color: Theme.cardBg; border.color: Theme.cardBorder
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 14; spacing: 4
                        Text { text: "TỔNG HỌC PHÍ TÍCH LŨY"; font.pixelSize: 11; font.bold: true; color: Theme.textMuted }
                        Text { 
                            text: ((tutorStatisticsRoot.statsData.totalEarnings || 0).toLocaleString(Qt.locale("vi_VN"), 'f', 0)) + " VNĐ"
                            font.bold: true
                            font.pixelSize: 20
                            color: "#16a34a" 
                        }
                    }
                    ToolTip.visible: mouseAreaStatsIncome.containsMouse
                    ToolTip.text: "Giá trị học phí của các khóa học đã hoàn thành theo thỏa thuận; không đồng nghĩa với số tiền đã thực nhận."
                    MouseArea {
                        id: mouseAreaStatsIncome
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }
                }
            }

            // SECTION 2: RATING DISTRIBUTION
            Rectangle {
                Layout.fillWidth: true
                height: 140
                radius: 12
                color: Theme.cardBg
                border.color: Theme.cardBorder

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10

                    Text { text: "PHÂN BỐ ĐÁNH GIÁ (SAO)"; font.bold: true; font.pixelSize: 14; color: Theme.textDark }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 20

                        Repeater {
                            model: [
                                { label: "5 ★", count: tutorStatisticsRoot.statsData.star5 || 0 },
                                { label: "4 ★", count: tutorStatisticsRoot.statsData.star4 || 0 },
                                { label: "3 ★", count: tutorStatisticsRoot.statsData.star3 || 0 },
                                { label: "2 ★", count: tutorStatisticsRoot.statsData.star2 || 0 },
                                { label: "1 ★", count: tutorStatisticsRoot.statsData.star1 || 0 }
                            ]

                            Rectangle {
                                Layout.fillWidth: true
                                height: 50
                                radius: 8
                                color: Theme.bgApp
                                border.color: Theme.cardBorder

                                ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 2
                                    Text { text: modelData.label; font.bold: true; font.pixelSize: 12; color: "#d97706" }
                                    Text { text: modelData.count + " đánh giá"; font.pixelSize: 11; color: Theme.textMuted }
                                }
                            }
                        }
                    }
                }
            }

            // SECTION 3: REVIEWS LOG
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 12

                Text { text: "DANH SÁCH ĐÁNH GIÁ TỪ HỌC VIÊN (" + tutorStatisticsRoot.reviewsList.length + ")"; font.bold: true; font.pixelSize: 15; color: Theme.textDark }

                // Empty State
                Rectangle {
                    Layout.fillWidth: true
                    height: 150
                    radius: 10
                    color: Theme.cardBg
                    border.color: Theme.cardBorder
                    visible: tutorStatisticsRoot.reviewsList.length === 0

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 8
                        Text {
                            text: "⭐ Chưa có đánh giá nào từ học viên"
                            font.bold: true
                            font.pixelSize: 16
                            color: Theme.textDark
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Text {
                            text: "Sau khi học viên hoàn thành khóa học, phản hồi của họ sẽ tự động được ghi nhận tại đây."
                            font.pixelSize: 13
                            color: Theme.textMuted
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                Repeater {
                    model: tutorStatisticsRoot.reviewsList

                    Rectangle {
                        Layout.fillWidth: true
                        height: 90
                        radius: 10
                        color: Theme.cardBg
                        border.color: Theme.cardBorder

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 14

                            Rectangle {
                                width: 44
                                height: 44
                                radius: 22
                                color: Theme.sidebarBg
                                Text { anchors.centerIn: parent; text: modelData.studentInitials || "H"; font.bold: true; font.pixelSize: 18; color: Theme.textLight }
                            }

                            ColumnLayout {
                                spacing: 4
                                RowLayout {
                                    spacing: 8
                                    Text { text: modelData.studentName; font.bold: true; font.pixelSize: 14; color: Theme.textDark }
                                    Text { text: modelData.ratingScore + " ★"; font.bold: true; font.pixelSize: 13; color: "#d97706" }
                                    Text { text: "· " + (modelData.createdAt || "N/A"); font.pixelSize: 11; color: Theme.textMuted }
                                }

                                Text {
                                    text: "\"" + modelData.comment + "\""
                                    font.italic: true
                                    font.pixelSize: 12
                                    color: Theme.textDark
                                    elide: Text.ElideRight
                                    maximumLineCount: 2
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }
                }
            }
        }
    }
}
