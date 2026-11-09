#include "../../include/ds/ScheduleGuard.h"
#include <sstream>

namespace TutorMatch {

static const char* DAY_NAMES[] = {"Thứ 2", "Thứ 3", "Thứ 4", "Thứ 5", "Thứ 6", "Thứ 7", "Chủ Nhật"};
static const char* SLOT_TIMES[] = {
    "07:30-09:00", "09:15-10:45", "13:30-15:00", "15:15-16:45",
    "17:00-18:30", "18:45-20:15", "20:30-22:00", "22:00-23:30"
};

std::string ScheduleGuard::formatSlotSingle(int dayIndex, int slotIndex) {
    if (dayIndex < 0 || dayIndex >= DAYS_PER_WEEK || slotIndex < 0 || slotIndex >= SLOTS_PER_DAY) {
        return "Ca học không hợp lệ";
    }
    return std::string(DAY_NAMES[dayIndex]) + " (" + SLOT_TIMES[slotIndex] + ")";
}

std::string ScheduleGuard::formatSchedule(uint64_t mask) {
    if (mask == 0) return "Không có lịch rảnh";

    std::ostringstream oss;
    bool first = true;
    for (int day = 0; day < DAYS_PER_WEEK; ++day) {
        bool dayHasSlot = false;
        std::ostringstream dayOss;
        dayOss << DAY_NAMES[day] << " [";

        for (int slot = 0; slot < SLOTS_PER_DAY; ++slot) {
            uint64_t sMask = getSlotMask(day, slot);
            if ((mask & sMask) != 0) {
                if (dayHasSlot) dayOss << ", ";
                dayOss << SLOT_TIMES[slot];
                dayHasSlot = true;
            }
        }
        dayOss << "]";

        if (dayHasSlot) {
            if (!first) oss << " | ";
            oss << dayOss.str();
            first = false;
        }
    }
    return oss.str();
}

} // namespace TutorMatch
