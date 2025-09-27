#ifndef P1_HELPER_H
#define P1_HELPER_H

#include <iostream>
#include <string>
#include <vector>

/**
 * @struct Course
 * @brief A structure to hold the data for a single course.
 */
struct Course {
    std::string course_code;
    std::string title;
    std::string subject;
    std::string instructor;
    std::vector<std::string> prerequisites;
    int seats_available;
    int capacity;
    std::string description;
};

/**
 * @brief Loads course data from the specified file into memory.
 * @param filename The name of the database file (e.g., "courses.db").
 * @return A vector of Course structs containing all loaded courses.
 *
 * This function handles file I/O, parsing of semicolon-separated values, and
 * populating a vector with Course structs.
 */
std::vector<Course> load_courses_from_db(const std::string& filename);

/**
 * @brief Searches for courses based on a filter and search term.
 * @param courses The vector of all courses.
 * @param filter The category to search by (e.g., "subject", "instructor", "course-code").
 * @param search_term The term to search for.
 * @return A vector of matching Course structs.
 */
std::vector<Course> search_courses(const std::vector<Course>& courses, const std::string& filter, const std::string& search_term);

/**
 * @brief Retrieves a specific course by its course code.
 * @param courses The vector of all courses.
 * @param course_code The unique identifier for the course.
 * @return The matching Course struct, or a default constructed Course if not found.
 */
Course get_course_by_code(const std::vector<Course>& courses, const std::string& course_code);

/**
 * @brief Enrolls a student in a course, decrementing the available seats.
 * @param courses The vector of all courses.
 * @param course_code The course to enroll in.
 * @return true if enrollment was successful, false otherwise.
 */
bool enroll_in_course(std::vector<Course>& courses, const std::string& course_code);

/**
 * @brief Drops a student from a course, incrementing the available seats.
 * @param courses The vector of all courses.
 * @param course_code The course to drop.
 * @return true if drop was successful, false otherwise.
 */
bool drop_course(std::vector<Course>& courses, const std::string& course_code);

/**
 * @brief Checks if a student meets the prerequisites for a course.
 * @param enrolled_courses A vector of courses the student is already enrolled in.
 * @param course_to_enroll The course the student wants to enroll in.
 * @return true if all prerequisites are met, false otherwise.
 */
bool check_prerequisites(const std::vector<Course>& enrolled_courses, const Course& course_to_enroll);

#endif // P1_HELPER_H
