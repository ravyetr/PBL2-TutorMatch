# TÀI LIỆU THAM CHIẾU LỚP VÀ PHƯƠNG THỨC (CLASS & METHOD REFERENCE)
## Đặc Tả Toàn Diện Chữ Ký Phương Thức C++, Thuộc Tính QML và Hợp Đồng Nghiệp Vụ

---

## 1. PHÂN HỆ TRÌNH DIỄN: BỘ ĐIỀU HỢP `GUIBridge` `[VERIFIED]`

- **Đường dẫn tệp:** [`include/presentation/bridges/GUIBridge.h`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/include/presentation/bridges/GUIBridge.h) và [`src/presentation/bridges/GUIBridge.cpp`](file:///C:/Users/DELL/.gemini/antigravity/scratch/TutorMatch/src/presentation/bridges/GUIBridge.cpp)
- **Kế thừa:** `public QObject`

### 1.1. Thuộc Tính QML (`Q_PROPERTY`)
```cpp
Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY stateChanged)
Q_PROPERTY(QString currentUserRole READ getCurrentUserRole NOTIFY stateChanged)
Q_PROPERTY(QString currentUserFullName READ getCurrentUserFullName NOTIFY stateChanged)
Q_PROPERTY(QString statusMessage READ getStatusMessage NOTIFY statusMessageChanged)
```

### 1.2. Tín Hiệu (`Q_SIGNAL`)
```cpp
void stateChanged();
void statusMessageChanged();
```

### 1.3. Nhóm Phương Thức Xác Thực & Phiên (`Q_INVOKABLE`)
- `bool qmlLogin(const QString& username, const QString& password);`
  - Xác thực người dùng, thiết lập `currentUser`, phát tín hiệu `stateChanged`.
- `void qmlLogout();`
  - Đóng phiên làm việc, xóa trạng thái, điều hướng về màn hình đăng nhập.
- `bool qmlRegisterStudent(const QString& username, const QString& password, const QString& fullName, const QString& email, int gradeLevel, const QString& preferredSubject, double maxBudget);`
  - Đăng ký tài khoản học viên mới vào hệ thống.
- `bool qmlRegisterTutor(const QString& username, const QString& password, const QString& fullName, const QString& email, const QString& subjectSpecialty, double hourlyRate, const QString& bio);`
  - Đăng ký tài khoản gia sư mới vào hệ thống.

### 1.4. Nhóm Phương Thức Thời Khóa Biểu & Lịch Học (`Q_INVOKABLE`)
- `QVariantList getWeeklyScheduleMatrix(const QString& weekStartDateStr = "");`
  - Trả về ma trận 56 ô thời khóa biểu tuần của người dùng hiện tại (hợp nhất lịch bận và khóa học).
- `QVariantList getStudentWeeklySchedule(const QString& weekStartDateStr = "");`
  - Trả về danh sách chi tiết các buổi học trong tuần của học viên.
- `QVariantList getTutorWeeklySchedule(const QString& weekStartDateStr = "");`
  - Trả về danh sách chi tiết các buổi học trong tuần của gia sư.
- `bool qmlToggleStudentScheduleSlot(int dayIndex, int slotIndex);`
  - Đảo trạng thái rảnh/bận của một ca học đối với học viên.
- `bool qmlToggleTutorScheduleSlot(int dayIndex, int slotIndex);`
  - Đảo trạng thái sẵn sàng nhận dạy của một ca học đối với gia sư.
- `bool clearStudentBusySchedule();`
  - Đặt lại toàn bộ 56 ca của học viên về trạng thái rảnh.
- `QVariantList getTutorAvailableBookingSlots(int tutorId, int studentId);`
  - Lấy danh sách các ca học khả dụng mà gia sư rảnh và không bị trùng khóa học nào.

### 1.5. Nhóm Phương Thức Tìm Kiếm & Ghép Đôi SmartMatch (`Q_INVOKABLE`)
- `QVariantList searchAndFilterTutors(const QString& keyword, const QString& subjectFilter, double minPrice, double maxPrice, double minRating);`
  - Tìm kiếm gia sư từ khóa tự do qua `TutorSearchMatcher`.
- `QVariantList qmlSearchSmartMatches(const QString& subject, int gradeLevel, double maxBudget, const QVariantList& requestedSlots, const QString& startDate, int weeksCount, bool strictSchedule, double minRating = 0.0);`
  - Thực thi thuật toán gợi ý tối ưu SmartMatch qua `MatchingEngine`.

### 1.6. Nhóm Phương Thức Quản Lý Khóa Học (`Q_INVOKABLE`)
- `bool requestCourseBookingMulti(int tutorId, const QString& subject, double hourlyRate, const QString& startDate, int weeksCount, const QVariantList& weeklySlots, bool acknowledgeCommitment);`
  - Học viên tạo đơn đăng ký khóa học nhiều tuần kèm cam kết `COMMITMENT_V1`.
- `bool qmlAcceptBooking(int courseBookingId, bool tutorAcknowledgeCommitment);`
  - Gia sư chấp nhận yêu cầu khóa học kèm cam kết.
- `bool qmlRejectBooking(int courseBookingId);`
  - Gia sư từ chối yêu cầu khóa học.
- `bool qmlCancelBooking(int courseBookingId);`
  - Học viên hoặc Admin hủy đơn đăng ký khóa học.
- `bool requestEarlyTermination(int courseBookingId, const QString& reason);`
  - Học viên hoặc Gia sư đề xuất kết thúc sớm khóa học.
- `bool respondEarlyTermination(int courseBookingId, bool agree);`
  - Phản hồi đồng thuận kết thúc sớm khóa học (đồng ý hoặc từ chối).

### 1.7. Nhóm Phương Thức Đánh Giá & Quản Trị (`Q_INVOKABLE`)
- `bool qmlSubmitReview(int bookingId, int ratingScore, const QString& comment);`
  - Học viên gửi đánh giá cho khóa học đã hoàn tất hoặc kết thúc sớm.
- `bool qmlToggleUserLock(int userId);`
  - Admin thực hiện khóa hoặc mở khóa tài khoản người dùng.
- `bool qmlDeleteViolatingReview(int reviewId);`
  - Admin xóa bài đánh giá vi phạm tiêu chuẩn.
- `bool qmlTerminateMatch(int bookingId);`
  - Admin hủy cưỡng chế một khóa học đang hoạt động.
- `QVariantMap getAdminMetrics();`
  - Lấy số liệu thống kê tổng thể toàn sàn.

---

## 2. PHÂN HỆ CÁC DỊCH VỤ NGHIỆP VỤ (BUSINESS SERVICES) `[VERIFIED]`

### 2.1. `AuthService`
```cpp
explicit AuthService(std::shared_ptr<IUserRepository> repository);
bool login(const std::string& username, const std::string& password, std::string& outError);
void logout();
bool isLoggedIn() const;
std::shared_ptr<User> getCurrentUser() const;
UserRole getCurrentRole() const;
bool registerStudent(const std::string& username, const std::string& password, const std::string& fullName, const std::string& email, const std::string& gradeLevel, std::string& outError);
bool registerTutor(const std::string& username, const std::string& password, const std::string& fullName, const std::string& email, const std::string& subjectSpecialty, double hourlyRate, const std::string& bio, std::string& outError);
```

### 2.2. `UserService`
```cpp
explicit UserService(std::shared_ptr<IUserRepository> repository);
std::shared_ptr<User> getUserById(int userId) const;
std::shared_ptr<Student> getStudentById(int studentId) const;
std::shared_ptr<Tutor> getTutorById(int tutorId) const;
std::vector<std::shared_ptr<Tutor>> getAllTutors() const;
bool updateStudentProfile(int studentId, const std::string& fullName, const std::string& email, int gradeLevel, std::string& outError);
bool updateTutorProfile(int tutorId, const std::string& fullName, const std::string& email, const std::string& bio, const std::string& subjectSpecialty, double hourlyRate, std::string& outError);
bool updateUserSchedule(int userId, uint64_t newBitmask, std::string& outError);
```

### 2.3. `BookingService`
```cpp
BookingService(std::shared_ptr<IUserRepository> uRepo,
               std::shared_ptr<IReviewRepository> rRepo,
               std::shared_ptr<ICourseBookingRepository> cbRepo);
bool hasCourseConflict(int studentId, int tutorId, int dayOfWeek, int slotIndex, const std::string& startDate, int weeksCount, int excludeBookingId = 0) const;
bool hasCourseConflictMulti(int studentId, int tutorId, const std::vector<std::pair<int, int>>& slots, const std::string& startDate, int weeksCount, int excludeBookingId = 0, std::string* outDetails = nullptr, std::string* outConflictDate = nullptr) const;
bool bookCourseMultiSlot(int studentId, int tutorId, const std::string& subject, double hourlyRate, const std::string& startDate, int weeksCount, const std::vector<std::pair<int, int>>& weeklySlots, bool studentAcknowledgeCommitment, std::string& outMsg, int* outBookingId = nullptr);
bool acceptCourseBooking(int tutorId, int courseBookingId, bool tutorAcknowledgeCommitment, std::string& outMsg, std::string* outConflictDate = nullptr);
bool rejectCourseBooking(int tutorId, int courseBookingId, std::string& outMsg);
bool cancelCourseBooking(int operatorUserId, UserRole operatorRole, int courseBookingId, std::string& outMsg);
bool completeCourseBooking(int operatorUserId, UserRole operatorRole, int courseBookingId, std::string& outMsg);
void autoCompleteExpiredCourses();
double calculateTutorAccumulatedIncome(int tutorId) const;
void reconcileTutorEarnings(int tutorId);
void reconcileAllTutorEarningsOnStartup();
bool requestEarlyTermination(int operatorUserId, UserRole operatorRole, int courseBookingId, const std::string& reason, std::string& outMsg);
bool respondEarlyTermination(int operatorUserId, UserRole operatorRole, int courseBookingId, bool agree, std::string& outMsg);
std::vector<TimetableSessionDTO> getAcceptedSessionsForWeek(int userId, UserRole role, const std::string& weekStartDate) const;
```

### 2.4. `ReviewService`
```cpp
ReviewService(std::shared_ptr<IReviewRepository> rRepo,
              std::shared_ptr<IUserRepository> uRepo,
              std::shared_ptr<ICourseBookingRepository> cbRepo = nullptr);
bool submitReview(int studentId, int bookingId, int ratingScore, const std::string& comment, std::string& outMsg);
bool deleteReview(int reviewId, std::string& outMsg);
std::vector<Review> getReviewsByTutorId(int tutorId) const;
std::vector<Review> getReviewsByStudentId(int studentId) const;
std::vector<Review> getAllReviews() const;
void recalculateTutorRating(int tutorId);
bool calculateTutorRating(int tutorId, double& outRating, int& outCount) const;
void reconcileRatingsOnStartup();
bool isEligibleForReview(int studentId, int bookingId, std::string& outReason) const;
```

### 2.5. `MatchingService` & `MatchingEngine`
```cpp
// MatchingService
MatchingService(std::shared_ptr<IUserRepository> uRepo,
                std::shared_ptr<ICourseBookingRepository> cbRepo = nullptr,
                std::shared_ptr<MatchingEngine> engine = nullptr);
std::vector<TutorMatchResult> findTopMatchesWithCriteria(const SmartMatchCriteria& criteria, size_t topK = 5);

// MatchingEngine
void setStrategy(std::unique_ptr<IMatchingStrategy> newStrategy);
IMatchingStrategy* getStrategy() const;
static bool isEligible(const SmartMatchCriteria& criteria, const Tutor& tutor, const std::vector<CourseBooking>& activeBookings, std::vector<std::pair<int, int>>& outMatchedSlots);
std::vector<TutorMatchResult> findTopMatchesWithCriteria(const SmartMatchCriteria& criteria, const std::vector<std::shared_ptr<Tutor>>& tutors, const std::vector<CourseBooking>& activeBookings = {}, size_t topK = 5);
```

### 2.6. `AdminService`
```cpp
AdminService(std::shared_ptr<IUserRepository> uRepo,
             std::shared_ptr<IReviewRepository> rRepo,
             std::shared_ptr<BookingService> bService);
AdminMetrics calculateMetrics();
std::vector<std::shared_ptr<User>> getAllUsers();
std::vector<std::shared_ptr<User>> getFilteredUsers(const std::string& filter, const std::string& searchKeyword);
bool toggleUserLock(int userId, std::string& outMsg);
bool deleteViolatingReview(int reviewId, std::string& outMsg);
bool terminateMatch(int bookingId, std::string& outMsg);
```

---

## 3. PHÂN HỆ TRUY XUẤT DỮ LIỆU & TUẦN TỰ HÓA `[VERIFIED]`

### 3.1. Các Giao Diện Kho Lưu Trữ (`IRepository.h`)
```cpp
// IUserRepository
virtual int getNextUserId() = 0;
virtual bool saveUser(const User& user) = 0;
virtual std::shared_ptr<User> findUserById(int userId) = 0;
virtual std::shared_ptr<User> findUserByUsername(const std::string& username) = 0;
virtual bool updateUser(const User& user) = 0;
virtual bool deleteUserSoft(int userId) = 0;
virtual std::vector<std::shared_ptr<Tutor>> getAllTutors() = 0;
virtual std::vector<std::shared_ptr<Student>> getAllStudents() = 0;
virtual std::vector<std::shared_ptr<User>> getAllUsers() = 0;
virtual std::vector<int> findTutorIdsByPriceRange(double minRate, double maxRate) = 0;
virtual std::vector<int> findTutorIdsByMinRating(double minRating) = 0;

// ICourseBookingRepository
virtual int getNextCourseBookingId() = 0;
virtual bool saveCourseBooking(const CourseBooking& booking) = 0;
virtual std::shared_ptr<CourseBooking> findCourseBookingById(int bookingId) = 0;
virtual bool updateCourseBooking(const CourseBooking& booking) = 0;
virtual std::vector<CourseBooking> getCourseBookingsByStudentId(int studentId) = 0;
virtual std::vector<CourseBooking> getCourseBookingsByTutorId(int tutorId) = 0;
virtual std::vector<CourseBooking> getAllCourseBookings() = 0;

// IReviewRepository
virtual int getNextReviewId() = 0;
virtual bool saveReview(const Review& review) = 0;
virtual std::vector<Review> getReviewsByTutorId(int tutorId) = 0;
virtual std::vector<Review> getReviewsByStudentId(int studentId) = 0;
virtual std::vector<Review> getAllReviews() = 0;
virtual bool deleteReview(int reviewId) = 0;
```

### 3.2. Các Bộ Tuần Tự Hóa (Serializers)
```cpp
// UserSerializer
static std::string toCsv(const User& user);
static std::shared_ptr<User> fromCsv(const std::string& line);

// CourseBookingSerializer
static std::string toCsv(const CourseBooking& booking);
static bool fromCsv(const std::string& line, CourseBooking& outBooking);
static bool isValidStatus(const std::string& status);

// ReviewSerializer
static std::string toCsv(const Review& review);
static bool fromCsv(const std::string& line, Review& outReview);

// SerializerUtils
static std::string escapeCsv(const std::string& text);
static std::string unescapeCsv(const std::string& text);
static std::vector<std::string> splitCsvLine(const std::string& line);
static int parseIntSafe(const std::string& str, int defaultValue = 0);
static double parseDoubleSafe(const std::string& str, double defaultValue = 0.0);
static uint64_t parseUInt64Safe(const std::string& str, uint64_t defaultValue = 0);
```
