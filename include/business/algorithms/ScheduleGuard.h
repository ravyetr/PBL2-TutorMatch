#ifndef TUTOR_MATCH_SCHEDULE_GUARD_H
#define TUTOR_MATCH_SCHEDULE_GUARD_H

#include <cstdint>
#include <string>
#include <vector>

namespace TutorMatch {

/**
 * Smart Schedule Guard using 64-bit Bitmasks.
 * 
 * 7 Days a week (Mon-Sun), 8 slots per day (08:00 to 23:00):
 * Total 56 active slots fit neatly inside a single uint64_t bitmask.
 * 
 * Slot indexing: bitOffset = (dayIndex * 8) + slotIndex
 * Day indices: 0 = Mon, 1 = Tue, ..., 6 = Sun
 * Slot indices: 0-7 per day
 */
class ScheduleGuard {
public:
    static const int DAYS_PER_WEEK = 7;
    static const int SLOTS_PER_DAY = 8;
    static const int TOTAL_SLOTS = 56;
    static constexpr uint64_t VALID_SCHEDULE_MASK = 0x00FFFFFFFFFFFFFFULL; // Exact 56 valid slots (bits 0..55)

    // Sanitizes bitmask by stripping any phantom/ghost bits outside the 56 valid slots
    static uint64_t sanitizeMask(uint64_t mask) {
        return mask & VALID_SCHEDULE_MASK;
    }

    // Checks if tutor's schedule mask covers all bits requested
    static bool isSlotAvailable(uint64_t tutorScheduleMask, uint64_t requestedSlotMask) {
        uint64_t cleanTutor = sanitizeMask(tutorScheduleMask);
        uint64_t cleanReq = sanitizeMask(requestedSlotMask);
        return (cleanTutor & cleanReq) == cleanReq;
    }

    // Detects collision between two active schedule masks
    static bool hasConflict(uint64_t existingMask, uint64_t newMask) {
        return (sanitizeMask(existingMask) & sanitizeMask(newMask)) != 0;
    }

    // Calculate single slot bitmask
    static uint64_t getSlotMask(int dayIndex, int slotIndex) {
        if (dayIndex < 0 || dayIndex >= DAYS_PER_WEEK || slotIndex < 0 || slotIndex >= SLOTS_PER_DAY) {
            return 0;
        }
        int bitOffset = dayIndex * SLOTS_PER_DAY + slotIndex;
        return (1ULL << bitOffset);
    }

    // Set/Toggle slot in bitmask
    static uint64_t setSlot(uint64_t mask, int dayIndex, int slotIndex, bool available) {
        uint64_t sMask = getSlotMask(dayIndex, slotIndex);
        uint64_t cleanMask = sanitizeMask(mask);
        if (available) {
            return sanitizeMask(cleanMask | sMask);
        } else {
            return sanitizeMask(cleanMask & (~sMask));
        }
    }

    // Count common available slots strictly within the 56 valid slots
    static int countMatchingSlots(uint64_t tutorMask, uint64_t studentMask) {
        uint64_t common = sanitizeMask(tutorMask) & sanitizeMask(studentMask);
        int count = 0;
        while (common > 0) {
            count += static_cast<int>(common & 1ULL);
            common >>= 1ULL;
        }
        return count;
    }

    // Format single slot description
    static std::string formatSlotSingle(int dayIndex, int slotIndex);

    // Format full bitmask schedule string
    static std::string formatSchedule(uint64_t mask);
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_SCHEDULE_GUARD_H
