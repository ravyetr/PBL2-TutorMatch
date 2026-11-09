import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: studentTutorSearchView

    property var tutorList: []

    ListModel {
        id: subjectModel
        ListElement { text: "Tất cả môn"; value: "" }
        ListElement { text: "Toán"; value: "math" }
        ListElement { text: "Vật lý"; value: "physics" }
        ListElement { text: "Hóa học"; value: "chemistry" }
        ListElement { text: "Ngoại ngữ"; value: "english" }
        ListElement { text: "Tin học"; value: "computer_science" }
        ListElement { text: "Sinh học"; value: "biology" }
        ListElement { text: "Văn học"; value: "literature" }
    }

    function reloadTutors() {
        var kw = searchKeywordInput.text || ""
        var subj = (subjectCombo.currentValue !== undefined && subjectCombo.currentValue !== null) ? subjectCombo.currentValue : (subjectCombo.currentText || "")
        var minP = parseFloat(minPriceInput.text) || 0.0
        var maxP = parseFloat(maxPriceInput.text) || 0.0
        var minR = 0.0
        if (ratingCombo.currentIndex === 1) minR = 3.0
        else if (ratingCombo.currentIndex === 2) minR = 4.0
        else if (ratingCombo.currentIndex === 3) minR = 4.5

        if (typeof guiBridge !== "undefined") {
            tutorList = guiBridge.searchAndFilterTutors(kw, subj, minP, maxP, minR)
        }
    }

    function resetFilters() {
        searchKeywordInput.text = ""
        subjectCombo.currentIndex = 0
        minPriceInput.text = ""
        maxPriceInput.text = ""
        ratingCombo.currentIndex = 0
        if (typeof guiBridge !== "undefined") {
            guiBridge.clearStatusMessage()
        }
        reloadTutors()
    }

    Component.onCompleted: {
        reloadTutors()
    }

    Connections {
        target: (typeof guiBridge !== "undefined") ? guiBridge : null
        ignoreUnknownSignals: true
        function onDataChanged() {
            reloadTutors()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        // Page Title & Header
        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 4
                Text {
                    text: "TÌM KIẾM & KẾT NỐI GIA SƯ THÔNG MINH"
                    font.bold: true
                    font.pixelSize: 18
                    color: Theme.textDark
                }
                Text {
                    text: "Tìm kiếm, lọc danh sách gia sư chất lượng cao và đăng ký ca học trực tuyến"
                    font.pixelSize: 12
                    color: Theme.textMuted
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "Tìm thấy " + studentTutorSearchView.tutorList.length + " gia sư"
                font.bold: true
                font.pixelSize: 13
                color: "#d97706"
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
                }
            }
        }

        // Filter Toolbar Card
        Rectangle {
            Layout.fillWidth: true
            height: 90
            color: Theme.cardBg
            radius: 10
            border.color: Theme.cardBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    // Keyword Search Input
                    ColumnLayout {
                        spacing: 2
                        Layout.preferredWidth: 200
                        Text { text: "Từ khóa tìm kiếm:"; font.pixelSize: 11; font.bold: true; color: Theme.textDark }
                        TextField {
                            id: searchKeywordInput
                            Layout.fillWidth: true
                            height: 32
                            placeholderText: "Nhập tên gia sư, môn học..."
                            color: Theme.textDark
                            background: Rectangle { color: Theme.bgApp; radius: 6; border.color: searchKeywordInput.activeFocus ? Theme.sidebarBg : Theme.cardBorder }
                            onTextChanged: studentTutorSearchView.reloadTutors()
                        }
                    }

                    // Subject Selector
                    ColumnLayout {
                        spacing: 2
                        Layout.preferredWidth: 140
                        Text { text: "Môn học:"; font.pixelSize: 11; font.bold: true; color: Theme.textDark }
                        ComboBox {
                            id: subjectCombo
                            Layout.fillWidth: true
                            height: 32
                            model: subjectModel
                            textRole: "text"
                            valueRole: "value"
                            onCurrentIndexChanged: studentTutorSearchView.reloadTutors()
                        }
                    }

                    // Min Price Input
                    ColumnLayout {
                        spacing: 2
                        Layout.preferredWidth: 120
                        Text { text: "Giá tối thiểu (VNĐ):"; font.pixelSize: 11; font.bold: true; color: Theme.textDark }
                        TextField {
                            id: minPriceInput
                            Layout.fillWidth: true
                            height: 32
                            placeholderText: "Từ 0..."
                            validator: DoubleValidator { bottom: 0; top: 10000000 }
                            color: Theme.textDark
                            background: Rectangle { color: Theme.bgApp; radius: 6; border.color: Theme.cardBorder }
                        }
                    }

                    // Max Price Input
                    ColumnLayout {
                        spacing: 2
                        Layout.preferredWidth: 120
                        Text { text: "Giá tối đa (VNĐ):"; font.pixelSize: 11; font.bold: true; color: Theme.textDark }
                        TextField {
                            id: maxPriceInput
                            Layout.fillWidth: true
                            height: 32
                            placeholderText: "Đến..."
                            validator: DoubleValidator { bottom: 0; top: 10000000 }
                            color: Theme.textDark
                            background: Rectangle { color: Theme.bgApp; radius: 6; border.color: Theme.cardBorder }
                        }
                    }

                    // Rating Filter ComboBox
                    ColumnLayout {
                        spacing: 2
                        Layout.preferredWidth: 130
                        Text { text: "Đánh giá Rating:"; font.pixelSize: 11; font.bold: true; color: Theme.textDark }
                        ComboBox {
                            id: ratingCombo
                            Layout.fillWidth: true
                            height: 32
                            model: ["Tất cả", "3.0★ trở lên", "4.0★ trở lên", "4.5★ trở lên"]
                            onCurrentIndexChanged: studentTutorSearchView.reloadTutors()
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Action Buttons
                    RowLayout {
                        spacing: 8
                        Layout.alignment: Qt.AlignBottom

                        Button {
                            text: "Lọc kết quả"
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
                                implicitHeight: 32
                                radius: 6
                                color: parent.hovered ? Theme.cSilverLakeBlue : Theme.sidebarBg
                            }
                            onClicked: studentTutorSearchView.reloadTutors()
                        }

                        Button {
                            text: "Đặt lại bộ lọc"
                            contentItem: Text {
                                text: parent.text
                                font.bold: true
                                font.pixelSize: 12
                                color: Theme.textDark
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                implicitWidth: 100
                                implicitHeight: 32
                                radius: 6
                                color: parent.hovered ? Theme.bgApp : Theme.accentBlue
                                border.color: Theme.cardBorder
                            }
                            onClicked: studentTutorSearchView.resetFilters()
                        }
                    }
                }
            }
        }

        // Empty State Banner
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.cardBg
            radius: 10
            border.color: Theme.cardBorder
            border.width: 1
            visible: studentTutorSearchView.tutorList.length === 0

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 12

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "🔍 Không tìm thấy gia sư phù hợp"
                    font.bold: true
                    font.pixelSize: 18
                    color: Theme.textDark
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Vui lòng điều chỉnh hoặc thử lại với bộ lọc tìm kiếm khác."
                    font.pixelSize: 13
                    color: Theme.textMuted
                }
                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Đặt lại tất cả bộ lọc"
                    contentItem: Text {
                        text: parent.text
                        font.bold: true
                        color: Theme.btnText
                    }
                    background: Rectangle {
                        implicitWidth: 160
                        implicitHeight: 36
                        radius: 6
                        color: Theme.btnPrimary
                    }
                    onClicked: studentTutorSearchView.resetFilters()
                }
            }
        }

        // Tutor Grid Catalog
        GridView {
            id: tutorGridView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cellWidth: (tutorGridView.width - 20) / 2
            cellHeight: 220
            visible: studentTutorSearchView.tutorList.length > 0
            model: studentTutorSearchView.tutorList

            delegate: Item {
                width: tutorGridView.cellWidth - 10
                height: tutorGridView.cellHeight - 10

                TutorCard {
                    anchors.fill: parent
                    tutorData: modelData

                    onDetailRequested: function(tutor) {
                        tutorDetailModal.openWithTutorId(tutor.id)
                    }

                    onBookingRequested: function(tutor) {
                        bookingModal.openWithTutor(tutor)
                    }
                }
            }
        }
    }

    // Modals
    TutorDetailModal {
        id: tutorDetailModal
        onBookCourseRequested: function(tutor) {
            bookingModal.openWithTutor(tutor)
        }
    }

    BookingModal {
        id: bookingModal
        onBookingCompleted: {
            studentTutorSearchView.reloadTutors()
        }
    }
}
