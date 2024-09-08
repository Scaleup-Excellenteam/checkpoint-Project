//
// Created by Saleh on 08/09/2024.
//

#ifndef CHECKPOINT_STUDENT_MANAGEMENT_SYSTEM_H
#define CHECKPOINT_STUDENT_MANAGEMENT_SYSTEM_H

#define HASH_SIZE 10007
#define MAX_NAME 20
#define MAX_PHONE 15
#define MAX_GRADES 12
#define MAX_CLASSES 10
#define SUBJECTS 10
typedef struct{
    char first_name[MAX_NAME];
    char last_name[MAX_NAME];
    char phone[MAX_PHONE];
    int grade;
    int class;
    int grades[10];
    double average_grade;
    struct Student* next;
} Student;

typedef struct {
    Student** students;
    int capacity;
    int num_students;
    int class_id;
} Class;

typedef struct {
    Class classes[10];
    int grade_id;
    int num_classes;
} Grade;

typedef struct {
    Student* buckets[HASH_SIZE];
    int size;
} HashTable;

typedef struct {
    HashTable hash_table;
    Grade grades[12];
    int num_of_grades;
    int total_students;
} School;



//functions
unsigned long hash(const char* first_name, const char* last_name);
HashTable* create_hash_table();
School* read_data_from_file(const char* file_name);
School* create_school();
void insert_student(School* school, Student* student);
void insertNewStudent(School* school);
void deleteStudent(School* school);
void editStudentGrade(School* school);
void searchStudent(School* school);
void printAllStudents(School* school);
void printTopNStudentsPerCourse(School* school);
void printUnderperformedStudents(School* school, int threshold);
void printAverage(School* school);
void exportDatabase(School* school, const char* file_name);
void destroySchool(School* school);

#endif //CHECKPOINT_STUDENT_MANAGEMENT_SYSTEM_H
