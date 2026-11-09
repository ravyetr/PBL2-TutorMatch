import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TutorMatch

Rectangle {
    id: adminReviewsView
    color: Theme.cardBg
    radius: 10
    border.color: Theme.cardBorder
    border.width: 1

    property var reviewsModel: []
    property int pendingDeleteId: 0
    property string pendingDeleteLabel: ""

    function reloadReviews() {
        if (typeof guiBridge !== "undefined") {
            reviewsModel = guiBridge.qmlGetAllReviews()
        }
    }

    Component.onCompleted: {
        reloadReviews()
    }

    Connections {
        target: (typeof guiBridge !== "undefined") ? guiBridge : null
        ignoreUnknownSignals: true
        function onDataChanged() { reloadReviews() }
    }

    function formatStars(score) {
        var s = ""
        for (var i = 1; i <= 5; i++) {
            s += (i <= score) ? "★" : "☆"
        }
        return s + " (" + score + "/5)"
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 16

            // 1. Title Header & Reload Button
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                ColumnLayout {
                    spacing: 4
                    Text {
                        text: "Kiểm duyệt Đánh giá"
                        font.bold: true
                        font.pixelSize: 18
                        color: Theme.textDark
                    }
                    Text {
                        text: "Quản lý phản hồi của học viên và duy trì chất lượng cộng đồng"
                        font.pixelSize: 12
                        color: Theme.textMuted
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "Tổng số đánh giá: " + adminReviewsView.reviewsModel.length
                    font.bold: true
                    font.pixelSize: 13
                    color: Theme.textDark
                }

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
                    onClicked: reloadReviews()
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

            // 2. Reviews List
            ListView {
                id: reviewsListView
                Layout.fillWidth: true
                implicitHeight: Math.max(300, count * 140)
                clip: true
                spacing: 12
                model: adminReviewsView.reviewsModel

                delegate: Rectangle {
                    width: reviewsListView.width
                    height: 130
                    color: Theme.bgApp
                    radius: 12
                    border.color: Theme.cardBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        // Top Row: Participant Info & Rating
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            // Student & Tutor Info
                            ColumnLayout {
                                spacing: 2
                                RowLayout {
                                    spacing: 6
                                    Text {
                                        text: "Học viên: " + modelData.studentName
                                        font.bold: true
                                        font.pixelSize: 13
                                        color: Theme.textDark
                                    }
                                    Text {
                                        text: "➔"
                                        font.pixelSize: 12
                                        color: Theme.textMuted
                                    }
                                    Text {
                                        text: "Gia sư: " + modelData.tutorName
                                        font.bold: true
                                        font.pixelSize: 13
                                        color: Theme.accentBlue
                                    }
                                }
                                Text {
                                    text: "Ngày gửi: " + modelData.createdAt + " · Mã lớp: #" + modelData.bookingId
                                    font.pixelSize: 11
                                    color: Theme.textMuted
                                }
                            }

                            Item { Layout.fillWidth: true }

                            // Star Rating Badge
                            Rectangle {
                                implicitWidth: ratingText.implicitWidth + 16
                                height: 26
                                radius: 6
                                color: "#fffbe6"
                                border.color: "#ffe58f"
                                border.width: 1

                                Text {
                                    id: ratingText
                                    anchors.centerIn: parent
                                    text: adminReviewsView.formatStars(modelData.ratingScore)
                                    font.bold: true
                                    font.pixelSize: 12
                                    color: "#d48806"
                                }
                            }

                            // Action Button: Delete Review
                            Button {
                                text: "Xóa đánh giá"
                                contentItem: Text {
                                    text: parent.text
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: "#cf1322"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    implicitWidth: 100
                                    implicitHeight: 30
                                    radius: 6
                                    color: parent.hovered ? "#ffa39e" : "#fff1f0"
                                    border.color: "#ffa39e"
                                    border.width: 1
                                }
                                onClicked: {
                                    adminReviewsView.pendingDeleteId = modelData.id
                                    adminReviewsView.pendingDeleteLabel = "Đánh giá #" + modelData.id + " của " + modelData.studentName + " dành cho gia sư " + modelData.tutorName
                                    deleteConfirmModal.open()
                                }
                            }
                        }

                        // Divider Line
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Theme.cardBorder
                            opacity: 0.5
                        }

                        // Comment Content
                        Text {
                            Layout.fillWidth: true
                            text: "\"" + modelData.comment + "\""
                            font.pixelSize: 12
                            font.italic: true
                            color: Theme.textDark
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                // Empty State Placeholder
                Rectangle {
                    anchors.centerIn: parent
                    visible: adminReviewsView.reviewsModel.length === 0
                    color: "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "Hiện chưa có đánh giá nào trên hệ thống"
                        font.pixelSize: 13
                        color: Theme.textMuted
                    }
                }
            }
        }
    }

    // Confirmation Modal Dialog
    Dialog {
        id: deleteConfirmModal
        anchors.centerIn: parent
        modal: true
        focus: true
        title: "Xác nhận gỡ đánh giá"
        standardButtons: Dialog.No | Dialog.Yes

        background: Rectangle {
            implicitWidth: 420
            implicitHeight: 180
            color: Theme.cardBg
            radius: 10
            border.color: Theme.cardBorder
            border.width: 1
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Text {
                text: "XÁC NHẬN GỠ ĐÁNH GIÁ VI PHẠM"
                font.bold: true
                font.pixelSize: 15
                color: Theme.textDark
            }

            Text {
                text: "Bạn có chắc chắn muốn gỡ:\n" + adminReviewsView.pendingDeleteLabel + "?\n\nĐánh giá sẽ bị xóa vĩnh viễn và điểm đánh giá trung bình của gia sư sẽ được tự động tính toán lại."
                font.pixelSize: 12
                color: Theme.textDark
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10

                Button {
                    text: "Hủy bỏ"
                    onClicked: deleteConfirmModal.close()
                }

                Button {
                    text: "Xác nhận xóa"
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        color: "#FFFFFF"
                    }
                    background: Rectangle {
                        color: "#cf1322"
                        radius: 4
                    }
                    onClicked: {
                        deleteConfirmModal.close()
                        if (typeof guiBridge !== "undefined") {
                            guiBridge.qmlDeleteReview(adminReviewsView.pendingDeleteId)
                            adminReviewsView.reloadReviews()
                        }
                    }
                }
            }
        }
    }
}
